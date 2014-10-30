#include <yandex/maps/sprav/model/storages/revision.h>
#include <yandex/maps/sprav/model/to_algorithms.h>
#include <yandex/maps/sprav/common/config.h>
#include <yandex/maps/sprav/common/stopwatch.h>
#include <yandex/maps/sprav/algorithms/datamodel/company.h>
#include <yandex/maps/sprav/algorithms/similarity/similarity.h>
#include <yandex/maps/sprav/algorithms/unification/email_unifier.h>
#include <yandex/maps/sprav/algorithms/unification/url_unifier.h>
#include <yandex/maps/sprav/algorithms/unification/address_unifier.h>

#include <yandex/maps/pgpool2/pgpool.h>
#include <yandex/maps/common/exception.h>

#include <boost/lexical_cast.hpp>

#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>


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

// 30% - 70%
std::vector<model::Company> getCandidates(const algorithms::Company& company, model::Session& session)
{
    std::vector<model::search::Filter> filters;

    filters.push_back(model::search::CompanyPos.intersects(company.address().pos(), 10000));
    filters.push_back(model::search::CompanyAddress.match(company.address().formatted().value()));
    std::cout << company.address().formatted().value() << std::endl;

    std::vector<model::ID> objectIds = session.loadCompanyIds(filters);
    std::vector<model::Company> result;
    for (auto id : objectIds) {
        auto company = session.loadCompany(id);
        result.push_back(company);
    }
    return result;
    
}

// около 40%
// std::vector<model::Company> getCandidates(const algorithms::Company& company, model::Session& session)
// {
//     std::vector<model::search::Filter> filters;

//     filters.push_back(model::search::CompanyPos.intersects(company.address().pos(), 10000));

//     for (auto& name: company.names()) {
//         if (name.type() == algorithms::Name::Type::Official && name.value().locale().language().lang == maps::LangCode::LANG_RU) {
//             filters.push_back(model::search::CompanyName.match(name.value().value()));
//             std::cout << "find" << std::endl;
//             break;
//         }
//     }

//     std::vector<model::ID> objectIds = session.loadCompanyIds(filters);
//     std::vector<model::Company> result;
//     for (auto id : objectIds) {
//         auto company = session.loadCompany(id);
//         result.push_back(company);
//     }
//     return result;
// }


// std::vector<model::Company> getExperimentalCompanies(model::Session& session)
// {
//     std::vector<model::search::Filter> filters;

//     filters.push_back(model::search::CompanyPublishingStatus.equals(model::Company::PublishingStatus::Duplicate));

//     std::vector<model::ID> objectIds = session.loadCompanyIds(filters, 0, 100);

//     std::vector<model::Company> result;
//     for (auto id : objectIds) {
//         auto company = session.loadCompany(id);
//         result.push_back(company);
//     }
//     return result;
// }

std::vector<model::Company> getExperimentalCompanies(model::Session& session)
{
    std::ifstream is("experiment_ids");
    std::string idStr;
    std::vector<model::ID> ids;
    while(std::getline(is, idStr)) {
        ids.push_back(boost::lexical_cast<model::ID>(idStr));
    }
    return session.loadCompany(ids);
}

struct SingleExperimentResult
{
    // SingleExperimentResult():
    //     matchInCandidates(false),
    //     getCandidatesTimeMs(0),
    //     fail(false)
    bool matchInCandidates = false;
    uint64_t getCandidatesTimeMs = 0;
    bool fail = false;
};

SingleExperimentResult
doSingleExperiment(model::Company& duplicateCompany, model::Company& originalCompany, model::Session& session)
{
    SingleExperimentResult result;
    auto algDuplicateCompany = model::to_algorithms::convert(duplicateCompany);
    
    std::vector<model::Company> candidates;
    try {
        sprav::Stopwatch timer(sprav::Stopwatch::Running);
        candidates = getCandidates(algDuplicateCompany, session);
        result.getCandidatesTimeMs = timer.elapsed<std::chrono::milliseconds>();
    } catch (...) {
        result.fail = true;
        return result;
    }

    std::vector<model::ID> candidatesIds;
    std::transform(candidates.begin(),
                   candidates.end(),
                   std::back_inserter(candidatesIds),
                   std::mem_fn(&model::Company::id));

    result.matchInCandidates = std::find(
        candidatesIds.begin(), candidatesIds.end(), originalCompany.id()) != candidatesIds.end();

    if (!result.matchInCandidates) {
        std::cout << duplicateCompany.id() << " " << originalCompany.id() << std::endl;
    }

    // std::cout << "duplicate id = " << duplicateCompany.id() << std::endl;
    // std::cout << "original id = " << originalCompany.id() << std::endl;

    // for (const auto& candidate: candidates) {

    //     auto algCand = model::to_algorithms::convert(candidate);
    //     auto sim = algorithms::calculateSimilarity(algDuplicateCompany, algCand);
    //     if (sim < 0.8) {
    //         continue;
    //     }
    //     std::cout << "candidate id = " << candidate.id() << " ";
    //     std::cout << "sim = " << sim << " ";
    //     if (candidate.id() == originalCompany.id()) {
    //         std::cout << "match!";
    //     }
    //     std::cout << std::endl;
    // }

    return result;
}

void
analyzeResults(const std::vector<SingleExperimentResult>& results)
{
    std::cout << "total experiments = " << results.size() << std::endl;
    auto matches = std::count_if(results.begin(), results.end(), [](const SingleExperimentResult& r) {
        return r.matchInCandidates;
    });
    std::cout << "matches " << matches << "(" << matches * 100. / (double)results.size() << "%)" << std::endl;

    auto fails = std::count_if(results.begin(), results.end(), [](const SingleExperimentResult& r) {
        return r.fail;
    });
    std::cout << "fails " << fails << "(" << fails * 100. / (double)results.size() << "%)" << std::endl;


    double averageGetCandidatesTime = 0.;
    std::for_each(results.begin(),results.end(),[&](const SingleExperimentResult& r){
        averageGetCandidatesTime += r.getCandidatesTimeMs;
    });
    averageGetCandidatesTime /= results.size();

    std::cout << "average getCandidates time = " << averageGetCandidatesTime << "ms" << std::endl;
}

int main()
{
    std::cout << "hello" << std::endl;

    auto connectionManager =
        sprav::config::createConnectionManager(
            sprav::config::config(),
            sprav::config::ConnectionPoolProfile::Editor);
    ReadSessionHolder holder(*connectionManager);

    auto companies = getExperimentalCompanies(holder.session);
    std::vector<SingleExperimentResult> results;

    for (auto company : companies) {
        if (!company.duplicateCompanyId()) {
            std::cout << "Err, company id = " << company.id() << std::endl;
            continue;
        }
        auto originalCompany = holder.session.loadCompany(*company.duplicateCompanyId());

        auto singleResult = doSingleExperiment(company, originalCompany, holder.session);
        results.push_back(singleResult);
    }

    analyzeResults(results);
    return 0;
}