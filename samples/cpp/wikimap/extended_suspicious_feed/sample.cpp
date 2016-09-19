#include <yandex/maps/pgpool3/pgpool3.h>
#include <yandex/maps/pgpool3/adapter_helpers.h>

#include <yandex/maps/wiki/acl/aclgateway.h>
#include <yandex/maps/wiki/acl/acl.h>

#include <yandex/maps/wiki/revision/branch.h>
#include <yandex/maps/wiki/revision/branch_manager.h>
#include <yandex/maps/wiki/revision/commit.h>
#include <yandex/maps/wiki/revision/snapshot.h>
#include <yandex/maps/wiki/revision/revisionsgateway.h>
#include <yandex/maps/wiki/revision/snapshot_id.h>
#include <yandex/maps/wiki/revision/exception.h>

#include <yandex/maps/wiki/social/gateway.h>
#include <yandex/maps/wiki/social/feed.h>
#include <yandex/maps/wiki/social/moderation.h>

#include <yandex/maps/wiki/common/date_time.h>
#include <yandex/maps/wiki/common/paged_result.h>

#include <boost/algorithm/string/split.hpp>

#include <iostream>
#include <string>

namespace mpgp = maps::pgpool3;
namespace mw = maps::wiki;
namespace mwr = maps::wiki::revision;
namespace mws = maps::wiki::social;

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

void printFeed(const mws::Feed& feed)
{
    const size_t batchSize = 100;
    auto count = feed.count();
    for (size_t batchIndex = 0; batchIndex <= count / batchSize; ++batchIndex) {
        auto events = feed.events(batchIndex * batchSize, batchSize);
        for (const auto& event: events) {
            std::cout << "created by: " << event.createdBy() << "; "
                << "commit id: " << event.commitData().commitId() << "; " << std::endl;
        }
        // std::cout << "event count = " << events.size() << std::endl;
    }
}

void evalSomething(
    maps::pgpool3::Pool& pool,
    std::string since,
    std::string till,
    const std::vector<std::string>& allowedActionStrs)
{
    auto txn = pool.slaveTransaction();
    // mwr::BranchManager branchManager(*txn);

    mws::Gateway sgw(*txn);
    mws::FeedFilter feedFilter;
    feedFilter.createdAfter(mw::common::parseIsoDateTime(since));
    feedFilter.createdBefore(mw::common::parseIsoDateTime(till));
    std::vector<mws::FeedAction> allowedActions;
    for (const auto& str: allowedActionStrs) {
        allowedActions.push_back(boost::lexical_cast<mws::FeedAction>(str));
    }
    feedFilter.actionsAllowed(allowedActions);
    // auto tillBranch = branchManager.load(tillBranchId);
    // mwr::RevisionsGateway rgw(*txn, tillBranch);
    // auto reader = rgw.reader();

    auto feed = sgw.suspiciousFeed(0, feedFilter);
    std::cout << "feed count = " << feed.count() << std::endl;
    printFeed(feed);
    // std::cout << "since = " << since << std::endl;
    // std::cout << "till = " << till << std::endl;

}

std::vector<std::string> strToVec(const std::string& str)
{
    auto isComma = [](char c)
    {
        return (c == ',');
    };
    std::vector<std::string> result;
    boost::split(result, str, isComma);
    return result;
}

int main(int argc, const char** argv)
{

    std::cout << "hello" << std::endl;
    if (argc < 5) {
        std::cout << "expected cmd line: ./<exe> <connection string> <since> <till> <allowed actions>" << std::endl;
        return 1;
    }

    std::string connStr(argv[1]);
    std::string since(argv[2]);
    std::string till(argv[3]);
    auto allowedActions = strToVec(argv[4]);

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

    evalSomething(*pool, since, till, allowedActions);

    return 0;
}
