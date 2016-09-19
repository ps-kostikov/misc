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

void printFeed(
    const mws::Feed& feed,
    const std::map<mws::TUid, mw::acl::User>& usersMap)
{
    const size_t batchSize = 100;
    auto count = feed.count();
    for (size_t batchIndex = 0; batchIndex <= count / batchSize; ++batchIndex) {
        auto events = feed.events(batchIndex * batchSize, batchSize);
        for (const auto& event: events) {
            const auto uid = event.createdBy();
            const auto& user = usersMap.at(uid);
            std::cout << "created by: " << user.login() << "(" << uid << "); "
                << "commit id: " << event.commitData().commitId() << "; " << std::endl;
        }
        // std::cout << "event count = " << events.size() << std::endl;
    }
}

std::vector<mws::TUid> uidsFromRole(
    maps::wiki::acl::ACLGateway& agw,
    const std::string& roleName)
{
    auto role = agw.role(roleName);
    auto pagedUserVector = agw.users(0, role.id(), 0, boost::none, 0, 0);
    std::vector<mws::TUid> result;
    for (const auto& user: pagedUserVector.value()) {
        result.push_back(user.uid());
    }
    std::cout << "role '" << roleName << "' consists of " << result.size() << " users" << std::endl;
    return result;
}

std::map<mws::TUid, mw::acl::User>
loadUsers(
    maps::wiki::acl::ACLGateway& agw,
    const std::vector<mws::TUid>& uids)
{
    auto users = agw.users(uids);
    std::map<mws::TUid, mw::acl::User> result;
    for (const auto& user: users) {
        result.emplace(user.uid(), user);
    }
    return result;
}

void evalSomething(
    maps::pgpool3::Pool& pool,
    std::string since,
    std::string till,
    const std::vector<std::string>& allowedActionStrs,
    const std::string& roleName)
{
    auto txn = pool.slaveTransaction();
    // mwr::BranchManager branchManager(*txn);

    maps::wiki::acl::ACLGateway agw(*txn);
    mws::Gateway sgw(*txn);

    mws::FeedFilter feedFilter;
    feedFilter.createdAfter(mw::common::parseIsoDateTime(since));
    feedFilter.createdBefore(mw::common::parseIsoDateTime(till));
    std::vector<mws::FeedAction> allowedActions;
    for (const auto& str: allowedActionStrs) {
        allowedActions.push_back(boost::lexical_cast<mws::FeedAction>(str));
    }
    feedFilter.actionsAllowed(allowedActions);
    auto uids = uidsFromRole(agw, roleName);
    feedFilter.createdBy(uids);

    auto usersMap = loadUsers(agw, uids);
    // auto tillBranch = branchManager.load(tillBranchId);
    // mwr::RevisionsGateway rgw(*txn, tillBranch);
    // auto reader = rgw.reader();

    auto feed = sgw.suspiciousFeed(0, feedFilter);
    std::cout << "feed count = " << feed.count() << std::endl;
    printFeed(feed, usersMap);
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
    if (argc < 6) {
        std::cout << "expected cmd line: ./<exe> <connection string> <since> <till> <allowed actions> <role>" << std::endl;
        return 1;
    }

    std::string connStr(argv[1]);
    std::string since(argv[2]);
    std::string till(argv[3]);
    auto allowedActions = strToVec(argv[4]);
    std::string roleName(argv[5]);

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

    evalSomething(*pool, since, till, allowedActions, roleName);

    return 0;
}
