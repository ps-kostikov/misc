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
    std::cout << "before load revisions" << std::endl;
    auto revisions = reader.loadRevisions(ids);
    std::cout << "after load revisions" << std::endl;

    mwr::RevisionIds prevIds;
    for (const auto& revision: revisions) {
        if (!revision.prevId().empty()) {
            prevIds.push_back(revision.prevId());
        }
    }
    std::cout << "before load prev revisions" << std::endl;
    auto prevRevisions = reader.loadRevisions(prevIds);
    std::cout << "after load prev revisions" << std::endl;

    mwr::RevisionIds originalIds;
    for (const auto& revision: prevRevisions) {
        if (!revision.prevId().empty()) {
            originalIds.push_back(revision.prevId());
        }
    }
    std::cout << "before load orig revisions" << std::endl;
    auto originalRevisions = reader.loadRevisions(originalIds);
    std::cout << "after load orig revisions" << std::endl;


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

void evalSomething(maps::pgpool3::Pool& pool, int sinceBranchId, int tillBranchId)
{
    auto txn = pool.slaveTransaction();
    mwr::BranchManager branchManager(*txn);

    auto tillBranch = branchManager.load(tillBranchId);
    mwr::RevisionsGateway rgw(*txn);
    // mwr::RevisionsGateway rgw(*txn, tillBranch);
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
        mwr::filters::CommitAttr::isTrunk() &&
        mwr::filters::CommitAttr::stableBranchId() > sinceBranchId &&
        mwr::filters::CommitAttr::stableBranchId() <= tillBranchId &&
        mwr::filters::ObjRevAttr::isNotDeleted() &&
        mwr::filters::Attr("cat:rd_el").defined() && 
        mwr::filters::CommitAttr("created_by").in(uids) /*&&
        mwr::filters::CommitAttribute("action").equals("commit-reverted")*/);
    std::cout << "revision ids size = " << revisionIds.size() << std::endl;

    auto allRevisionsMap = preloadObjectRevisions(rgw, revisionIds);
    std::cout << "all revisions size = " << allRevisionsMap.size() << std::endl;
}



int main(int argc, const char** argv)
{

    std::cout << "hello" << std::endl;
    std::cout << "argc = " << argc << std::endl;
    if (argc < 4) {
        std::cout << "expected cmd line: ./<exe> <connection string> <since branch> <till branch>" << std::endl;
        return 1;
    }

    std::string connStr(argv[1]);
    int sinceBranchId = std::stoi(argv[2]);
    int tillBranchId = std::stoi(argv[3]);

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

    evalSomething(*pool, sinceBranchId, tillBranchId);

    return 0;
}
