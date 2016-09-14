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

#include <iostream>
#include <string>

namespace mpgp = maps::pgpool3;
namespace mw = maps::wiki;
namespace mwr = maps::wiki::revision;

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

struct Action
{
    mwr::Commit commit;
    mwr::ObjectRevision revision;
};

typedef std::vector<Action> Actions;


std::map<mwr::RevisionID, mwr::ObjectRevision> preloadObjectRevisions(
    mwr::RevisionsGateway& rgw, const mwr::RevisionIds& ids)
{
    auto reader = rgw.reader();
    auto revisions = reader.loadRevisions(ids);

    mwr::RevisionIds prevIds;
    for (const auto& revision: revisions) {
        if (!revision.prevId().empty()) {
            prevIds.push_back(revision.prevId());
        }
    }
    auto prevRevisions = reader.loadRevisions(prevIds);

    mwr::RevisionIds originalIds;
    for (const auto& revision: prevRevisions) {
        if (!revision.prevId().empty()) {
            originalIds.push_back(revision.prevId());
        }
    }
    auto originalRevisions = reader.loadRevisions(originalIds);


    std::map<mwr::RevisionID, mwr::ObjectRevision> result;
    for (const auto& revision: revisions) {
        result.emplace(revision.id(), revision);
    }
    for (const auto& revision: prevRevisions) {
        result.emplace(revision.id(), revision);
    }
    for (const auto& revision: originalRevisions) {
        result.emplace(revision.id(), revision);
    }
    return result;
}

std::map<mwr::DBID, mwr::Commit> preloadCommits(
    mwr::RevisionsGateway& rgw, const mwr::RevisionIds& ids)
{
    mwr::DBID maxCommitId = ids[0].commitId();
    for (const auto& id: ids) {
        maxCommitId = std::max(maxCommitId, id.commitId());
    }
    mwr::DBID minCommitId = ids[0].commitId();
    for (const auto& id: ids) {
        minCommitId = std::min(minCommitId, id.commitId());
    }
    auto commits = mwr::Commit::load(
        rgw.work(),
        mwr::filters::CommitAttr::id() <= maxCommitId &&
        mwr::filters::CommitAttr::id() >= minCommitId);

    std::map<mwr::DBID, mwr::Commit> result;
    for (const auto& commit: commits) {
        result.emplace(commit.id(), commit);
    }
    return result;
}

std::vector<Actions> loadActionChains(mwr::RevisionsGateway& rgw, const mwr::RevisionIds& ids)
{
    auto revisionMap = preloadObjectRevisions(rgw, ids);
    auto commitMap = preloadCommits(rgw, ids);

    std::vector<Actions> result;
    for (auto id: ids) {
        Actions actions;

        auto& revision = revisionMap.at(id);
        if (revisionMap.count(revision.prevId()) == 0) {
            continue;
        }
        auto& prevRevision = revisionMap.at(revision.prevId());
        if (revisionMap.count(prevRevision.prevId()) == 0) {
            continue;
        }
        auto& originalRevision = revisionMap.at(prevRevision.prevId());

        if (commitMap.count(revision.id().commitId()) == 0) {
            continue;
        }
        if (commitMap.count(prevRevision.id().commitId()) == 0) {
            continue;
        }
        if (commitMap.count(originalRevision.id().commitId()) == 0) {
            continue;
        }

        auto commit = commitMap.at(revision.id().commitId());
        auto prevCommit = commitMap.at(prevRevision.id().commitId());
        auto originalCommit = commitMap.at(originalRevision.id().commitId());

        actions.push_back(Action{commit, revision});
        actions.push_back(Action{prevCommit, prevRevision});
        actions.push_back(Action{originalCommit, originalRevision});

        result.emplace_back(actions);
    }
    return result;
}


bool isSuspicious(const Actions& actions, const std::string& attrName)
{
    if (actions.size() < 3) {
        return false;
    }

    const auto& action = actions[0];
    const auto& prevAction = actions[1];
    const auto& originalAction = actions[2];

    for (auto& r: {action.revision, prevAction.revision, originalAction.revision}) {
        if (r.data().attributes->count(attrName) == 0) {
            return false;
        }
    }

    if (action.commit.createdBy() == prevAction.commit.createdBy()) {
        return false;
    }

    auto attr = action.revision.data().attributes->at(attrName);
    auto prevAttr = prevAction.revision.data().attributes->at(attrName);
    auto originalAttr = originalAction.revision.data().attributes->at(attrName);
    if (attr != prevAttr and attr == originalAttr) {
        return true;
    }

    return false;
}

void evalSomething(maps::pgpool3::Pool& pool, int sinceBranchId, int tillBranchId, const std::string& attrName)
{
    auto txn = pool.slaveTransaction();
    mwr::BranchManager branchManager(*txn);

    auto tillBranch = branchManager.load(tillBranchId);
    mwr::RevisionsGateway rgw(*txn, tillBranch);
    auto reader = rgw.reader();




    maps::wiki::acl::ACLGateway agw(*txn);

    auto role = agw.role("moderator");
    auto pagedUserVector = agw.users(0, role.id(), 0, boost::none, 0, 0);
    std::vector<mw::social::TUid> uids;
    for (const auto& user: pagedUserVector.value()) {
        uids.push_back(user.uid());
    }

    std::cout << "uids size = " << uids.size() << std::endl;


    auto revisionIds = reader.loadRevisionIds(
        mwr::filters::CommitAttr::stableBranchId() > sinceBranchId &&
        mwr::filters::CommitAttr::stableBranchId() <= tillBranchId &&
        mwr::filters::ObjRevAttr::isNotDeleted() &&
        mwr::filters::Attr("cat:rd_el").defined() && 
        mwr::filters::CommitAttr("created_by").in(uids) /*&&
        mwr::filters::CommitAttribute("action").equals("commit-reverted")*/);
    std::cout << "revision ids size = " << revisionIds.size() << std::endl;

    auto chains = loadActionChains(rgw, revisionIds);
    std::cout << "num of chains = " << chains.size() << std::endl;

    Actions suspiciousActions;
    for (const auto& chain: chains) {
        if (isSuspicious(chain, attrName)) {
            suspiciousActions.push_back(chain[0]);
        }
    }
    std::cout << "num of suspicious actions = " << suspiciousActions.size() << std::endl;

    std::map<mwr::UserID, int> userCounts;
    for (const auto& action: suspiciousActions) {
        userCounts[action.commit.createdBy()] += 1;
    }

    std::vector<std::pair<mwr::UserID, int>> usersWithCounts;
    for (auto it = userCounts.begin(); it != userCounts.end(); ++it) {
        usersWithCounts.push_back(std::pair<mwr::UserID, int>{it->first, it->second});
    }
    std::sort(usersWithCounts.begin(), usersWithCounts.end(), [](const std::pair<mwr::UserID, int>& left, const std::pair<mwr::UserID, int>& right){
        return left.second < right.second;
    });

    std::cout << "moderators top:" << std::endl;
    for (auto it = usersWithCounts.rbegin(); it != usersWithCounts.rend(); ++it) {
        std::cout << "user id = " << it->first << "; num of corrections = " << it->second << std::endl;
    }
    std::cout << std::endl;

    std::sort(suspiciousActions.begin(), suspiciousActions.end(), [&userCounts](const Action& left, const Action& right) {
        auto leftAuthor = left.commit.createdBy();
        auto rightAuthor = right.commit.createdBy();

        auto leftCount = userCounts[leftAuthor];
        auto rightCount = userCounts[rightAuthor];

        if (leftCount != rightCount) {
            return leftCount > rightCount;
        }
        return leftAuthor < rightAuthor;
    });

    for (const auto& action: suspiciousActions) {
        std::cout << "user id = " << action.commit.createdBy()
            << "; commit id = " << action.revision.id().commitId()
            << "; object id = " << action.revision.id().objectId()
            << std::endl;
    }
}



int main(int argc, const char** argv)
{

    std::cout << "hello" << std::endl;
    std::cout << "argc = " << argc << std::endl;
    if (argc < 5) {
        std::cout << "expected cmd line: ./<exe> <connection string> <since branch> <till branch> <attr name>" << std::endl;
        return 1;
    }

    std::string connStr(argv[1]);
    int sinceBranchId = std::stoi(argv[2]);
    int tillBranchId = std::stoi(argv[3]);
    std::string attrName(argv[4]);

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

    evalSomething(*pool, sinceBranchId, tillBranchId, attrName);

    return 0;
}
