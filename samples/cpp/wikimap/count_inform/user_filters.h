#pragma once

#include "blackbox.h"
#include "common.h"

#include <yandex/maps/wiki/acl/aclgateway.h>
#include <yandex/maps/wiki/social/gateway.h>
#include <yandex/maps/wiki/revision/common.h>
#include <yandex/maps/pgpool3/pgpool3.h>

#include <boost153/optional.hpp>

namespace maps {
namespace wiki {
namespace releases_notification {

class UserFilter
{
public:
    UserFilter(BlackboxConfiguration bbConfig, pqxx::transaction_base& txn);

    boost153::optional<UserSocialInfo> checkAndGetSocialInfo(maps::wiki::revision::UserID uid);

private:
    Blackbox blackbox_;
    social::Gateway socialGateway_;
    acl::ACLGateway aclGateway_;
};

} // releases_notification
} // wiki
} // maps