#include "common.h"
#include "ammo.h"

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

typedef unsigned long long OriginalId;
typedef unsigned long long CardId;
typedef model::ID ObjectId;


std::map<CardId, OriginalId>
parseIdToOriginal(const std::string& filename)
{
    std::ifstream is(filename.c_str());
    std::string line;
    std::map<CardId, OriginalId> result;

    while(std::getline(is, line)) {
        std::vector<std::string> parts;
        boost153::split(parts, line, boost153::is_any_of(" "));
        auto id = boost153::lexical_cast<CardId>(parts[0]);
        auto originalId = boost153::lexical_cast<OriginalId>(parts[1]);
        result[id] = originalId;
    }
    return result;
}

std::map<OriginalId, std::vector<ObjectId>>
parseOriginalIds(const std::string& filename)
{
    std::ifstream is(filename.c_str());
    std::string line;
    std::map<OriginalId, std::vector<ObjectId>> result;

    while(std::getline(is, line)) {
        std::vector<std::string> parts;
        // FIXME pkostikov: use only tabs as separator
        boost153::split(parts, line, boost153::is_any_of(" \t"));
        auto originalId = boost153::lexical_cast<OriginalId>(parts[0]);
        auto objectId = boost153::lexical_cast<ObjectId>(parts[1]);
        result[originalId].push_back(objectId);
    }
    return result;

}

std::map<CardId, std::vector<ObjectId>>
makeCardIdToObjectId(
    const std::map<CardId, OriginalId>& cardIdToOriginalId,
    const std::map<OriginalId, std::vector<ObjectId>>& originalIdToObjectId)
{
    std::map<CardId, std::vector<ObjectId>> result;
    for (const auto& kv: cardIdToOriginalId) {
        const auto& cardId = kv.first;
        const auto& originalId = kv.second;

        if (originalIdToObjectId.count(originalId) == 0) {
            std::cout << "can not found original id " << originalId << std::endl;
            continue;
        }
        result[cardId] = originalIdToObjectId.at(originalId);
    }
    return result;
}

int main(int argc, char* argv[])
{
    namespace bpo = boost153::program_options;

    bpo::options_description desc("Usage");
    desc.add_options()
        ("help,h", "Print this help message and exit")
        ("number,n", bpo::value<size_t>(), "Number of experiments")
        ("miningson,m", bpo::value<std::string>(), "input filename")
        ("id-to-original", bpo::value<std::string>(),
            "card id to original space separated value file")
        ("original-ids", bpo::value<std::string>(),
            "original ids space separated value file")
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
        vars.count("id-to-original") == 0 ||
        vars.count("original-ids") == 0 ||
        vars.count("output") == 0)
    {
        std::cout << desc << std::endl;
        return 0;
    }

    std::string miningsonFilename(vars["miningson"].as<std::string>());
    std::string idToOriginalFilename(
        vars["id-to-original"].as<std::string>());
    std::string originalIdsFilename(
        vars["original-ids"].as<std::string>());
    std::string outputFilename(vars["output"].as<std::string>());
    auto maxExperimentsNumber = boost153::make_optional<size_t>(false, 0);
    if (vars.count("number")) {
        maxExperimentsNumber = vars["number"].as<size_t>();
    }

    auto cardIdToOriginalId = parseIdToOriginal(idToOriginalFilename);
    auto originalIdToObjectId = parseOriginalIds(originalIdsFilename);
    auto cardIdToObjectId = makeCardIdToObjectId(
        cardIdToOriginalId, originalIdToObjectId);

    auto connectionManager =
        sprav::config::createConnectionManager(
            sprav::config::config(),
            sprav::config::ConnectionPoolProfile::Default);
    ReadSessionHolder holder(*connectionManager);

    MiningsonReader miningsonReader(miningsonFilename);
    AmmoWriter ammoWriter(outputFilename);

    size_t brokenCount = 0;
    size_t preparedCount = 0;

    while (true) {
        auto card = miningsonReader.next();
        if (!card) {
            break;
        }

        CardId cardId = boost153::python::extract<CardId>(card->attr("id"));

        std::unique_ptr<algorithms::Company> algCompany;
        try {
            algCompany.reset(
                new algorithms::Company(maps::mining::sprav2miningson::toSpravCompany(*card))
            );
        } catch (...) {
            std::cout << "can not convert miningson to alg company " 
                << "card id = " << cardId << std::endl;
            ++brokenCount;
            continue;
        }

        if (cardIdToObjectId.count(cardId) == 0) {
            ++brokenCount;
            continue;
        }

        auto objectIds = cardIdToObjectId[cardId];
        if (objectIds.size() == 0 || objectIds.size() > 3) {
            ++brokenCount;
            continue;
        }

        std::vector<ObjectId> rightIds;
        try {
            auto companies = holder.session.loadCompany(objectIds);
            for (auto& company: companies) {
                if (company.publishingStatus() == model::Company::PublishingStatus::Duplicate) {
                    if (company.duplicateCompanyId()) {
                        rightIds.push_back(*company.duplicateCompanyId());
                    }
                } else {
                    rightIds.push_back(company.id());
                }
            }
        } catch (...) {
            std::cout << "can not load companies from db "
                << "card id = " << cardId
                << " (ids to load: ";
            for (auto id: objectIds) {
                std::cout << id << " ";
            }
            std::cout << ")" << std::endl;
            ++brokenCount;
            continue;
        }

        if (rightIds.empty()) {
            std::cout << "duplicate company has absent duplicateCompanyId"
                << "card id = " << cardId
                << " (ids to load: ";
            for (auto id: objectIds) {
                std::cout << id << " ";
            }
            std::cout << ")" << std::endl;
            ++brokenCount;
            continue;
        }

        std::string ammoId = boost153::lexical_cast<std::string>(cardId);

        Ammo ammo{
            ammoId,
            *algCompany,
            rightIds
        };

        ammoWriter.write(ammo);
        if (maxExperimentsNumber && preparedCount >= *maxExperimentsNumber) {
            break;
        }
        std::cout << "write ammo with id = " << ammoId << std::endl;
        ++preparedCount;
    }

    std::cout << "broken inputs number: " << brokenCount << std::endl;
    std::cout << "prepared cards number: " << preparedCount << std::endl;

    return 0;
}
