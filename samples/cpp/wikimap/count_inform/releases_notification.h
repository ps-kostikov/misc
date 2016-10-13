#pragma once

#include "common.h"
#include "sender.h"

#include <yandex/maps/wiki/common/geom.h>
#include <yandex/maps/wiki/common/date_time.h>
#include <yandex/maps/wiki/common/task_logger.h>
#include <yandex/maps/wiki/tasks/tasks.h>
#include <yandex/maps/wiki/revision/common.h>
#include <yandex/maps/wiki/revision/commit.h>

#include <yandex/maps/pgpool3/pgpool3.h>

#include <chrono>

namespace maps {
namespace wiki {
namespace releases_notification {

const std::string PUBLICATION_ZONE_AOI_NAME = "RKUB";

class VecUserData
{
public:
    explicit VecUserData(revision::UserID uid = 0) : uid_(uid), commitsCount_(0) {}

    void addCommit(const maps::wiki::revision::Commit& /*commit*/) { commitsCount_++; }
    size_t getCommitsCount() const { return commitsCount_; }
    revision::UserID getUid() const { return uid_; }

private:
    revision::UserID uid_;
    size_t commitsCount_;
};


class SatUserData
{
public:
    explicit SatUserData(revision::UserID uid = 0) : uid_(uid) {}

    revision::UserID getUid() const { return uid_; }
private:
    revision::UserID uid_;
};

std::map<maps::wiki::revision::UserID, VecUserData> getVecReleaseUsers_withoutReverts(
    maps::pgpool3::Pool& pool,
    maps::wiki::revision::DBID sinceBranchId,
    maps::wiki::revision::DBID tillBranchId);

std::map<maps::wiki::revision::UserID, VecUserData> getVecReleaseUsers_original(
    maps::pgpool3::Pool& pool,
    maps::wiki::revision::DBID sinceBranchId,
    maps::wiki::revision::DBID tillBranchId);

std::map<maps::wiki::revision::UserID, VecUserData> getVecReleaseUsers_takeAllRevertedInAccount(
    maps::pgpool3::Pool& pool,
    maps::wiki::revision::DBID sinceBranchId,
    maps::wiki::revision::DBID tillBranchId);

std::map<maps::wiki::revision::UserID, VecUserData> getVecReleaseUsers_takeAllDeletedInAccount(
    maps::pgpool3::Pool& pool,
    maps::wiki::revision::DBID sinceBranchId,
    maps::wiki::revision::DBID tillBranchId);

std::map<maps::wiki::revision::UserID, VecUserData> getVecReleaseUsers_onlyLastCommits(
    maps::pgpool3::Pool& pool,
    maps::wiki::revision::DBID sinceBranchId,
    maps::wiki::revision::DBID tillBranchId);

std::map<maps::wiki::revision::UserID, VecUserData> getVecReleaseUsers_onlyLastCommits_Final(
    maps::pgpool3::Pool& pool,
    maps::wiki::revision::DBID sinceBranchId,
    maps::wiki::revision::DBID tillBranchId);

std::map<revision::UserID, SatUserData> getSatReleaseUsers(
    maps::pgpool3::Pool& pool,
    const common::Geom& releaseGeom);

void notifyUsers(
    maps::pgpool3::Pool& pool,
    revision::DBID taskId,
    ISender& sender,
    tasks::TaskPgLogger &logger);

SimpleDate getBranchCreateTime(
    pqxx::transaction_base& txn,
    revision::DBID branchId);

enum class ReleaseType
{
    Sat, Vec
};

std::istream& operator>>(std::istream& is, ReleaseType& r);

enum class Mode
{
    Real, // Full workflow, available only in production

    Dry,  // Use fake sender and write messages only to log instead of real emails

    Test  // Send emails only to predefined list, 
          // even if there aren't any users with that email to notify
};

std::istream& operator>>(std::istream& is, Mode& r);

class TaskParams
{
public:
    TaskParams(pqxx::transaction_base& txn, revision::DBID dbTaskId);

    revision::DBID sinceBranchId() const
    {
        REQUIRE(sinceBranchId_, "Does not exists for that release type");
        return *sinceBranchId_;
    }

    revision::DBID tillBranchId() const
    {
        REQUIRE(tillBranchId_, "Does not exists for that release type");
        return *tillBranchId_;
    }

    common::Geom releaseGeom() const
    {
        REQUIRE(releaseGeom_, "Does not exists for that release type");
        return *releaseGeom_;
    }

    const std::string& blogUrl() const { return blogUrl_; }
    Mode mode() const { return mode_; }
    ReleaseType releaseType() const { return releaseType_; }
    revision::DBID dbTaskId() const { return dbTaskId_; }

private:
    boost::optional<revision::DBID> sinceBranchId_;
    boost::optional<revision::DBID> tillBranchId_;
    boost::optional<common::Geom> releaseGeom_;

    std::string blogUrl_;
    Mode mode_;
    ReleaseType releaseType_;
    revision::DBID dbTaskId_;
};

class ReleasesNotificationConfig
{
public:
    ReleasesNotificationConfig(const common::ExtendedXmlDoc& configDocPtr)
    : configDocPtr_(configDocPtr)
    {}

    std::string stringParam(const std::string& relPath) const;

private:
    const common::ExtendedXmlDoc& configDocPtr_;
};

size_t calculateAndSaveUsersForRelease(
    maps::pgpool3::Pool& corePool,
    maps::pgpool3::Pool& longReadPool,
    const ReleasesNotificationConfig& localConfig,
    const TaskParams& params);


} // releases_notification
} // wiki
} // maps