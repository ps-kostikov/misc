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

std::map<std::pair<std::string, std::string>, std::vector<model::ID>>
fetchIdsMap(const std::string& filename)
{
    std::ifstream is(filename.c_str());
    std::string idStr;
    std::map<std::pair<std::string, std::string>, std::vector<model::ID>> result;
    while(std::getline(is, idStr)) {
        std::vector<std::string> strs;
        boost153::split(strs, idStr, boost153::is_any_of(" "));
        std::string partnerId = strs[0];
        std::string partnerCardId = strs[1];
        auto objectId = boost153::lexical_cast<model::ID>(strs[2]);
        result[std::make_pair(partnerId, partnerCardId)].push_back(objectId);
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
        ("threshold,t", bpo::value<double>(), "similarity threshold")
        ("output,o", bpo::value<std::string>(), "output filename")
    ;

    bpo::variables_map vars;
    bpo::store(
        bpo::parse_command_line(argc, argv, desc),
        vars
    );
    bpo::notify(vars);

    if (vars.count("help") > 0 ||
        vars.count("miningson") == 0 ||
        vars.count("output") == 0 ||
        vars.count("ids") == 0)
    {
        std::cout << desc << std::endl;
        return 0;
    }

    std::string miningsonFilename(vars["miningson"].as<std::string>());
    std::string outputFilename(vars["output"].as<std::string>());
    std::string idsFilename(vars["ids"].as<std::string>());
    auto maxExperimentsNumber = boost153::make_optional<size_t>(false, 0);
    if (vars.count("number")) {
        maxExperimentsNumber = vars["number"].as<size_t>();
    }
    auto offsetNumber = boost153::make_optional<size_t>(false, 0);
    if (vars.count("offset")) {
        offsetNumber = vars["offset"].as<size_t>();
    }
    auto similarityThreshold = boost153::make_optional<double>(false, 0);
    if (vars.count("threshold")) {
        similarityThreshold = vars["threshold"].as<double>();
    }

    auto connectionManager =
        sprav::config::createConnectionManager(
            sprav::config::config(),
            sprav::config::ConnectionPoolProfile::Default);
    ReadSessionHolder holder(*connectionManager);

    Py_Initialize();
    boost153::python::register_exception_translator<maps::mining::sprav2miningson::ConversionFailedError>(
        [](const maps::mining::sprav2miningson::ConversionFailedError& exc) {
            const std::string message =
                exc.attrs_.count("pythonException") ? exc.attrs_.at("pythonException") : exc.what();
            PyErr_SetString(PyExc_ValueError, message.c_str());
        });

    auto partnerToObject = fetchIdsMap(idsFilename);

    auto package = boost153::python::import("yandex.maps.mining.miningson");
    auto reader = package.attr("card_reader")(miningsonFilename);

    if (offsetNumber) {
        for (size_t i = 0; i < *offsetNumber; ++i) {
            try {
                reader.attr("next")();
            } catch (...) {
                break;
            }
        }
    }

    size_t brokenCount = 0;
    size_t lowSimilarityCount = 0;
    size_t preparedCount = 0;

    auto writer = package.attr("Writer")(outputFilename);
    writer.attr("__enter__")();

    while (true) {

        boost153::python::object card;
        try {
            card = reader.attr("next")();
        } catch (...) {
            break;
        }

        std::string cardId;
        std::string partnerId;
        try {
            cardId = boost153::python::extract<std::string>(
                card.attr("raw_origin")["partner_card_id"].attr("__str__")());
            partnerId = boost153::python::extract<std::string>(
                card.attr("raw_origin")["partner_id"].attr("__str__")());
        } catch (...) {
            ++brokenCount;
            continue;
        }
        
        std::unique_ptr<algorithms::Company> algCompany;
        try {
            algCompany.reset(
                new algorithms::Company(maps::mining::sprav2miningson::toSpravCompany(card))
            );
        } catch (...) {
            ++brokenCount;
            continue;
        }

        auto rightAnswersIds = partnerToObject[std::make_pair(partnerId, cardId)];
        if (rightAnswersIds.size() == 0) {
            ++brokenCount;
            continue;
        }
        if (rightAnswersIds.size() > 3) {
            ++brokenCount;
            continue;
        }
        auto rightAnswers = holder.session.loadCompany(rightAnswersIds);


        if (similarityThreshold) {
            double bestRightSim = 0.;
            for (auto ra: rightAnswers) {
                auto sim = calcSimilarity(ra, *algCompany, holder.session);
                bestRightSim = std::max(bestRightSim, sim);
            }
            if (bestRightSim < *similarityThreshold) {
                ++lowSimilarityCount;
                continue;
            }
        }

        auto cleanCard = maps::mining::sprav2miningson::toMiningsonCard(*algCompany);
        boost153::python::list rightAnswersIdsList;
        for (auto ra: rightAnswers) {
            rightAnswersIdsList.append(ra.id());
        }
        cleanCard.attr("debug")["right_ids"] = rightAnswersIdsList;

        std::string ammoId = "card_id: " + cardId + " partner_id: " + partnerId;
        cleanCard.attr("debug")["ammo_id"] = ammoId;

        writer.attr("write")(cleanCard);
        if (maxExperimentsNumber && preparedCount >= *maxExperimentsNumber) {
            break;
        }
        std::cout << cardId << std::endl;
        ++preparedCount;
    }
    writer.attr("__exit__")(0, 0, 0);

    std::cout << "broken inputs number: " << brokenCount << std::endl;
    std::cout << "low similarity number: " << lowSimilarityCount << std::endl;
    std::cout << "prepared cards number: " << preparedCount << std::endl;

    return 0;
}
