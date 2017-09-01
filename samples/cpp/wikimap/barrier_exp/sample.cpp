#include <yandex/maps/pgpool3/pgpool3.h>
#include <yandex/maps/pgpool3/adapter_helpers.h>

#include <yandex/maps/geolib3/point.h>
#include <yandex/maps/geolib3/polygon.h>
#include <yandex/maps/geolib3/serialization.h>
#include <yandex/maps/geolib3/transform.h>

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
#include <yandex/maps/wiki/social/feedback/task.h>
#include <yandex/maps/wiki/social/moderation.h>

#include <yandex/maps/wiki/common/date_time.h>
#include <yandex/maps/wiki/common/geom_utils.h>
#include <yandex/maps/wiki/common/paged_result.h>
#include <yandex/maps/common/profiletimer.h>

#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>

namespace mpgp = maps::pgpool3;
namespace mg3 = maps::geolib3;
namespace mw = maps::wiki;
namespace mwr = maps::wiki::revision;
namespace mwrf = maps::wiki::revision::filters;
namespace mws = maps::wiki::social;
namespace mwc = maps::wiki::common;

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

mg3::Point2
wkbStringToPoint(const std::string& wkbStr)
{
    std::stringstream ss;
    ss << wkbStr;
    return mg3::WKB<mg3::Point2>::read(ss);
}

struct BarrierTask
{
    mwc::TimePoint createdAt;
    mg3::Point2 position;
};

std::vector<BarrierTask>
loadTasks(maps::pgpool3::Pool& pool)
{
    auto txn = pool.slaveTransaction();

    std::stringstream query;
    query << "select created_at, st_asbinary(position) as position from social.feedback_task"
        << " where description like '%%На карте отсутствует шлагбаум%%'"
        << " and created_at > now() - '30 days'::interval";
        ;
    auto rows = txn->exec(query.str());
    std::vector<BarrierTask> result;
    for (const auto& row: rows) {
        BarrierTask b{
            mwc::parseSqlDateTime(row["created_at"].as<std::string>()),
            wkbStringToPoint(pqxx::binarystring(row["position"]).str())
        };
        result.push_back(b);
    }

    return result;
}

bool isCommitYonger(
    maps::pgpool3::Pool& revisionPool,
    int64_t commitId,
    const BarrierTask& barrierTask)
{
    auto txn = revisionPool.slaveTransaction();
    std::stringstream query;
    query << "select count(1) from revision.commit"
        << " where id = " << commitId
        << " and created < " << txn->quote(mwc::formatSqlDateTime(barrierTask.createdAt));
    auto rows = txn->exec(query.str());
    return rows[0]["count"].as<int>() > 0;
}

double correctDistance(double distance, const BarrierTask& barrierTask)
{
    auto geoPoint = mg3::GeodeticToMercatorTransform2()(
        barrierTask.position,
        mg3::TransformDirection::Backward);
    return distance / mwc::mercatorDistanceRatio(geoPoint.y());
}

bool barrierHadBeenAlreadySet(
    maps::pgpool3::Pool& revisionPool,
    maps::pgpool3::Pool& viewTrunkPool,
    const BarrierTask& barrierTask,
    double distance)
{
    distance = correctDistance(distance, barrierTask);
    auto txn = viewTrunkPool.slaveTransaction();

    auto x = barrierTask.position.x();
    auto y = barrierTask.position.y();

    std::stringstream barrieSS;
    barrieSS << std::setprecision(8);
    barrieSS << "select min(commit_id) from vrevisions_trunk.objects_p" 
        << " where service_attrs->'srv:hotspot_label' = '{{attr-values:cond-cond_type__5}}'"
        << "    and st_dwithin(the_geom, st_setsrid(st_makepoint(" << x << ", " << y << "), 3395), " << distance << ")";
    auto rows = txn->exec(barrieSS.str());
    auto row = rows[0];

    if (row["min"].is_null()) {
        return false;
    } 
    return isCommitYonger(revisionPool, row["min"].as<int64_t>(), barrierTask);
}



int main(int argc, const char** argv)
{

    std::cout << "hello" << std::endl;
    std::cout << "argc = " << argc << std::endl;
    if (argc < 4) {
        std::cout << "expected cmd line: ./<exe> <revision connection string> <social connection string> <view_trunk connection string>" << std::endl;
        return 1;
    }

    std::string revisionConnStr(argv[1]);
    std::string socialConnStr(argv[2]);
    std::string viewTrunkConnStr(argv[3]);

    std::cout << "revision connection string = '" << revisionConnStr << "'" << std::endl;
    mpgp::PoolConfigurationPtr revisionPoolConfiguration(mpgp::PoolConfiguration::create());
    std::string revisionAuthParams;
    std::tie(revisionPoolConfiguration->master, revisionAuthParams) = mpgp::helpers::parseConnString(revisionConnStr);
    mpgp::PoolConstantsPtr revisionPoolConstants(mpgp::PoolConstants::create(1, 1, 0, 0));
    revisionPoolConstants->getTimeoutMs = 5000;
    revisionPoolConstants->pingIntervalMs = 5000;

    PgpoolLogger revisionLogger;
    std::unique_ptr<mpgp::Pool> revisionPool(new mpgp::Pool(
        revisionLogger,
        std::move(revisionPoolConfiguration),
        revisionAuthParams,
        std::move(revisionPoolConstants)
    ));


    std::cout << "social connection string = '" << socialConnStr << "'" << std::endl;
    mpgp::PoolConfigurationPtr socialPoolConfiguration(mpgp::PoolConfiguration::create());
    std::string socialAuthParams;
    std::tie(socialPoolConfiguration->master, socialAuthParams) = mpgp::helpers::parseConnString(socialConnStr);
    mpgp::PoolConstantsPtr socialPoolConstants(mpgp::PoolConstants::create(1, 1, 0, 0));
    socialPoolConstants->getTimeoutMs = 5000;
    socialPoolConstants->pingIntervalMs = 5000;

    PgpoolLogger socialLogger;
    std::unique_ptr<mpgp::Pool> socialPool(new mpgp::Pool(
        socialLogger,
        std::move(socialPoolConfiguration),
        socialAuthParams,
        std::move(socialPoolConstants)
    ));


    std::cout << "view trunk connection string = '" << viewTrunkConnStr << "'" << std::endl;
    mpgp::PoolConfigurationPtr viewTrunkPoolConfiguration(mpgp::PoolConfiguration::create());
    std::string viewTrunkAuthParams;
    std::tie(viewTrunkPoolConfiguration->master, viewTrunkAuthParams) = mpgp::helpers::parseConnString(viewTrunkConnStr);
    mpgp::PoolConstantsPtr viewTrunkPoolConstants(mpgp::PoolConstants::create(1, 1, 0, 0));
    viewTrunkPoolConstants->getTimeoutMs = 5000;
    viewTrunkPoolConstants->pingIntervalMs = 5000;

    PgpoolLogger viewTrunkLogger;
    std::unique_ptr<mpgp::Pool> viewTrunkPool(new mpgp::Pool(
        viewTrunkLogger,
        std::move(viewTrunkPoolConfiguration),
        viewTrunkAuthParams,
        std::move(viewTrunkPoolConstants)
    ));


    auto tasks = loadTasks(*socialPool);
    auto total = tasks.size();
    std::cout << "total: " << total << std::endl;

    int count10 = 0;
    int count20 = 0;
    int count30 = 0;
    int count40 = 0;
    int count50 = 0;
    int count100 = 0;

    for (const auto& task: tasks) {
        if (barrierHadBeenAlreadySet(*revisionPool, *viewTrunkPool, task, 10.)) {
            ++count10;
        }
        if (barrierHadBeenAlreadySet(*revisionPool, *viewTrunkPool, task, 20.)) {
            ++count20;
        }
        if (barrierHadBeenAlreadySet(*revisionPool, *viewTrunkPool, task, 30.)) {
            ++count30;
        }
        if (barrierHadBeenAlreadySet(*revisionPool, *viewTrunkPool, task, 40.)) {
            ++count40;
        }
        if (barrierHadBeenAlreadySet(*revisionPool, *viewTrunkPool, task, 50.)) {
            ++count50;
        }
        if (barrierHadBeenAlreadySet(*revisionPool, *viewTrunkPool, task, 100.)) {
            ++count100;
        }
    }

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "10m: " << 100 * count10 / double(total) << "%% would be closed" << std::endl;
    std::cout << "20m: " << 100 * count20 / double(total) << "%% would be closed" << std::endl;
    std::cout << "30m: " << 100 * count30 / double(total) << "%% would be closed" << std::endl;
    std::cout << "40m: " << 100 * count40 / double(total) << "%% would be closed" << std::endl;
    std::cout << "50m: " << 100 * count50 / double(total) << "%% would be closed" << std::endl;
    std::cout << "100m: " << 100 * count100 / double(total) << "%% would be closed" << std::endl;
    return 0;
}
