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
#include <yandex/maps/geolib3/distance.h>

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

std::vector<std::vector<size_t>>
countIntCombinations(size_t n, size_t k)
{

    std::vector<bool> v(n);
    std::fill(v.begin() + n - k, v.end(), true);

    std::vector<std::vector<size_t>> result;

    do {
        std::vector<size_t> combination;
        for (size_t i = 0; i < n; ++i) {
            if (v[i]) {
                combination.push_back(i);
            }
        }
        result.push_back(combination);
    } while (std::next_permutation(v.begin(), v.end()));
    return result;
}

std::vector<std::vector<std::string>>
countCombinations(std::vector<std::string> v, size_t k)
{
    auto intCombinations = countIntCombinations(v.size(), k);
    std::vector<std::vector<std::string>> result;
    for (const auto& intCombination: intCombinations) {
        std::vector<std::string> strCombination;
        for (auto pos: intCombination) {
            strCombination.push_back(v[pos]);
        }
        result.push_back(strCombination);
    }
    return result;
}

std::vector<model::ID>
unionVectors(const std::vector<model::ID>& v1, const std::vector<model::ID>& v2)
{
    std::set<model::ID> ids;
    for (auto id: v1) {
        ids.insert(id);
    }
    for (auto id: v2) {
        ids.insert(id);
    }
    return std::vector<model::ID>(ids.begin(), ids.end());
}

model::search::Filter
andFilters(const std::vector<model::search::Filter>& filters)
{
    auto result = filters.at(0);
    for (size_t i = 1; i < filters.size(); ++i) {
        result &= filters[i];
    }
    return result;
}

model::search::Filter
orFilters(const std::vector<model::search::Filter>& filters)
{
    auto result = filters.at(0);
    for (size_t i = 1; i < filters.size(); ++i) {
        result |= filters[i];
    }
    return result;
}

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
makeTrickyNameFilter(const algorithms::Name& name)
{
    std::string value = name.value().value();
    std::vector<std::string> words;
    boost::split(words, value, boost::is_any_of(" "));
    size_t k;
    if (words.size() % 2) {
        k = (words.size() / 2) + 1;
    } else {
        k = (words.size() / 2);
    }
    std::vector<model::search::Filter> conjunctions;
    for (auto combination: countCombinations(words, k)) {
        std::vector<model::search::Filter> filters;
        for (auto str: combination) {
            filters.push_back(model::search::CompanyName.lexemesMatch(str));
        }
        conjunctions.push_back(andFilters(filters));
    }
    return orFilters(conjunctions);
}

model::search::Filter
makePhoneFilter(const algorithms::Company& company)
{
    std::vector<std::string> phoneStrs;
    for (const auto& phone: company.phones()) {
        phoneStrs.push_back(phone.number());
    }
    return model::search::CompanyPhoneNumber.containsAny(phoneStrs);
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
    auto distanceFilter = model::search::CompanyPos.intersects(company.address().pos(), 10);

    auto mainFilter = phoneFilter || urlFilter;
    for (const auto& name: company.names()) {
        mainFilter |= makeLexemesNameFilter(name);
    }

    return distanceFilter && mainFilter;
}

model::search::Filter
makeL1Filter(const algorithms::Company& company)
{
    auto phoneFilter = makePhoneFilter(company);
    auto urlFilter = makeUrlFilter(company);
    auto distanceFilter = 
        model::search::CompanyPos.intersects(company.address().pos(), 10000) || 
        model::search::CompanyPos.intersects(company.address().boundingBox());

    auto mainFilter = phoneFilter || urlFilter;
    for (const auto& name: company.names()) {
        mainFilter |= makeLexemesNameFilter(name);
    }

    return distanceFilter && mainFilter;
}

model::search::Filter
makeLongDistanceFilter(const algorithms::Company& company)
{
    auto phoneFilter = makePhoneFilter(company);
    auto distanceFilter = model::search::CompanyPos.intersects(company.address().pos(), 50000);

    return distanceFilter && phoneFilter;
}

model::search::Filter
makeFuzzyFilter(const algorithms::Company& company)
{
    auto distanceFilter = model::search::CompanyPos.intersects(company.address().pos(), 10);
    // replace phone filter with FALSE
    auto fuzzyFilter = makePhoneFilter(company);
    for (const auto& name: company.names()) {
        fuzzyFilter |= makeTrickyNameFilter(name);
    }
    return distanceFilter && fuzzyFilter;
}

double countSimilarity(model::ID id1, model::ID id2, model::Session& session)
try
{
    auto c1 = model::convertCompany(session.loadCompany(id1), session);
    auto c2 = model::convertCompany(session.loadCompany(id2), session);
    return algorithms::calculateSimilarity(c1, c2);
} catch (...) {
    return 0.;
}

double countDistance(model::ID id1, model::ID id2, model::Session& session)
try
{
    auto c1 = session.loadCompany(id1);
    auto c2 = session.loadCompany(id2);
    if (!c1.address() || !c2.address()) {
        return -1.;
    }
    // auto p1 = c1.address()->pos();
    // auto p2 = c2.address()->pos();
    // std::cout << std::endl << p1.x() << " " << p1.y() << " - " << p2.x() << " " << p2.y() << std::endl;
    return maps::geolib3::distance(c1.address()->pos(), c2.address()->pos());
} catch (...) {
    return -2.;
}

bool satisfactoryResults(std::vector<model::ID> ids, model::ID ownId, 
    model::Session& session)
{
    if (ids.size() == 0) {
        return false;
    }

    for (auto id: ids) {
        if (id == ownId) {
            continue;
        }
        auto similarity = countSimilarity(id, ownId, session);
        if (similarity > 0.75) {
            return true;
        }
    }
    return false;
}

struct Candidates
{
    std::vector<model::Company> companies;
    std::string comment;
};

Candidates getCandidates(
    const algorithms::Company& company, model::Session& session, model::ID ownId)
{
    auto almostExactResults = session.loadCompanyIds({
        makeAlmostExactFilter(company)
    });
    std::vector<model::ID> allIds = almostExactResults;
    if (satisfactoryResults(almostExactResults, ownId, session)) {
        // for (auto id: almostExactResults) {
        //     auto similarity = countSimilarity(id, ownId, session);
        //     std::cout << "cand " << id << " similarity " << similarity << std::endl;
        // }
        return Candidates{session.loadCompany(allIds), "exact"};
    }

    auto l1Results = session.loadCompanyIds({
        makeL1Filter(company)
    });
    allIds = unionVectors(allIds, l1Results);
    if (satisfactoryResults(l1Results, ownId, session)) {
        return Candidates{session.loadCompany(allIds), "l1"};
    }

    auto longDistanceResults = session.loadCompanyIds({
        makeLongDistanceFilter(company)
    });
    allIds = unionVectors(allIds, longDistanceResults);
    if (satisfactoryResults(longDistanceResults, ownId, session)) {
        return Candidates{session.loadCompany(allIds), "long"};
    }

    // auto fuzzyResults = session.loadCompanyIds({
    //     makeFuzzyFilter(company)
    // });
    // allIds = unionVectors(allIds, fuzzyResults);
    return Candidates{session.loadCompany(allIds), "fuzzy"};
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
    std::vector<model::Company> originalCompanies;
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
    std::ifstream is("experiment_ids_clusters");
    std::string idStr;
    std::vector<std::vector<model::ID>> idGroups;
    while(std::getline(is, idStr)) {
        std::vector<std::string> strs;
        boost::split(strs, idStr, boost::is_any_of(" "));
        std::vector<model::ID> idGroup;
        for (auto s: strs) {
            idGroup.push_back(boost::lexical_cast<model::ID>(s));
        }
        idGroups.push_back(idGroup);
    }
    std::vector<model::ID> allIds;
    for (const auto& group: idGroups) {
        for (auto id: group) {
            allIds.push_back(id);
        }
    }
    auto allCompanies = session.loadCompany(allIds);
    std::map<model::ID, model::Company> idToCompany;
    for (size_t i = 0; i < allIds.size(); ++i) {
        idToCompany[allIds[i]] = allCompanies[i];
    }

    std::vector<ExperimentInput> result;
    for (const auto& idGroup: idGroups) {
        model::Company company = idToCompany[idGroup[0]];
        std::vector<model::Company> originalCompanies;
        for (size_t i = 1; i < idGroup.size(); ++i) {
            originalCompanies.push_back(idToCompany[idGroup[i]]);
        }
        result.emplace_back(ExperimentInput{company, originalCompanies});
    }
    return result;
}


struct SingleExperimentResult
{
    bool matchInCandidates = false;
    uint64_t getCandidatesTimeMs = 0;
    bool fail = false;
    size_t candidatesNumber = 0;
    std::string comment = "";
};

SingleExperimentResult
doSingleExperiment(const ExperimentInput& input, model::Session& session)
try {
    SingleExperimentResult result;
    auto algDuplicateCompany = model::convertCompany(input.company, session);
    
    std::vector<model::Company> candidates;
    {
        sprav::Stopwatch timer(sprav::Stopwatch::Running);
        auto res = getCandidates(algDuplicateCompany, session, input.company.id());
        candidates = res.companies;
        result.comment = res.comment;
        result.getCandidatesTimeMs = timer.elapsed<std::chrono::milliseconds>();
    }

    std::vector<model::ID> candidatesIds;
    std::transform(candidates.begin(),
                   candidates.end(),
                   std::back_inserter(candidatesIds),
                   std::mem_fn(&model::Company::id));

    for (auto cId: candidatesIds) {
        for (const auto& oComp: input.originalCompanies) {
            if (oComp.id() == cId) {
                result.matchInCandidates = true;
            }
        }
    }

    if (!result.matchInCandidates) {
        std::cout << input.company.id();
        for (const auto& oComp: input.originalCompanies) {
            std::cout << " " << oComp.id();
            auto similarity = countSimilarity(input.company.id(), oComp.id(), session);
            auto distance = countDistance(input.company.id(), oComp.id(), session);
            std::cout << " (sim=" << similarity << ", dist=" << distance << ")";
        }
        std::cout << std::endl;
    }

    // for (auto id: candidatesIds) {
    //     auto similarity = countSimilarity(input.company.id(), id, session);
    //     std::cout << "candidate id = " << id << " sim = " << similarity << std::endl;
    // }

    result.candidatesNumber = candidatesIds.size();

    // if (result.matchInCandidates && result.candidatesNumber > 100) {
    //     std::cout << input.company.id() << " " << originalCompany.id() << " "
    //         << result.candidatesNumber << std::endl;
    // }

    // std::cout << "duplicate id = " << input.company.id() << std::endl;
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
    std::cout << "fail " << input.company.id();
    for (const auto& oComp: input.originalCompanies) {
        std::cout << " " << oComp.id();
    }
    std::cout << std::endl;
    return result;
}

void
analyzeInputs(const std::vector<ExperimentInput>& /*inputs*/, model::Session& /*session*/)
{
    // for (const auto& input: inputs) {
    //     std::cout << input.company.id() << std::endl;
    //     for (auto oc: input.originalCompanies) {
    //         auto similarity = countSimilarity(input.company.id(), oc.id(), session);
    //         std::cout << oc.id() << "  sim = " << similarity << std::endl;
    //     }
    //     std::cout << std::endl;
    // }
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

    std::map<std::string, int> commentsMap;
    for (const auto& result: results) {
        if (commentsMap.find(result.comment) == commentsMap.end()) {
            commentsMap[result.comment] = 1;
        } else {
            commentsMap[result.comment] = commentsMap[result.comment] + 1;
        }
    }
    if (matches) {
        for (auto it: commentsMap) {
            std::cout << "'" << it.first << "' - " << it.second 
                << " (" << it.second * 100. / (double)matches << "%)" << std::endl;
        }
    }

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

    auto experimentInputs = getExperimentalCompanies(holder.session);
    analyzeInputs(experimentInputs, holder.session);
    // return 0;
    std::vector<SingleExperimentResult> results;

    for (auto eInput: experimentInputs) {
        auto singleResult = doSingleExperiment(eInput, holder.session);
        results.push_back(singleResult);
    }

    analyzeResults(results);
    return 0;
}
