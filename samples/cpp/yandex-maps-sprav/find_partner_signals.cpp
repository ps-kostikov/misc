#include <yandex/maps/sprav/matching/matching.h>
#include <yandex/maps/sprav/model/storages/revision.h>
#include <yandex/maps/sprav/model/convert.h>
#include <yandex/maps/sprav/common/config.h>
#include <yandex/maps/sprav/common/stopwatch.h>
#include <yandex/maps/sprav/algorithms/datamodel/company.h>
#include <yandex/maps/sprav/algorithms/similarity/similarity.h>

#include <yandex/maps/mining/sprav2miningson.h>

#include <yandex/maps/log.h>
#include <yandex/maps/pgpool2/pgpool.h>
#include <yandex/maps/common/exception.h>
#include <yandex/maps/json/builder.h>
#include <yandex/maps/json/value.h>
#include <yandex/maps/json/std.h>
#include <yandex/maps/geolib3/conversion.h>
#include <yandex/maps/threadpool/threadpool.h>

#include <boost153/program_options.hpp>
#include <boost153/python/object.hpp>
#include <boost153/python.hpp>
#include <boost153/algorithm/string.hpp>
#include <boost153/optional.hpp>

#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <thread>


namespace json = maps::json;
namespace sprav = maps::sprav;
namespace model = maps::sprav::model;
namespace algorithms = maps::sprav::algorithms;


struct ReadSessionHolder {
    ReadSessionHolder(maps::pgpool2::ConnectionMgr& connMgr) :
        transaction(connMgr.readTransaction()),
        storage(new model::RevisionStorage(*transaction)),
        session(storage, model::Session::CachingPolicy::CacheAll)
    {}

    maps::pgpool2::PgTransaction transaction;
    std::shared_ptr<model::RevisionStorage> storage;
    model::Session session;
};

inline std::string formatCurrentPythonException()
{
    PyObject* exceptionTypePtr;
    PyObject* exceptionValuePtr;
    PyObject* tracebackPtr;
    PyErr_Fetch(&exceptionTypePtr, &exceptionValuePtr, &tracebackPtr);
    boost153::python::handle<> exceptionType(exceptionTypePtr);
    boost153::python::handle<> exceptionValue(boost153::python::allow_null(exceptionValuePtr));
    boost153::python::handle<> traceback(boost153::python::allow_null(tracebackPtr));

    const auto messageLines = boost153::python::import("traceback").attr("format_exception")(
        exceptionType, exceptionValue, traceback);
    return boost153::python::extract<std::string>(boost153::python::str().join(messageLines));
}

template <typename F>
inline void wrapPythonExceptions(F f)
{
    try {
        f();
    } catch (const boost153::python::error_already_set&) {
        const std::string pythonException = formatCurrentPythonException();
        std::cout << "Python error occurred:\n" << pythonException << std::endl;
    }
}

std::map<std::string, std::vector<model::ID>>
fetchIdsMap(const std::string& filename)
{
    std::ifstream is(filename.c_str());
    std::string idStr;
    std::map<std::string, std::vector<model::ID>> result;
    while(std::getline(is, idStr)) {
        std::vector<std::string> strs;
        boost153::split(strs, idStr, boost153::is_any_of(" "));
        auto objectId = boost153::lexical_cast<model::ID>(strs[1]);
        result[strs[0]].push_back(objectId);
    }
    return result;
}

double
calcSimilarity(
    const model::Company& company,
    const algorithms::Company& algCompany,
    model::Session& session)
try {
    auto ac = model::convertCompany(company, session);
    return algorithms::calculateSimilarity(ac, algCompany);
} catch (model::ObjectsBroken&) {
    return 0.;
} catch (model::ObjectNotFound&) {
    return 0.;
}

struct ExperimentInput
{
    algorithms::Company algCompany;
    std::string partnerCardId;
    std::vector<model::Company> rightAnswers;
    double bestRightSimilarity;
};

typedef std::vector<ExperimentInput> ExperimentInputs;

class ExperimentPreparer
{
public:
    ExperimentPreparer(const std::string& minigsonFilename,
                       const std::string& answersFilename,
                       boost153::optional<size_t> offsetNumber)
    {
        Py_Initialize();
        boost153::python::register_exception_translator<maps::mining::sprav2miningson::ConversionFailedError>(
            [](const maps::mining::sprav2miningson::ConversionFailedError& exc) {
                const std::string message =
                    exc.attrs_.count("pythonException") ? exc.attrs_.at("pythonException") : exc.what();
                PyErr_SetString(PyExc_ValueError, message.c_str());
            });

        partnerToObject_ = fetchIdsMap(answersFilename);

        wrapPythonExceptions([&]() {
            auto package = boost153::python::import("yandex.maps.mining.miningson");
            reader_ = package.attr("card_reader")(minigsonFilename);

            if (offsetNumber) {
                for (size_t i = 0; i < *offsetNumber; ++i) {
                    try {
                        reader_.attr("next")();
                    } catch (...) {
                        break;
                    }
                }
            }
        });

    }

    boost153::optional<ExperimentInput> next(model::Session& session)
    {
        while (true) {
            boost153::python::object card;
            try {
                card = reader_.attr("next")();
            } catch (...) {
                return boost153::none;
            }

            std::string cardId;
            try {
                cardId = boost153::python::extract<std::string>(
                    card.attr("raw_origin")["partner_card_id"].attr("__str__")());
            } catch (...) {
                ++brokenCount_;
                continue;
            }

            std::unique_ptr<algorithms::Company> algCompany;
            try {
                algCompany.reset(
                    new algorithms::Company(maps::mining::sprav2miningson::toSpravCompany(card))
                );
            } catch (...) {
                ++brokenCount_;
                continue;
            }

            auto rightAnswersIds = partnerToObject_[cardId];
            if (rightAnswersIds.size() == 0) {
                ++brokenCount_;
                continue;
            }
            if (rightAnswersIds.size() > 3) {
                ++brokenCount_;
                continue;
            }
            auto rightAnswers = session.loadCompany(rightAnswersIds);

            double bestRightSim = 0.;
            for (auto ra: rightAnswers) {
                auto sim = calcSimilarity(ra, *algCompany, session);
                if (sim > bestRightSim) {
                    bestRightSim = sim;
                }
            }

            if (bestRightSim < 0.65) {
                ++lowSimilarityCount_;
                continue;
            }

            ++experimentCount_;
            return ExperimentInput{
                *algCompany, cardId, rightAnswers, bestRightSim
            };
        }
        return boost153::none;
    }

friend std::ostream& operator<<(std::ostream&, const ExperimentPreparer&);

private:
    size_t brokenCount_ = 0;
    size_t lowSimilarityCount_ = 0;
    size_t experimentCount_ = 0;
    std::map<std::string, std::vector<model::ID>> partnerToObject_;
    boost153::python::object reader_;
};

std::ostream&
operator<<(std::ostream& out, const ExperimentPreparer& p)
{
    out << "=============================" << std::endl;
    out << "broken inputs number: " << p.brokenCount_ << std::endl;
    out << "low similarity number: " << p.lowSimilarityCount_ << std::endl;
    out << "prepare " << p.experimentCount_ << " experiments" << std::endl;
    out << "=============================" << std::endl;
    return out;
}


template<class T>
double calculateAverage(const std::vector<T>& values)
{
    double sum = 0.;
    if (values.empty()) {
        return sum;
    }
    for (auto value: values) {
        sum += value;
    }
    return sum / values.size();
}

template<class T>
double calculateMedian(const std::vector<T>& values)
{
    if (values.empty()) {
        return 0;
    }
    std::vector<uint64_t> sortedValues(values.begin(), values.end());
    std::sort(sortedValues.begin(), sortedValues.end());
    if (sortedValues.size() % 2) {
        return sortedValues[sortedValues.size() / 2];
    } else {
        return (sortedValues[sortedValues.size() / 2 - 1] +
            sortedValues[sortedValues.size() / 2]) / 2.;
    }
}

struct ExperimentWorkflowInfo
{
    bool dbCorruption = false;
    bool noResult = false;
    bool match = false;
    bool nonMatch = false;
    bool rightSimBetter = false;
    uint64_t timeMs = 0;
    uint64_t searchTimeMs = 0;
    uint64_t loadCompaniesTimeMs = 0;
    uint64_t loadFeaturesTimeMs = 0;
    size_t candidatesNumber = 0;
};

class Statistics
{
public:
    void appendExperiment(const ExperimentWorkflowInfo& workflowInfo) {
        ++experimentCount_;
        if (workflowInfo.dbCorruption) {
            ++dbCorruptionCount_;
            return;
        }
        if (workflowInfo.noResult) {
            ++noResultCount_;
        }
        if (workflowInfo.match) {
            ++matchCount_;
        }
        if (workflowInfo.nonMatch) {
            ++nonMatchCount_;
        }
        if (workflowInfo.rightSimBetter) {
            ++rightSimBetterCount_;
        }
        timesMs_.push_back(workflowInfo.timeMs);
        searchTimesMs_.push_back(workflowInfo.searchTimeMs);
        loadCompaniesTimesMs_.push_back(workflowInfo.loadCompaniesTimeMs);
        loadFeaturesTimesMs_.push_back(workflowInfo.loadFeaturesTimeMs);
        candidatesNumbers_.push_back(workflowInfo.candidatesNumber);
    }

friend std::ostream& operator<<(std::ostream&, const Statistics&);

private:
    mutable std::mutex mutex_;
    std::vector<uint64_t> timesMs_;
    std::vector<uint64_t> searchTimesMs_;
    std::vector<uint64_t> loadCompaniesTimesMs_;
    std::vector<uint64_t> loadFeaturesTimesMs_;
    std::vector<size_t> candidatesNumbers_;
    size_t experimentCount_ = 0;
    size_t dbCorruptionCount_ = 0;
    size_t noResultCount_ = 0;
    size_t matchCount_ = 0;
    size_t nonMatchCount_ = 0;
    size_t rightSimBetterCount_ = 0;
};

std::ostream& operator<<(std::ostream& out, const Statistics& s)
{
    out << "=============================" << std::endl;
    out << "total experiments: " << s.experimentCount_ << std::endl;
    out << "total db corruption: " << s.dbCorruptionCount_ << std::endl;
    out << "total no results: " << s.noResultCount_ << std::endl;
    out << "total match: " << s.matchCount_ << std::endl;
    out << "total non match: " << s.nonMatchCount_ << std::endl;
    out << "total right sim better: " << s.rightSimBetterCount_ << std::endl;
    out << "average time: " << calculateAverage(s.timesMs_) << "ms" << std::endl;
    out << "average search time: "
        << calculateAverage(s.searchTimesMs_) << "ms" << std::endl;
    out << "average load companies time: "
        << calculateAverage(s.loadCompaniesTimesMs_) << "ms" << std::endl;
    out << "average load features time: "
        << calculateAverage(s.loadFeaturesTimesMs_) << "ms" << std::endl;
    out << "median time: " << calculateMedian(s.timesMs_) << "ms" << std::endl;
    out << "average candidates number: "
        << calculateAverage(s.candidatesNumbers_) << std::endl;
    out << "median candidates number: "
        << calculateMedian(s.candidatesNumbers_) << std::endl;
    out << "=============================" << std::endl;
    return out;
}

void
doExperiment(
    const ExperimentInput& input,
    model::Session& session,
    ExperimentWorkflowInfo& workflowInfo,
    std::ostream& log)
{
    log << "partner id: " << input.partnerCardId << std::endl;
    sprav::matching::SimilarCompanies similarCompanies;
    try {
        sprav::Stopwatch timer(sprav::Stopwatch::Running);
        double recall = 0.95;
        auto debugResult = maps::sprav::matching::findSimilarCompaniesDebug(
            input.algCompany,
            session,
            recall,
            5);
        similarCompanies = debugResult.result;
        for (auto kv: debugResult.workflowInfo) {
            log << kv.first << ": " << kv.second << std::endl;
        }
        workflowInfo.timeMs = timer.elapsed<std::chrono::milliseconds>();
        for (std::string prefix: {"exact", "distance exact", "long distance", "fuzzy"}) {
            auto& debug = debugResult.workflowInfo;

            auto searchTimeKey = prefix + " search time ms";
            if (debug.find(searchTimeKey) != debug.end()) {
                workflowInfo.searchTimeMs += boost153::lexical_cast<uint64_t>(
                    debug[searchTimeKey]
                );
            }

            auto loadCompaniesTimeKey = prefix + " load companies time ms";
            if (debug.find(loadCompaniesTimeKey) != debug.end()) {
                workflowInfo.loadCompaniesTimeMs += boost153::lexical_cast<uint64_t>(
                    debug[loadCompaniesTimeKey]
                );
            }

            auto loadFeaturesTimeKey = prefix + " load features time ms";
            if (debug.find(loadFeaturesTimeKey) != debug.end()) {
                workflowInfo.loadFeaturesTimeMs += boost153::lexical_cast<uint64_t>(
                    debug[loadFeaturesTimeKey]
                );
            }

        }

        workflowInfo.candidatesNumber = similarCompanies.size();
        log << "candidates number = " << similarCompanies.size() << std::endl;
        log << "right answer similarity = " << input.bestRightSimilarity << std::endl;
        log << "time = " << workflowInfo.timeMs << "ms" << std::endl;
    } catch (model::ObjectsBroken&) {
        workflowInfo.dbCorruption = true;
        log << "corruption" << std::endl;
        return;
    } catch (model::ObjectNotFound&) {
        workflowInfo.dbCorruption = true;
        log << "corruption" << std::endl;
        return;
    }
    if (similarCompanies.size() == 0) {
        workflowInfo.noResult = true;
        log << "no result; right answer ids: ";
        for (const auto& ra: input.rightAnswers) {
            log << ra.id() << " ";
        }
        log << std::endl;
        return;
    }

    bool match = false;
    for (auto ra: input.rightAnswers) {
        for (auto& sc: similarCompanies) {
            if (sc.company.id() == ra.id()) {
                match = true;
            }
        }
    }
    if (match) {
        workflowInfo.match = true;
    } else {
        workflowInfo.nonMatch = true;
        log << "non match" << std::endl;
        log << "candidates (size=" << similarCompanies.size() << "):" << std::endl;
        for (const auto& sc: similarCompanies) {
           log << "id=" << sc.company.id() << " sim=" << sc.similarity << std::endl;
        }
        log << "right answers:" << std::endl;
        for (auto ra: input.rightAnswers) {
            auto sim = calcSimilarity(ra, input.algCompany, session);
            log << "id=" << ra.id() << " sim=" << sim << std::endl;
        }
        log << std::endl;
    }

    if (similarCompanies[0].similarity < input.bestRightSimilarity) {
        workflowInfo.rightSimBetter = true;
    }
}

int main(int argc, char* argv[])
{
    maps::log::setLevel("DEBUG");

    namespace bpo = boost153::program_options;

    bpo::options_description desc("Usage");
    desc.add_options()
        ("help,h", "Print this help message and exit")
        ("number,n", bpo::value<size_t>(), "Number of experiments")
        ("offset", bpo::value<size_t>(), "Number of cards to skip")
        ("miningson,m", bpo::value<std::string>(), "input filename")
        ("ids,i", bpo::value<std::string>(), "partner id to object id filename")
        ("threads-number", bpo::value<size_t>(), "threads number")
    ;

    bpo::variables_map vars;
    bpo::store(
        bpo::parse_command_line(argc, argv, desc),
        vars
    );
    bpo::notify(vars);

    if (vars.count("help") > 0 ||
        vars.count("miningson") == 0 ||
        vars.count("ids") == 0)
    {
        std::cout << desc << std::endl;
        return 0;
    }

    std::string miningsonFilename(vars["miningson"].as<std::string>());
    std::string idsFilename(vars["ids"].as<std::string>());
    auto maxExperimentsNumber = boost153::make_optional<size_t>(false, 0);
    if (vars.count("number")) {
        maxExperimentsNumber = vars["number"].as<size_t>();
    }
    auto offsetNumber = boost153::make_optional<size_t>(false, 0);
    if (vars.count("offset")) {
        offsetNumber = vars["offset"].as<size_t>();
    }
    size_t threadNumber = 1;
    if (vars.count("threads-number")) {
        threadNumber = vars["threads-number"].as<size_t>();
    }

    auto connectionManager =
        sprav::config::createConnectionManager(
            sprav::config::config(),
            sprav::config::ConnectionPoolProfile::Default);
    ReadSessionHolder holder(*connectionManager);

    ExperimentPreparer experimentPreparer(miningsonFilename, idsFilename, offsetNumber);

    Statistics statistics;

    maps::threadpool::ThreadPool threadPool{
        maps::threadpool::Params().setThreadsNumber(threadNumber)
    };

    std::mutex experimentMutex;

    auto workerFunction = [&](const ExperimentInput& input) {
        try {
            ReadSessionHolder holder(*connectionManager);
            std::stringstream out;
            ExperimentWorkflowInfo workflowInfo;
            doExperiment(input, holder.session, workflowInfo, out);
            std::lock_guard<std::mutex> lock(experimentMutex);
            statistics.appendExperiment(workflowInfo);
            std::cout << out.str() << std::endl;
        } catch (...) {
            std::lock_guard<std::mutex> lock(experimentMutex);
            std::cout << "exception on card id = " << input.partnerCardId
                << std::endl << std::endl;
        }
    };

    std::vector<std::future<void>> results;
    size_t experimentsNumber = 0;
    while (true) {
        auto input = experimentPreparer.next(holder.session);
        if (!input) {
            break;
        }
        if (maxExperimentsNumber && experimentsNumber >= *maxExperimentsNumber) {
            break;
        }
        results.push_back(threadPool.async(workerFunction, *input));
        ++experimentsNumber;
    }
    for (auto& result: results) {
        result.get();
    }
    threadPool.join();
    std::cout << experimentPreparer << std::endl;
    std::cout << statistics << std::endl;

    return 0;
}
