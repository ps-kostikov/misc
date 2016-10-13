#include <yandex/maps/wiki/acl/aclgateway.h>

#include <yandex/maps/pgpool3/pgpool3.h>
#include <yandex/maps/pgpool3/adapter_helpers.h>


#include <iostream>
#include <string>
#include <set>

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

std::set<mw::acl::UID> uidsFromRoleName(mw::acl::ACLGateway& aclGw, const std::string& roleName)
{
    auto role = aclGw.role(roleName);
    auto pagedUserVector = aclGw.users(0, role.id(), 0, boost::none, 0, 0);
    std::set<mw::acl::UID> result;
    for (const auto& user: pagedUserVector.value()) {
        result.insert(user.uid());
    }
    return result;
}

void doSmth(mpgp::Pool& pool)
{
    auto connection = pool.getMasterConnection();
    auto transaction = mpgp::makeReadOnlyTransaction(std::move(connection));
    mw::acl::ACLGateway aclGw(*transaction);

    auto fetchResult = [&](mw::acl::UID uid, const std::string& roleName) {
        if (roleName == "cartographer") {
            auto cartographerUids = uidsFromRoleName(aclGw, "cartographer");
            return cartographerUids.count(uid) > 0;
        } else if (roleName == "yandex-moderator") {
            auto cartographerUids = uidsFromRoleName(aclGw, "cartographer");
            auto yaModeratorUids = uidsFromRoleName(aclGw, "yandex-moderator");
            return yaModeratorUids.count(uid) > 0 and cartographerUids.count(uid) == 0;
        } else if (roleName == "moderator") {
            auto cartographerUids = uidsFromRoleName(aclGw, "cartographer");
            auto yaModeratorUids = uidsFromRoleName(aclGw, "yandex-moderator");
            auto moderatorUids = uidsFromRoleName(aclGw, "moderator");
            return moderatorUids.count(uid) > 0 and yaModeratorUids.count(uid) == 0 and cartographerUids.count(uid) == 0;
        }
        auto uids = uidsFromRoleName(aclGw, roleName);
        return uids.count(uid) > 0;
    };

    for (mw::acl::UID uid: {336031435, 194620901, 278981621}) {
        for (std::string roleName: {"moderator", "yandex-moderator", "cartographer"}) {
            bool result = fetchResult(uid, roleName);
            std::cout << uid << " " << roleName << ": " << result << std::endl;
        }
    }

}


int main(int argc, const char** argv)
{

    std::cout << "hello" << std::endl;
    std::cout << "argc = " << argc << std::endl;
    if (argc < 2) {
        std::cout << "expected cmd line: ./<exe> <connection string>" << std::endl;
        return 1;
    }

    std::string connStr(argv[1]);

    std::cout << "connection string = '" << connStr << "'" << std::endl;

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

    doSmth(*pool);

    return 0;
}
