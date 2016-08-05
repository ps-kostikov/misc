#include "releases_notification.h"
#include "common.h"
#include "sender.h"
#include "user_filters.h"

#include <yandex/maps/log8/log.h>
#include <yandex/maps/pgpool3/pgpool3.h>
#include <yandex/maps/wiki/acl/aclgateway.h>
#include <yandex/maps/wiki/common/long_tasks.h>
#include <yandex/maps/wiki/common/moderation.h>
#include <yandex/maps/wiki/revision/branch.h>
#include <yandex/maps/wiki/revision/branch_manager.h>
#include <yandex/maps/wiki/revision/commit.h>
#include <yandex/maps/wiki/revision/snapshot.h>
#include <yandex/maps/wiki/revision/revisionsgateway.h>
#include <yandex/maps/wiki/revision/snapshot_id.h>
#include <yandex/maps/wiki/revision/exception.h>
#include <yandex/maps/wiki/social/gateway.h>

#include <algorithm>
#include <list>
#include <pqxx>
#include <set>
#include <vector>

#include <geos/geom/Geometry.h>
#include <geos/geom/GeometryFactory.h>
#include <geos/io/WKTReader.h>
#include <geos/io/WKTWriter.h>

namespace acl = maps::wiki::acl;
namespace revision = maps::wiki::revision;
namespace common = maps::wiki::common;
namespace social = maps::wiki::social;

namespace maps {
namespace wiki {
namespace releases_notification {

namespace {

typedef std::list<revision::Commit> CommitsBatch;
constexpr size_t BATCH_SIZE = 1000;

const std::string STATUS_SENT = "sent";
const std::string STATUS_SCHEDULED = "scheduled";

const std::string CONFIG_PREFIX = "/config/services/tasks/releases-notification";

const std::string MODE_TEST_STR = "test";
const std::string MODE_DRY_STR = "dry";
const std::string MODE_REAL_STR = "real";

const std::string RELEASE_TYPE_VEC_STR = "vec";
const std::string RELEASE_TYPE_SAT_STR = "sat";

}

class PublicationZone
{
public:
    PublicationZone(
        pqxx::transaction_base& txn,
        const revision::Branch& stableBranch)
    : socialGateway_(txn)
    {
        revision::RevisionsGateway rg(txn, stableBranch);
        auto releaseSnapshot = rg.stableSnapshot(rg.headCommitId());
        auto rkubRevisions = releaseSnapshot.objectRevisionsByFilter(
            revision::filters::Attr("aoi:name") == PUBLICATION_ZONE_AOI_NAME &&
            revision::filters::Geom::defined() &&
            revision::filters::ObjRevAttr::isNotDeleted() &&
            revision::filters::Attr("cat:aoi").defined());
        REQUIRE(rkubRevisions.size() == 1, "Publication zone cannot be determined");
        REQUIRE(rkubRevisions.front().data().geometry, "Incorrect geometry");

        geom_ = common::Geom(*rkubRevisions.front().data().geometry);
        INFO() << PUBLICATION_ZONE_AOI_NAME << " geometry loaded";
    }

    social::TIds filterIntersectCommits(const social::TIds& batchCommitIds)
    {
        auto commitEvents = socialGateway_.loadEditEventsByCommitIds(batchCommitIds);

        social::TIds intersectsCommitIds;
        for (const auto& event: commitEvents) {
            const auto& commitGeom = event.commitData().bbox();
            if (not commitGeom) {
                continue;
            }

            auto envelope = geos::geom::Envelope(
                commitGeom->minX(), commitGeom->maxX(), commitGeom->minY(), commitGeom->maxY());
            common::Geom bbox(geom_->getFactory()->toGeometry(&envelope));
            if (geom_->intersects(bbox.geosGeometryPtr())) {
                intersectsCommitIds.insert(event.commitData().commitId());
            }
        }

        return intersectsCommitIds;
    }

private:
    common::Geom geom_;
    social::Gateway socialGateway_;
};

std::map<revision::UserID, VecUserData> getVecReleaseUsers(
    maps::pgpool3::Pool& pool,
    revision::DBID sinceBranchId,
    revision::DBID tillBranchId
    )
{
    INFO() << "Get vec releases users. Since: " << sinceBranchId << " till: " << tillBranchId;

    auto txn = pool.slaveTransaction();
    revision::BranchManager branchManager(*txn);

    std::map<revision::UserID, VecUserData> committedUsers;
    for (revision::DBID branchId = sinceBranchId + 1; branchId <= tillBranchId; branchId++) {
        std::unique_ptr<revision::Branch> branchPtr;
        try {
            branchPtr.reset(new revision::Branch(branchManager.load(branchId)));
        } catch (const revision::BranchNotExistsException&) {
            if (branchId == tillBranchId) {
                throw;
            } else {
                continue;
            }
        }
        auto commits = revision::Commit::load(
            *txn,
            revision::filters::CommitAttr::stableBranchId() == branchId &&
            revision::filters::CommitAttr::isTrunk()
            );

        revision::DBIDSet revertedCommitIds;
        for (const auto& commit: commits) {
            const auto& curRevertedCommitIds = commit.revertedCommitIds();
            revertedCommitIds.insert(curRevertedCommitIds.begin(), curRevertedCommitIds.end());
        }

        PublicationZone publicationZone(*txn, *branchPtr);

        CommitsBatch batch;
        auto processBatch = [&]()
        {
            if (batch.empty()) {
                return;
            }
            social::TIds batchCommitIds;
            for (const auto& commit: batch) {
                batchCommitIds.insert(commit.id());
            }
            auto filteredCommitIds = publicationZone.filterIntersectCommits(batchCommitIds);
            for (const auto& commit: batch) {
                if (filteredCommitIds.count(commit.id())) {
                    if (committedUsers.count(commit.createdBy()) == 0) {
                        committedUsers[commit.createdBy()] = VecUserData(commit.createdBy());
                    }
                    committedUsers[commit.createdBy()].addCommit(commit);
                }
            }
            batch.clear();
        };
        for (const auto& commit: commits) {
            if (revertedCommitIds.count(commit.id()) == 0) {
                batch.push_back(commit);
            }
            if (batch.size() == BATCH_SIZE) {
                processBatch();
            }
        }
        processBatch();
    }

    INFO() << "Found " << committedUsers.size() << " unique users";
    return committedUsers;
}

std::map<revision::UserID, SatUserData> getUsersWithIntersectedSubscriptionZone(
    maps::pgpool3::Pool& pool,
    const common::Geom& releaseGeom)
{
    auto txn = pool.slaveTransaction();
    std::map<revision::UserID, SatUserData> affectedUsers;
    revision::RevisionsGateway rg(*txn);
    auto trunkSnapshot = rg.stableSnapshot(rg.headCommitId());

    std::stringstream query;
    query << " SELECT subscriber, feed_id as object_id"
          << " FROM social.subscription";

    std::map<revision::DBID, std::vector<revision::UserID>> feedSubscribers;
    std::set<revision::DBID> subscriptionObjectIds;
    for (const auto& row: txn->exec(query.str())) {
        auto uid = row["subscriber"].as<revision::UserID>();
        auto objectId = row["object_id"].as<revision::DBID>();

        feedSubscribers[objectId].push_back(uid);
        subscriptionObjectIds.insert(objectId);
    }

    if (subscriptionObjectIds.empty()) {
        return affectedUsers;
    }
    auto userSubscriptionRevisions = trunkSnapshot.objectRevisionsByFilter(
        revision::filters::Geom::defined() &&
        revision::filters::ObjRevAttr::isNotDeleted() &&
        revision::filters::ObjRevAttr::objectId().in(subscriptionObjectIds));

    for (const auto& revision: userSubscriptionRevisions) {
        if (!revision.data().geometry) {
            continue;
        }
        auto subscriptionGeom = common::Geom(*revision.data().geometry);
        if (subscriptionGeom->intersects(releaseGeom.geosGeometryPtr())) {
            for (auto uid: feedSubscribers[revision.id().objectId()]) {
                if (affectedUsers.count(uid) == 0) {
                    affectedUsers[uid] = SatUserData(uid);
                    INFO() << "Add user with uid = " << uid << " intersected by subscription";
                }
            }
        }
    }
    return affectedUsers;
}

std::map<revision::UserID, SatUserData> getUsersWithIntersectedModerationZone(
    maps::pgpool3::Pool& pool,
    const common::Geom& releaseGeom)
{
    auto txn = pool.slaveTransaction();
    std::map<revision::UserID, SatUserData> affectedUsers;

    acl::ACLGateway aclGateway(*txn);
    std::set<acl::ID> aoiIds;

    const auto& moderatorRole = aclGateway.role(common::MODERATION_STATUS_MODERATOR);
    for (const auto& policy: moderatorRole.policies()) {
        if (policy.aoi().wkb().empty()) {
            continue;
        }
        auto moderationGeom = common::Geom(policy.aoi().wkb());
        if (moderationGeom->intersects(releaseGeom.geosGeometryPtr())) {
            aoiIds.insert(policy.aoiID());
        }
    }

    for (auto aoiID: aoiIds) {
        auto users = aclGateway.users(
            0, moderatorRole.id(), aoiID, acl::User::Status::Active, 0, 0).value();
        for (const auto& user: users) {
            affectedUsers[user.uid()] = SatUserData(user.uid());
            INFO() << "Add user with uid = " << user.uid() << " intersected by moderation";
        }
    }

    return affectedUsers;
}

std::map<revision::UserID, SatUserData> getSatReleaseUsers(
    maps::pgpool3::Pool& pool,
    const common::Geom& releaseGeom)
{
    std::map<revision::UserID, SatUserData> affectedUsers;
    auto subscriptionZoneUsers = getUsersWithIntersectedSubscriptionZone(pool, releaseGeom);
    affectedUsers.insert(subscriptionZoneUsers.begin(), subscriptionZoneUsers.end());

    auto moderationZoneUsers = getUsersWithIntersectedModerationZone(pool, releaseGeom);
    affectedUsers.insert(moderationZoneUsers.begin(), moderationZoneUsers.end());

    return affectedUsers;
}

void notifyAndUpdate(
    pgpool3::Pool& pool,
    const pqxx::result::tuple& emailRow,
    ISender& sender)
{
    auto emailId = emailRow["id"].as<revision::DBID>();
    auto email = emailRow["email"].as<std::string>();
    auto attrs = NotificationEmailParams::fromJson(emailRow["attrs"].as<std::string>());

    auto setStatus = [&](const std::string& status) {
        std::ostringstream query;
        auto txn = pool.masterWriteableTransaction();
        query << " UPDATE service.releases_notification_email"
              << " SET status = " << txn->quote(status)
              << " WHERE id = " << emailId;
        txn->exec(query.str());
        txn->commit();
    };
    setStatus(STATUS_SENT);
    try {
        sender.send(email, attrs.toMap());
        INFO() << "Email to " << email << " has been sent";
    } catch (const std::exception& ex) {
        setStatus(STATUS_SCHEDULED);
        throw;
    }
}

class ProgressLogger
{
public:
    ProgressLogger(tasks::TaskPgLogger& logger, int totalNum)
    : logger_(logger)
    , totalNum_(totalNum)
    , currentNum_(0)
    {
        REQUIRE(totalNum_ > 0, "Incorrect logger param");
    }

    void addCompleted()
    {
        REQUIRE(currentNum_ + 1 <= totalNum_, "Progress overflow");
        int prevPercents = currentNum_ * 100 / totalNum_;
        int curPercents = (currentNum_ + 1) * 100 / totalNum_;
        currentNum_++;

        if (prevPercents / NOTIFICATION_LOG_FREQUENCY_PERCENTAGE < 
            curPercents / NOTIFICATION_LOG_FREQUENCY_PERCENTAGE) {
            logger_.logInfo() << currentNum_ << " of " << totalNum_ << " users have been notified";
        }
    }
private:
    tasks::TaskPgLogger& logger_;
    int totalNum_;
    int currentNum_;

    int NOTIFICATION_LOG_FREQUENCY_PERCENTAGE = 10;
};

void notifyUsers(
    pgpool3::Pool& pool,
    revision::DBID taskId,
    ISender& sender,
    tasks::TaskPgLogger& logger)
{
    auto mainTxn = pool.masterWriteableTransaction();
    int numEmailsSent = 0;
    int numEmailsTotal = 0;
    {
        std::ostringstream query;
        query << " SELECT id, email, hstore_to_json(sender_attrs) as attrs"
              << " FROM service.releases_notification_email"
              << " WHERE task_id = " << taskId
              << " AND status = " << mainTxn->quote(STATUS_SCHEDULED);
        auto result = mainTxn->exec(query.str());
        numEmailsTotal = result.affected_rows();
        if (numEmailsTotal > 0) {
            ProgressLogger progressLogger(logger, numEmailsTotal);
            for (const auto& row: result) {
                notifyAndUpdate(pool, row, sender);
                progressLogger.addCompleted();
                numEmailsSent++;
            }
        } else {
            logger.logInfo() << "No users to be notified";
        }
    }
    {
        std::ostringstream query;
        query << " UPDATE service.releases_notification_task"
              << " SET sent_notifications_num = " << numEmailsSent
              << " WHERE id = " << taskId;
        mainTxn->exec(query.str());
        mainTxn->commit();
    }
}

SimpleDate getBranchCreateTime(
    pqxx::transaction_base& txn,
    revision::DBID branchId)
{
    revision::BranchManager branchManager(txn);
    auto branch = branchManager.load(branchId);

    return SimpleDate::fromTimePoint(common::parseSqlDateTime(branch.createdAt()));
}

std::istream& operator>>(std::istream& is, ReleaseType& r)
{
    std::string s;
    is >> s;
    if (s == RELEASE_TYPE_VEC_STR) {
        r = ReleaseType::Vec;
    } else if (s == RELEASE_TYPE_SAT_STR) {
        r = ReleaseType::Sat;
    } else {
        throw Exception() << "Unrecognized release type: " << s;
    }
    return is;
}

std::istream& operator>>(std::istream& is, Mode& m)
{
    std::string s;
    is >> s;
    if (s == MODE_TEST_STR) {
        m = Mode::Test;
    } else if (s == MODE_REAL_STR) {
        m = Mode::Real;
    } else if (s == MODE_DRY_STR) {
        m = Mode::Dry;
    } else {
        throw Exception() << "Unrecognized mode: " << s;
    }
    return is;
}

TaskParams::TaskParams(pqxx::transaction_base& txn, revision::DBID dbTaskId)
: dbTaskId_(dbTaskId)
{
    {
        std::ostringstream query;
        query << " SELECT blog_url, mode, release_type"
              << " FROM service.releases_notification_task"
              << " WHERE id = " << dbTaskId_;
        auto result = txn.exec(query.str());
        REQUIRE(not result.empty(), "Task hasn't been prepared yet");

        blogUrl_ = result[0]["blog_url"].as<std::string>();
        mode_ = boost::lexical_cast<Mode>(result[0]["mode"].as<std::string>());
        releaseType_ = boost::lexical_cast<ReleaseType>(
            result[0]["release_type"].as<std::string>());
    }
    if (releaseType_ == ReleaseType::Vec) {
        std::ostringstream query;
        query << " SELECT since_branch_id, till_branch_id"
              << " FROM service.releases_notification_vec_param"
              << " WHERE task_id = " << dbTaskId_;
        auto result = txn.exec(query.str());
        REQUIRE(not result.empty(), "Cannot find vec params");

        sinceBranchId_ = result[0]["since_branch_id"].as<revision::DBID>();
        tillBranchId_ = result[0]["till_branch_id"].as<revision::DBID>();
    } else {
        std::ostringstream query;
        query << " SELECT ST_AsText(geom) as geom"
              << " FROM service.releases_notification_sat_param"
              << " WHERE task_id = " << dbTaskId;
        auto result = txn.exec(query.str());
        REQUIRE(not result.empty(), "Cannot find sat params");

        geos::io::WKTReader reader;
        releaseGeom_ = common::Geom(reader.read(result[0]["geom"].as<std::string>()));
    }
}

std::map<revision::UserID, NotificationEmailParams>
prepareSenderDataForVecRelease(
    const TaskParams& params,
    maps::pgpool3::Pool& longReadPool,
    const NotificationEmailParams& baseMessage)
{
    auto users = getVecReleaseUsers(
        longReadPool,
        params.sinceBranchId(),
        params.tillBranchId());

    std::map<revision::UserID, NotificationEmailParams> usersWithData;
    for (const auto& p: users) {
        auto message = baseMessage;
        message.addCorrectionsCount(p.second.getCommitsCount());
        usersWithData.insert({p.first, message});
    }
    return usersWithData;
}

std::map<revision::UserID, NotificationEmailParams>
prepareSenderDataForSatRelease(
    const TaskParams& params,
    maps::pgpool3::Pool& longReadPool,
    const NotificationEmailParams& baseMessage)
{
    auto users = getSatReleaseUsers(
        longReadPool,
        params.releaseGeom());

    std::map<revision::UserID, NotificationEmailParams> usersWithData;
    for (const auto& p: users) {
        auto message = baseMessage;
        usersWithData.insert({p.first, message});
    }
    return usersWithData;
}

std::map<Email, std::vector<NotificationEmailParams>>
prepareEmailsAndCheckNotificationPolicies(
    const BlackboxConfiguration& bbConfig,
    pqxx::transaction_base& txn,
    const std::map<revision::UserID, NotificationEmailParams>& usersWithMessages)
{
    UserFilter filter(bbConfig, txn);
    std::map<Email, std::vector<NotificationEmailParams>> result;
    for (const auto& userWithMessage: usersWithMessages) {
        auto socialInfo = filter.checkAndGetSocialInfo(userWithMessage.first);
        if (not socialInfo) {
            continue;
        }
        auto newMessage = userWithMessage.second;
        newMessage.addUid(socialInfo->uid);
        newMessage.addUserName(socialInfo->username ? *socialInfo->username : u8"пользователь");
        result[socialInfo->email].push_back(newMessage);
    }
    return result;
}

void addEmail(
    const TaskParams& params,
    const NotificationEmailParams& message,
    const Email& email,
    pqxx::transaction_base& txn)
{
    std::ostringstream query;
    query << " INSERT INTO service.releases_notification_email"
          << " (task_id, email, status, sender_attrs)"
          << " VALUES"
          << " ("
            << params.dbTaskId() << ", "
            << txn.quote(email) << ", "
            << "'scheduled'" << ", "
            << message.hstore(txn)
          << " )";
    txn.exec(query.str());
}

void updateEmail(
    const TaskParams& params,
    const NotificationEmailParams& message,
    const Email& email,
    pqxx::transaction_base& txn)
{
    std::ostringstream query;
    query << " UPDATE service.releases_notification_email"
          << " SET sender_attrs = " << message.hstore(txn)
          << " WHERE task_id = " << params.dbTaskId()
          << " AND email = " << txn.quote(email);
    txn.exec(query.str());
}

size_t calculateAndSaveUsersForRelease(
    maps::pgpool3::Pool& corePool,
    maps::pgpool3::Pool& longReadPool,
    const ReleasesNotificationConfig& localConfig,
    const TaskParams& params)
{
    NotificationEmailParams commonMessage(
        params.blogUrl(),
        localConfig.stringParam("/club-url"),
        localConfig.stringParam("/feedback-url"),
        localConfig.stringParam("/unsubscribe-base-url"));

    std::map<revision::UserID, NotificationEmailParams> usersWithMessages;
    auto slaveTxn = longReadPool.slaveTransaction();
    if (params.releaseType() == ReleaseType::Vec) {
        auto sinceDate = getBranchCreateTime(
            *slaveTxn,
            params.sinceBranchId());
        commonMessage.addSince(sinceDate);

        auto tillDate = getBranchCreateTime(
            *slaveTxn,
            params.tillBranchId());
        commonMessage.addTill(tillDate);
        commonMessage.addCorrectionsCount(0);

        usersWithMessages = prepareSenderDataForVecRelease(
            params, longReadPool, commonMessage);
    } else {
        usersWithMessages = prepareSenderDataForSatRelease(
            params, longReadPool, commonMessage);
    }

    BlackboxConfiguration bbConfig(
        localConfig.stringParam("/blackbox-url"),
        RetryPolicy::defaultPolicy());
    auto filteredEmailsWithMessages = prepareEmailsAndCheckNotificationPolicies(
        bbConfig,
        *slaveTxn,
        usersWithMessages);

    size_t totalNumberOfMessages = 0;
    auto taskTxn = corePool.masterWriteableTransaction();

    if (params.mode() == Mode::Test) {
        std::ostringstream query;
        query << " SELECT email"
              << " FROM service.releases_notification_email"
              << " WHERE task_id = " << params.dbTaskId();
        for (const auto& row: taskTxn->exec(query.str())) {
            const auto& email = row["email"].as<Email>();
            if (filteredEmailsWithMessages.count(email)) {
                bool first = true;
                for (const auto& message: filteredEmailsWithMessages.at(email)) {
                    if (first) {
                        updateEmail(params, message, email, *taskTxn);
                        first = false;
                    } else {
                        addEmail(params, message, email, *taskTxn);
                    }
                    totalNumberOfMessages++;
                }
            } else {
                auto message = commonMessage;
                message.addUserName(u8"тестовый пользователь");
                updateEmail(params, message, email, *taskTxn);
                totalNumberOfMessages++;
            }
        }
    } else {
        for (const auto& p: filteredEmailsWithMessages) {
            const auto& email = p.first;
            const auto& messages = p.second;
            for (const auto& message: messages) {
                addEmail(params, message, email, *taskTxn);
                totalNumberOfMessages++;
            }
        }
    }
    tasks::freezeTask(*taskTxn, params.dbTaskId());
    taskTxn->commit();
    return totalNumberOfMessages;
}

std::string ReleasesNotificationConfig::stringParam(const std::string& relPath) const
{
    return configDocPtr_.get<std::string>(CONFIG_PREFIX + relPath);
}

} // releases_notification
} // wiki
} // maps
