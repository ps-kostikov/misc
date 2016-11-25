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
#include <yandex/maps/common/profiletimer.h>

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

std::string
objectEditNotesKey(mwr::DBID objectId)
{
    return "edit_notes:" + std::to_string(objectId);
}

std::string
primaryObjectKey(mwr::DBID objectId)
{
    return "primary_object:" + std::to_string(objectId);
}


struct FeedItem
{
    mwr::DBID commitId;
    mwr::DBID objectId;
};

std::vector<FeedItem>
loadItems(maps::pgpool3::Pool& pool, std::string /*userName*/)
{
    auto txn = pool.slaveTransaction();
    mws::Gateway sgw(*txn);

    auto feed = sgw.feed(0, /*83229408*/299291332, mws::FeedType::User);

    auto events = feed.events(0, 20);

    std::vector<FeedItem> result;
    for (const auto& event: events) {
        if (!event.primaryObjectData()) {
            continue;
        }
        result.push_back(FeedItem{event.commitData().commitId(), event.primaryObjectData()->id()});
    }

    // for (const auto i: result) {
    //     std::cout << i.commitId << " " << i.objectId << std::endl;
    // }

    return result;
}

void conductExperiment(maps::pgpool3::Pool& pool, const std::vector<FeedItem>& items)
{
    auto txn = pool.slaveTransaction();

    std::vector<mwr::DBID> objectIds;
    for (const auto& item: items) {
        objectIds.push_back(item.objectId);
    }
    // const auto objectId = items.at(0).objectId;

    // auto branchCtx = BranchContextFacade(request_.branchId).acquireReadCoreOnly(request_.token);

    mwr::RevisionsGateway gateway(*txn);
    auto snapshot = gateway.historicalSnapshot(gateway.headCommitId());

    auto revsFilter = (
        mwr::filters::ObjRevAttr::objectId().in(objectIds) &&
        mwr::filters::ObjRevAttr::isNotRelation()
    );

    auto revisionIds = snapshot.revisionIdsByFilter(revsFilter);
    std::set<mwr::DBID> commitIds;
    for (const auto& revId : revisionIds) {
        commitIds.insert(revId.commitId());
    }

    auto relsFilter = (
        mwr::filters::ObjRevAttr::isRelation() && (
            mwr::filters::ObjRevAttr::slaveObjectId().in(objectIds) ||
            mwr::filters::ObjRevAttr::masterObjectId().in(objectIds)
        )
    );
    auto relations = snapshot.relationsByFilter(relsFilter);
    for (const auto& relation : relations) {
        commitIds.insert(relation.id().commitId());
    }

    
    std::vector<std::string> commitAttrs;
    for (const auto& objectId: objectIds) {
        commitAttrs.push_back(objectEditNotesKey(objectId));
        commitAttrs.push_back(primaryObjectKey(objectId));
    };
    auto commitsFilter = (
        mwr::filters::CommitAttribute::definedAny(commitAttrs) &&
        mwr::filters::CommitAttr::isTrunk()
    );
    auto commits = mwr::Commit::load(*txn, commitsFilter);

    for (const auto& commit : commits) {
        commitIds.insert(commit.id());
    }

    std::cout << commitIds.size() << std::endl;

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

    auto items = loadItems(*pool, "ps-kostikov");
    std::cout << items.size() << std::endl;
    // const mwr::DBID objectId = 1687954226;
    // std::vector<FeedItem> items{{0, 1687954226}};

    ProfileTimer timer;
    conductExperiment(*pool, items);
    std::cout << timer.GetElapsedTime() << std::endl;

    return 0;
}
