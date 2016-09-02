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

// void researchRevisionId(mwr::RevisionsGateway& rgw, const mwr::RevisionID& revisionId)
void researchRevisionId(
    mwr::ObjectRevision& revision,
    mwr::ObjectRevision& prevRevision,
    mwr::ObjectRevision& originalRevision)
{
    std::string attrName = "rd_el:speed_limit";
    // std::string attrName = "rd_el:speed_cat";
    for (auto& r: {revision, prevRevision, originalRevision}) {
        if (r.data().attributes->count(attrName) == 0) {
            return;
        }
    }
    auto speedCat = revision.data().attributes->at(attrName);
    auto prevSpeedCat = prevRevision.data().attributes->at(attrName);
    auto originalSpeedCat = originalRevision.data().attributes->at(attrName);
    if (speedCat != prevSpeedCat and speedCat == originalSpeedCat) {
        std::cout << "commit id = " << revision.id().commitId()
            << "; obj revision id = " << revision.id().objectId() << std::endl;

        std::cout << "speed cat: " << speedCat << std::endl;
        std::cout << "prev speed cat: " << prevSpeedCat << std::endl;
        std::cout << std::endl;
    }

}

void evalSomething(maps::pgpool3::Pool& pool)
{
    auto txn = pool.slaveTransaction();
    mwr::BranchManager branchManager(*txn);

    int sinceBranchId = 145;
    int tillBranchId = 146;

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

    auto revisionMap = preloadObjectRevisions(rgw, revisionIds);

    // int limit = std::min(static_cast<int>(revisionIds.size()), 20);
    int limit = static_cast<int>(revisionIds.size());
    for (int i = 0; i < limit; ++i) {
        auto& revision = revisionMap.at(revisionIds[i]);
        if (revisionMap.count(revision.prevId()) == 0) {
            continue;
        }
        auto& prevRevision = revisionMap.at(revision.prevId());
        if (revisionMap.count(prevRevision.prevId()) == 0) {
            continue;
        }
        auto& originalRevision = revisionMap.at(prevRevision.prevId());
        researchRevisionId(revision, prevRevision, originalRevision);
    }

  


    // auto commits = mwr::Commit::load(
    //     *txn,
    //     mwr::filters::CommitAttr::stableBranchId() > sinceBranchId &&
    //     mwr::filters::CommitAttr::stableBranchId() <= tillBranchId &&
    //     mwr::filters::CommitAttr::isTrunk()
    //     );
    // std::cout << "commit size = " << commits.size() << std::endl;

    // mwr::DBID maxCommitId = 0;
    // for (const auto& commit: commits) {
    //     maxCommitId = std::max(commit.id(), maxCommitId);
    // }
    // std::cout << "max commit id = " << maxCommitId << std::endl;

    // auto tillBranch = branchManager.load(tillBranchId);
    // mwr::RevisionsGateway rgw(*txn, tillBranch);
    // auto snapshot = rgw.snapshot(maxCommitId);
    // auto revisionIds = snapshot.revisionIdsByFilter(
    //     mwr::filters::CommitAttr::stableBranchId() > sinceBranchId &&
    //     mwr::filters::CommitAttr::stableBranchId() <= tillBranchId &&
    //     mwr::filters::ObjRevAttr::isNotDeleted());

    // std::cout << "revision ids size = " << revisionIds.size() << std::endl;

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

    evalSomething(*pool);

    return 0;
}
