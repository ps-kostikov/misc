#pragma once

#include "http.h"

#include <yandex/maps/http2.h>
#include <yandex/maps/wiki/revision/common.h>

#include <yandex/blackbox/blackbox2.h>

#include <boost153/optional.hpp>

#include <string>
#include <memory>
#include <functional>

namespace maps {
namespace wiki {
namespace releases_notification {

typedef std::string Email;
typedef uint64_t UID;


class BlackboxConfiguration
{
public:
    BlackboxConfiguration(const std::string& url,
        const RetryPolicy& retryPolicy,
        const std::chrono::milliseconds& httpTimeout = std::chrono::seconds{3});

    const std::string& host() const;
    const RetryPolicy& retryPolicy() const;
    const std::chrono::milliseconds httpTimeout() const;

private:
    std::string host_;
    RetryPolicy retryPolicy_;
    std::chrono::milliseconds httpTimeout_;
};

struct UserSocialInfo
{
    Email email;
    boost153::optional<std::string> username;
    revision::UserID uid;
};

class Blackbox
{
public:
    explicit Blackbox(const BlackboxConfiguration&);

    bool isEmailValid(const Email&, const UID& uid) const;
    boost153::optional<UserSocialInfo> userInfo(const UID& uid) const;

private:
    std::unique_ptr<bb::Response> performInfoRequest(const std::string& infoRequest) const;

    BlackboxConfiguration configuration_;
    HTTPRetriedPerformer httpPerformer_;
};


} // namespace releases_notification
} // namespace wiki
} // namespace maps