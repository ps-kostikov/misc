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
#include <exception>


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

std::set<std::string>
fetchPrevIds(const std::string& filename)
{
    std::ifstream is(filename.c_str());
    std::string line;
    std::set<std::string> result;
    while(std::getline(is, line)) {
        std::vector<std::string> strs;
        boost153::split(strs, line, boost153::is_any_of(":"));
        result.insert(strs[0]);
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


class ExperimentPreparer
{
public:
    ExperimentPreparer(const std::string& minigsonFilename,
                       const std::string& answersFilename,
                       boost153::optional<std::string> prevLogFilename,
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

        if (prevLogFilename) {
            knownCardIds_ = fetchPrevIds(*prevLogFilename);
        }

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
            
            if (knownCardIds_.find(cardId) != knownCardIds_.end()) {
                continue;
            }

            std::unique_ptr<algorithms::Company> algCompany;
            try {
                algCompany.reset(
                    new algorithms::Company(maps::mining::sprav2miningson::toSpravCompany(card))
                );
            } catch (...) {
                ++brokenCount_;
                std::cout << cardId << ": broken (can not convert)" << std::endl;
                continue;
            }

            auto rightAnswersIds = partnerToObject_[cardId];
            if (rightAnswersIds.size() == 0) {
                ++brokenCount_;
                std::cout << cardId << ": broken (no result)" << std::endl;
                continue;
            }
            if (rightAnswersIds.size() > 3) {
                ++brokenCount_;
                std::cout << cardId << ": broken (many results)" << std::endl;
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
                std::cout << cardId << ": low sim" << std::endl;
                continue;
            }

            std::cout << cardId << ": OK" << std::endl;
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
    std::set<std::string> knownCardIds_;
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
        ("log,l", bpo::value<std::string>(), "previous log")
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
    auto logFilename = boost153::make_optional<std::string>(false, "");
    if (vars.count("log")) {
        logFilename = vars["log"].as<std::string>();
    }
    auto maxExperimentsNumber = boost153::make_optional<size_t>(false, 0);
    if (vars.count("number")) {
        maxExperimentsNumber = vars["number"].as<size_t>();
    }
    auto offsetNumber = boost153::make_optional<size_t>(false, 0);
    if (vars.count("offset")) {
        offsetNumber = vars["offset"].as<size_t>();
    }

    auto connectionManager =
        sprav::config::createConnectionManager(
            sprav::config::config(),
            sprav::config::ConnectionPoolProfile::Default);
    ReadSessionHolder holder(*connectionManager);

    ExperimentPreparer experimentPreparer(miningsonFilename, idsFilename, logFilename, offsetNumber);

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
        ++experimentsNumber;
        std::cout << experimentsNumber << std::endl;
    }
    std::cout << "experiments number = " << experimentsNumber << std::endl;
    std::cout << experimentPreparer << std::endl;

    return 0;
}
