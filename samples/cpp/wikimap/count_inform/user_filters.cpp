#include "user_filters.h"

#include <sstream>

namespace maps {
namespace wiki {
namespace releases_notification {

const std::string REASON_NOT_ACTIVE = "not active";
const std::string REASON_DECLINED_SUBSCRIPTION = "declined subscription";
const std::string REASON_NO_BLACKBOX_INFO = "not enough information from blackbox";
const std::string REASON_INVALID_EMAIL = "profile's email is invalid";

inline void logFilteredUser(revision::UserID uid, const std::string& reason)
{
    INFO() << "User with id = " << uid << " was filtered. Reason: " << reason;
}

UserFilter::UserFilter(BlackboxConfiguration bbConfig, pqxx::transaction_base& txn)
    : blackbox_(bbConfig)
    , socialGateway_(txn)
    , aclGateway_(txn)
{
}

boost153::optional<UserSocialInfo>
UserFilter::checkAndGetSocialInfo(revision::UserID uid)
{
    auto user = aclGateway_.user(uid);

    if (user.status() != acl::User::Status::Active) {
        logFilteredUser(uid, REASON_NOT_ACTIVE);
        return boost153::none;
    }

    const auto& profiles = socialGateway_.getUserProfiles({uid});
    if (profiles.empty()) {
        return blackbox_.userInfo(uid);
    }

    const auto& profile = profiles.front();
    if (not profile.hasBroadcastSubscription()) {
        logFilteredUser(uid, REASON_DECLINED_SUBSCRIPTION);
        return boost153::none;
    }

    auto userInfo = blackbox_.userInfo(uid);
    if (not userInfo) {
        logFilteredUser(uid, REASON_NO_BLACKBOX_INFO);
        return boost153::none;
    }

    if (not profile.email().empty()) {
        if (userInfo->email == profile.email() or blackbox_.isEmailValid(profile.email(), uid)) {
            return UserSocialInfo{profile.email(), userInfo->username, userInfo->uid};
        } else {
            logFilteredUser(uid, REASON_INVALID_EMAIL);
            return boost153::none;
        }
    } else {
        return userInfo;
    }
}

} // releases_notification
} // wiki
} // maps