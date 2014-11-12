#include <yandex/maps/sprav/model/storages/revision.h>
#include <yandex/maps/sprav/model/convert.h>
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
#include <boost/algorithm/string.hpp>

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

model::search::Filter
makeLexemesNameFilter(const algorithms::Name& name)
{
    return model::search::CompanyName.lexemesMatch(name.value().value());
}

model::search::Filter
makePrefixNameFilter(const algorithms::Name& name)
{
    return model::search::CompanyName.prefixMatch(name.value().value());
}

model::search::Filter
makePhoneFilter(const algorithms::Company& company)
{
    std::vector<std::string> phoneStrs;
    for (const auto& phone: company.phones()) {
        phoneStrs.push_back(phone.number());
    }
    return model::search::CompanyPhone.containsAny(phoneStrs);
}

model::search::Filter
makeUrlFilter(const algorithms::Company& company)
{
    std::vector<std::string> urlStrs;    
    for (const auto& url: company.urls()) {
        urlStrs.push_back(url.str());
    }
    return model::search::CompanyUrl.containsAny(urlStrs);
}

model::search::Filter
makeAddressFilter(const algorithms::Company& company)
{
    return model::search::CompanyAddress.lexemesMatch(company.address().formatted().value());
}

model::search::Filter
makeAlmostExactFilter(const algorithms::Company& company)
{
    auto phoneFilter = makePhoneFilter(company);
    auto urlFilter = makeUrlFilter(company);
    auto addressFilter = makeAddressFilter(company);
    return addressFilter && (urlFilter || phoneFilter);
}

model::search::Filter
makeL1Filter(const algorithms::Company& company)
{
    auto phoneFilter = makePhoneFilter(company);
    auto urlFilter = makeUrlFilter(company);
    auto distanceFilter = model::search::CompanyPos.intersects(company.address().pos(), 1000);

    auto mainFilter = phoneFilter || urlFilter;
    for (const auto& name: company.names()) {
        mainFilter |= makeLexemesNameFilter(name);
    }

    return mainFilter && distanceFilter;
}

model::search::Filter
makeL2Filter(const algorithms::Company& company)
{
    auto phoneFilter = makePhoneFilter(company);
    auto urlFilter = makeUrlFilter(company);
    auto distanceFilter = model::search::CompanyPos.intersects(company.address().pos(), 10000);

    auto mainFilter = phoneFilter || urlFilter;
    for (const auto& name: company.names()) {
        mainFilter |= makePrefixNameFilter(name);
    }

    return mainFilter && distanceFilter;
}

std::vector<model::Company> getCandidates(const algorithms::Company& company, model::Session& session)
{
    auto almostExactResults = session.loadCompanyIds({
        makeAlmostExactFilter(company)
    });
    if (almostExactResults.size()) {
        return session.loadCompany(almostExactResults);
    }

    // auto l1Results = session.loadCompanyIds({
    //     makeL1Filter(company)
    // });
    // if (l1Results.size()) {
    //     return session.loadCompany(l1Results);
    // }

    auto l2Results = session.loadCompanyIds({
        makeL2Filter(company)
    });
    return session.loadCompany(l2Results);
}

// 30% - 70%
// std::vector<model::Company> getCandidates(const algorithms::Company& company, model::Session& session)
// {
//     std::vector<model::search::Filter> filters;

//     filters.push_back(model::search::CompanyPos.intersects(company.address().pos(), 10000));
//     filters.push_back(model::search::CompanyAddress.match(company.address().formatted().value()));
//     std::cout << company.address().formatted().value() << std::endl;

//     std::vector<model::ID> objectIds = session.loadCompanyIds(filters);
//     std::vector<model::Company> result;
//     for (auto id : objectIds) {
//         auto company = session.loadCompany(id);
//         result.push_back(company);
//     }
//     return result;
    
// }

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

struct ExperimentInput
{
    model::Company company;
    model::Company original;
};

// std::vector<ExperimentInput>
// getExperimentalCompanies(model::Session& session)
// {
//     std::ifstream is("experiment_ids");
//     std::string idStr;
//     std::vector<model::ID> ids;
//     while(std::getline(is, idStr)) {
//         ids.push_back(boost::lexical_cast<model::ID>(idStr));
//     }
//     auto companies = session.loadCompany(ids);
//     std::vector<ExperimentInput> result;
//     for (auto company: companies) {
//         if (!company.duplicateCompanyId()) {
//             std::cout << "Err, company id = " << company.id() << std::endl;
//             continue;
//         }
//         auto originalCompany = session.loadCompany(*company.duplicateCompanyId());
//         result.emplace_back(ExperimentInput{company, originalCompany});
//     }
//     return result;
// }

std::vector<ExperimentInput>
getExperimentalCompanies(model::Session& session)
{
    std::ifstream is("experiment_ids_pairs");
    std::string idStr;
    std::vector<model::ID> ids;
    std::vector<model::ID> originalIds;
    while(std::getline(is, idStr)) {
        std::vector<std::string> strs;
        boost::split(strs, idStr, boost::is_any_of(" "));
        ids.push_back(boost::lexical_cast<model::ID>(strs[0]));
        originalIds.push_back(boost::lexical_cast<model::ID>(strs[1]));
    }
    auto companies = session.loadCompany(ids);
    auto originalCompanies = session.loadCompany(originalIds);
    std::vector<ExperimentInput> result;
    for (size_t i = 0; i < ids.size(); ++i) {
        result.emplace_back(ExperimentInput{companies[i], originalCompanies[i]});
    }
    return result;
}


struct SingleExperimentResult
{
    bool matchInCandidates = false;
    uint64_t getCandidatesTimeMs = 0;
    bool fail = false;
    size_t candidatesNumber = 0;
};

SingleExperimentResult
doSingleExperiment(model::Company& duplicateCompany, model::Company& originalCompany, model::Session& session)
try {
    SingleExperimentResult result;
    auto algDuplicateCompany = model::convertCompany(duplicateCompany, session);
    
    std::vector<model::Company> candidates;
    {
        sprav::Stopwatch timer(sprav::Stopwatch::Running);
        candidates = getCandidates(algDuplicateCompany, session);
        result.getCandidatesTimeMs = timer.elapsed<std::chrono::milliseconds>();
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

    result.candidatesNumber = candidatesIds.size();

    // if (result.matchInCandidates && result.candidatesNumber > 100) {
    //     std::cout << duplicateCompany.id() << " " << originalCompany.id() << " "
    //         << result.candidatesNumber << std::endl;
    // }

    // std::cout << "duplicate id = " << duplicateCompany.id() << std::endl;
    // std::cout << "original id = " << originalCompany.id() << std::endl;

    // for (const auto& candidate: candidates) {

    //     auto algCand = model::convertCompany(candidate, session);
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
} catch (...) {
    SingleExperimentResult result;
    result.fail = true;
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

    double sumCandNumber = 0.;
    size_t minCandNumber = 1000000;
    size_t maxCandNumber = 0;
    for (const auto& result: results) {
        if (!result.fail) {
            sumCandNumber += result.candidatesNumber;
            maxCandNumber = std::max(result.candidatesNumber, maxCandNumber);
            minCandNumber = std::min(result.candidatesNumber, minCandNumber);
        }
    }
    auto correctExperiments = results.size() - fails;
    if (correctExperiments) {
        std::cout << "avg cand num " << sumCandNumber / correctExperiments << std::endl;
        std::cout << "min cand num " << minCandNumber << std::endl;
        std::cout << "max cand num " << maxCandNumber << std::endl;
    }


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

    auto exprimentInputs = getExperimentalCompanies(holder.session);
    std::vector<SingleExperimentResult> results;

    for (auto eInput: exprimentInputs) {
        auto singleResult = doSingleExperiment(eInput.company, eInput.original, holder.session);
        results.push_back(singleResult);
    }

    analyzeResults(results);
    return 0;
}