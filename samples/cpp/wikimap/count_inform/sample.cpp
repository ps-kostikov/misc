#include "releases_notification.h"

#include <yandex/maps/wiki/acl/aclgateway.h>

#include <yandex/maps/pgpool3/pgpool3.h>
#include <yandex/maps/pgpool3/adapter_helpers.h>


#include <iostream>
#include <string>

namespace mpgp = maps::pgpool3;

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

std::set<maps::wiki::revision::UserID> excludedUids(mpgp::Pool& pool)
{
    auto connection = pool.getMasterConnection();
    auto transaction = mpgp::makeReadOnlyTransaction(std::move(connection));
    maps::wiki::acl::ACLGateway aclGw(*transaction);

    std::vector<std::string> groupNames{
        "mpro",
        "cartographers-group",
        "ya-support",
        "yandex-moderators"
    };
    std::set<maps::wiki::revision::UserID> result;
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


int main(int argc, const char** argv)
{

    std::cout << "hello" << std::endl;
    std::cout << "argc = " << argc << std::endl;
    if (argc < 4) {
        std::cout << "expected cmd line: ./<exe> <connection string> <since branch id> <till branch id>" << std::endl;
        return 1;
    }

    std::string connStr(argv[1]);
    auto sinceBranchId = boost::lexical_cast<maps::wiki::revision::DBID>(argv[2]);
    auto tillBranchId = boost::lexical_cast<maps::wiki::revision::DBID>(argv[3]);

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
    auto userMap = maps::wiki::releases_notification::getVecReleaseUsers_original(*pool, sinceBranchId, tillBranchId);

    auto exclUids = excludedUids(*pool);

    std::map<maps::wiki::revision::UserID, maps::wiki::releases_notification::VecUserData> finalMap;

    for (const auto& p: userMap) {
        if (exclUids.count(p.first) == 0) {
            finalMap[p.first] = p.second;
        }
    }

    std::cout << "size = " << finalMap.size() << std::endl;

    for (const auto& p: finalMap) {
        std::cout << "uid = " << p.first << " commits count = " << p.second.getCommitsCount() << std::endl;
    }



    return 0;
}
