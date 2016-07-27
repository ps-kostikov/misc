#include <yandex/maps/wiki/acl/aclgateway.h>

#include <yandex/maps/pgpool3/pgpool3.h>


#include <iostream>
#include <string>

namespace mpgp = maps::pgpool3;

class PgpoolLogger: public mpgp::logging::Logger
{
public:
    PgpoolLogger()
    { }

    virtual void put(mpgp::logging::Message&& msg) override
    {
        std::cout << msg.source << " " << msg.message << std::endl;
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

int main()
{

    std::cout << "hello" << std::endl;

    mpgp::PoolConfigurationPtr poolConfiguration(mpgp::PoolConfiguration::create());

    poolConfiguration->master = mpgp::InstanceId{"pg93mpro.maps.dev.yandex.net", 5432};
    mpgp::PoolConstantsPtr poolConstants(mpgp::PoolConstants::create(1, 1, 0, 0));
    poolConstants->getTimeoutMs = 5000;
    poolConstants->pingIntervalMs = 5000;

    PgpoolLogger logger;
    std::unique_ptr<mpgp::Pool> pool(new mpgp::Pool(
        logger,
        std::move(poolConfiguration),
        "dbname=mapspro_prod_29_May_2015 user=mapspro password=mapspro",
        std::move(poolConstants)
    ));
    auto connection = pool->getMasterConnection();
    auto transaction = mpgp::makeWriteableTransaction(std::move(connection));
//     auto result = transaction->exec("select * from clusterizator.session");
// //    auto result = transaction->exec("select count(1) from clusterizator.session");
//     for (auto row: result) {
//         for (auto col: row) {
//             std::cout << col << " ";
//         }
//         std::cout << std::endl;
//     }
//    std::cout << result[0][0] << std::endl;


    maps::wiki::acl::ACLGateway gw(*transaction);
    // auto group = gw.group(378);
    auto group = gw.group(123);
    std::cout << "=======" << std::endl;
    for (const auto& user: group.users()) {
        std::cout << user.id() << std::endl;
    }
    std::cout << "=======" << std::endl;

    return 0;
}
