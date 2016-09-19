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

enum class BanStatus
{
    Active,
    TmpBanned,
    Banned
};

std::ostream& operator << (std::ostream& os, BanStatus s)
{
    switch (s) {
        case BanStatus::Active: os << "active"; break;
        case BanStatus::TmpBanned:  os << "tmp_banned";  break;
        case BanStatus::Banned: os << "banned";  break;
    }
    return os;
}
std::map<mws::TUid, BanStatus>
loadBanStatuses(
    maps::wiki::acl::ACLGateway& agw,
    const std::vector<mws::TUid>& uids)
{
    std::map<mws::TUid, BanStatus> result;
    for (const auto& uid: uids) {
        auto pagedBanRecords = agw.banRecords(uid, 0, 0);

        BanStatus status = BanStatus::Active;
        for (const auto& banRecord: pagedBanRecords.value()) {
            if (banRecord.expires().empty()) {
                status = BanStatus::Banned;
                break;
            } else {
                status = BanStatus::TmpBanned;
            }
        }
        result[uid] = status;
    }
    return result;
}

struct Situation
{
    mws::Event event;
    mwr::Commit moderatorCommit;
    mwr::ObjectRevision moderatorRevision;
    mwr::Commit userCommit;
    mwr::ObjectRevision userRevision;
};

std::vector<Situation>
loadSituations(
    const mws::Feed& feed,
    mwr::RevisionsGateway& rgw)
{
    std::vector<mws::Event> events;
    const size_t batchSize = 100;
    auto count = feed.count();
    for (size_t batchIndex = 0; batchIndex <= count / batchSize; ++batchIndex) {
        auto eventsBatch = feed.events(batchIndex * batchSize, batchSize);
        for (const auto& event: eventsBatch) {
            if (!event.primaryObjectData()) {
                continue;
            }
            events.push_back(event);
        }
    }

    std::map<mwr::DBID, mwr::Commit> preloadedCommits;
    std::map<mwr::RevisionID, mwr::ObjectRevision> preloadedObjectRevisions;

    {
        std::vector<mwr::DBID> commitIdsToLoad;
        for (const auto& event: events) {
            commitIdsToLoad.push_back(event.commitData().commitId());
        }
        std::cout << "before commit load" << std::endl;
        auto commits = mwr::Commit::load(
            rgw.work(),
            mwr::filters::CommitAttr::id().in(commitIdsToLoad));
        std::cout << "after commit load" << std::endl;

        for (const auto& commit: commits) {
            preloadedCommits.emplace(commit.id(), commit);
        }
    }

    std::vector<mwr::RevisionID> prevRevisionIdsToLoad;
    {
        std::vector<mwr::RevisionID> revisionIdsToLoad;
        for (const auto& event: events) {
            const auto objectId = event.primaryObjectData()->id();
            const auto commitId = event.commitData().commitId();
            revisionIdsToLoad.push_back(mwr::RevisionID{objectId, commitId});
        }
        auto reader = rgw.reader();
        std::cout << "before revisions load" << std::endl;
        auto revisions = reader.loadRevisions(revisionIdsToLoad);
        std::cout << "after revisions load" << std::endl;
        for (const auto& revision: revisions) {
            preloadedObjectRevisions.emplace(revision.id(), revision);
        }

        for (const auto& revision: revisions) {
            preloadedObjectRevisions.emplace(revision.id(), revision);
            if (!revision.prevId().empty()) {
                prevRevisionIdsToLoad.push_back(revision.prevId());
            }
        }
    }

    {
        std::vector<mwr::DBID> commitIdsToLoad;
        for (const auto& revisionId: prevRevisionIdsToLoad) {
            commitIdsToLoad.push_back(revisionId.commitId());
        }
        std::cout << "before prev commit load" << std::endl;
        auto commits = mwr::Commit::load(
            rgw.work(),
            mwr::filters::CommitAttr::id().in(commitIdsToLoad));
        std::cout << "after prev commit load" << std::endl;

        for (const auto& commit: commits) {
            preloadedCommits.emplace(commit.id(), commit);
        }
    }

    {
        auto reader = rgw.reader();
        std::cout << "before prev revisions load" << std::endl;
        auto revisions = reader.loadRevisions(prevRevisionIdsToLoad);
        std::cout << "after prev revisions load" << std::endl;
        for (const auto& revision: revisions) {
            preloadedObjectRevisions.emplace(revision.id(), revision);
        }
    }

    std::vector<Situation> situations;
    for (const auto& event: events) {

        const auto objectId = event.primaryObjectData()->id();
        const auto commitId = event.commitData().commitId();

        auto moderatorCommit = preloadedCommits.at(commitId);
        auto moderatorRevision = preloadedObjectRevisions.at(mwr::RevisionID{objectId, commitId});

        if (moderatorRevision.prevId().empty()) {
            continue;
        }
        auto userCommit = preloadedCommits.at(moderatorRevision.prevId().commitId());
        auto userRevision = preloadedObjectRevisions.at(moderatorRevision.prevId());

        situations.push_back(Situation{
            event,
            moderatorCommit,
            moderatorRevision,
            userCommit,
            userRevision
        });
    }
    return situations;
}

void printFeed(
    const mws::Feed& feed,
    const std::map<mws::TUid, mw::acl::User>& usersMap)
{
    const size_t batchSize = 100;
    auto count = feed.count();
    for (size_t batchIndex = 0; batchIndex <= count / batchSize; ++batchIndex) {
        auto events = feed.events(batchIndex * batchSize, batchSize);
        for (const auto& event: events) {
            if (!event.primaryObjectData()) {
                continue;
            }
            const auto uid = event.createdBy();
            const auto& user = usersMap.at(uid);
            const auto objectId = event.primaryObjectData()->id();
            const auto commitId = event.commitData().commitId();
            std::cout << "created by: " << user.login() << "(" << uid << "); "
                << "commit id: " << commitId << "; "
                << "primary object id: " << objectId << "; "
                << "history url: https://n.maps.yandex.ru/#!/objects/" << objectId << "/history/" << commitId << std::endl;
        }
        // std::cout << "event count = " << events.size() << std::endl;
    }
}


void printSituations(
    const std::vector<Situation>& situations,
    const std::map<mws::TUid, BanStatus>& banStatuses,
    maps::wiki::acl::ACLGateway& agw)
{
    std::vector<mws::TUid> uids;
    for (const auto& s: situations) {
        uids.push_back(s.moderatorCommit.createdBy());
        uids.push_back(s.userCommit.createdBy());
    }
    auto usersMap = loadUsers(agw, uids);

    for (const auto& s: situations) {
        auto moderator = usersMap.at(s.moderatorCommit.createdBy());
        auto user = usersMap.at(s.userCommit.createdBy());
        const auto objectId = s.event.primaryObjectData()->id();
        const auto commitId = s.event.commitData().commitId();
        std::cout << moderator.login() << "(" << moderator.uid() << ") "
            << "corrected " << user.login() << "(" << user.uid() << ") "
            << "(" << banStatuses.at(user.uid()) << ") "
            << "commit id: " << commitId << "; "
            << "primary object id: " << objectId << "; "
            << "history url: https://n.maps.yandex.ru/#!/objects/" << objectId << "/history/" << commitId << std::endl;
    }

}


std::vector<Situation>
filterSituations(
    const std::vector<Situation>& situations,
    const std::map<mws::TUid, BanStatus>& banStatuses)
{
    std::vector<Situation> result;
    for (const auto& s: situations) {
        if (s.moderatorCommit.createdBy() == s.userCommit.createdBy()) {
            continue;
        }

        if (banStatuses.at(s.userCommit.createdBy()) == BanStatus::Banned) {
            continue;
        }

        result.push_back(s);
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

    mwr::RevisionsGateway rgw(*txn);
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


    // printFeed(feed, usersMap);

    auto situations = loadSituations(feed, rgw);

    std::vector<mws::TUid> userUids;
    for (const auto& s: situations) {
        userUids.push_back(s.userCommit.createdBy());
    }
    auto banStatuses = loadBanStatuses(agw, userUids);
    // for (const auto& p: banStatuses) {
    //     std::cout << p.first << ": " << p.second << std::endl;
    // }
    auto filteredSituations = filterSituations(situations, banStatuses);
    printSituations(filteredSituations, banStatuses, agw);
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
