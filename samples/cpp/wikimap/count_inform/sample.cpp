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

int main(int argc, const char** argv)
{

    std::cout << "hello" << std::endl;
    std::cout << "argc = " << argc << std::endl;
    if (argc < 2) {
        std::cout << "connection string should be specified" << std::endl;
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
    auto userMap = maps::wiki::releases_notification::getVecReleaseUsers(*pool, 145, 146);
    std::cout << "size = " << userMap.size() << std::endl;



    return 0;
}
