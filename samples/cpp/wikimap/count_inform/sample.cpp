#include "releases_notification.h"

#include <yandex/maps/wiki/acl/aclgateway.h>

#include <yandex/maps/pgpool3/pgpool3.h>
#include <yandex/maps/pgpool3/adapter_helpers.h>


#include <iostream>
#include <string>

namespace mpgp = maps::pgpool3;
namespace mw = maps::wiki;

class PgpoolLogger: public mpgp::logging::Logger
{
public:
    PgpoolLogger()
    { }

    virtual void put(mpgp::logging::Message&& /*msg*/) override
    {
        // std::cout << msg.source << " " << msg.message << std::endl;
    }

    virtual mpgp::logging::Level level() const override
    {
        return mpgp::logging::Level::Debug;
    }

    virtual std::string name() const override
    {
        return "pgppool3";
    }
};

std::set<mw::revision::UserID> excludedUids(mpgp::Pool& pool)
{
    auto connection = pool.getMasterConnection();
    auto transaction = mpgp::makeReadOnlyTransaction(std::move(connection));
    mw::acl::ACLGateway aclGw(*transaction);

    std::vector<std::string> groupNames{
        "mpro",
        "cartographers-group",
        "ya-support",
        "yandex-moderators"
    };
    std::set<mw::revision::UserID> result;
    for (const auto& groupName: groupNames) {
        try {
            auto group = aclGw.group(groupName);
            for (const auto& user: group.users()) {
                result.insert(user.uid());
            }
        } catch (...) {
            ;
        }
    }
    return result;
}

void printStats(
    const std::map<mw::revision::UserID, mw::releases_notification::VecUserData>& userMap,
    const std::set<mw::revision::UserID>& exclUids)
{
    std::map<mw::revision::UserID, mw::releases_notification::VecUserData> afterExcl;
    for (const auto& p: userMap) {
        if (exclUids.count(p.first) == 0) {
            afterExcl[p.first] = p.second;
        }
    }
    std::cout << "total users = " << afterExcl.size() << std::endl;
    int commitCount = 0;
    for (const auto& p: afterExcl) {
        commitCount += p.second.getCommitsCount();
    }
    std::cout << "total commits = " << commitCount << std::endl;
}

int main(int argc, const char** argv)
{

    std::cout << "hello" << std::endl;
    std::cout << "argc = " << argc << std::endl;
    if (argc < 4) {
        std::cout << "expected cmd line: ./<exe> <connection string> <since branch id> <till branch id>" << std::endl;
        return 1;
    }

    std::string connStr(argv[1]);
    auto sinceBranchId = boost::lexical_cast<mw::revision::DBID>(argv[2]);
    auto tillBranchId = boost::lexical_cast<mw::revision::DBID>(argv[3]);

    std::cout << "connection string = '" << connStr << "'" << std::endl;
    std::cout << "since branch id = " << sinceBranchId << std::endl;
    std::cout << "till branch id = " << tillBranchId << std::endl;

    mpgp::PoolConfigurationPtr poolConfiguration(mpgp::PoolConfiguration::create());
    std::string authParams;
    std::tie(poolConfiguration->master, authParams) = mpgp::helpers::parseConnString(connStr);
    mpgp::PoolConstantsPtr poolConstants(mpgp::PoolConstants::create(1, 1, 0, 0));
    poolConstants->getTimeoutMs = 5000;
    poolConstants->pingIntervalMs = 5000;

    PgpoolLogger logger;
    std::unique_ptr<mpgp::Pool> pool(new mpgp::Pool(
        logger,
        std::move(poolConfiguration),
        authParams,
        std::move(poolConstants)
    ));

    auto exclUids = excludedUids(*pool);

    std::cout << "original:" << std::endl;
    printStats(
        mw::releases_notification::getVecReleaseUsers_original(
            *pool, sinceBranchId, tillBranchId),
        exclUids);
    std::cout << "take all reverts in account:" << std::endl;
    printStats(
        mw::releases_notification::getVecReleaseUsers_takeAllRevertedInAccount(
            *pool, sinceBranchId, tillBranchId),
        exclUids);
    std::cout << "take all deleted in account:" << std::endl;
    printStats(
        mw::releases_notification::getVecReleaseUsers_takeAllDeletedInAccount(
            *pool, sinceBranchId, tillBranchId),
        exclUids);
    std::cout << "only snapshot commits:" << std::endl;
    printStats(
        mw::releases_notification::getVecReleaseUsers_onlyLastCommits(
            *pool, sinceBranchId, tillBranchId),
        exclUids);
    return 0;
}
