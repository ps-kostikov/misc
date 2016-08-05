#pragma once

#include <yandex/maps/wiki/common/date_time.h>
#include <yandex/maps/wiki/common/extended_xml_doc.h>
#include <yandex/maps/wiki/revision/common.h>

#include <yandex/maps/http2.h>
#include <yandex/maps/log8.h>
#include <yandex/maps/json/builder.h>
#include <yandex/maps/json/std.h>
#include <yandex/maps/json/value.h>

#include <boost153/optional.hpp>

#include <chrono>
#include <map>
#include <thread>
#include <pqxx>

namespace maps {
namespace wiki {
namespace releases_notification {

constexpr int HTTP_STATUS_OK = 200;
constexpr int HTTP_STATUS_SERVER_ERROR = 500;

typedef std::string Email;
typedef std::string Hstore;

struct RetryPolicy
{
    size_t maxAttempts;
    std::chrono::milliseconds initialTimeout;
    double timeoutBackoff;

    static RetryPolicy defaultPolicy();
};

template<typename TRequest>
class HTTPRetriedPerformer
{
public:
    explicit HTTPRetriedPerformer(RetryPolicy policy)
    : policy_(std::move(policy))
    {}

    http2::Response perform(TRequest& request, const http2::URL& url) const
    {
        std::ostringstream errors;
        auto retryInterval = policy_.initialTimeout;
        for (size_t retryNumber = 1; retryNumber <= policy_.maxAttempts; ++retryNumber) {
            try {
                auto response = request.perform();
                if (response.status() < HTTP_STATUS_SERVER_ERROR) {
                    return response;
                } else {
                    WARN() << "Retry #" << retryNumber
                        << ": http code: " << response.status();
                    errors << "Retry #" << retryNumber
                        << ": http code: " << response.status() << ". ";
                }
            } catch (const http2::Error& e) {
                WARN() << "failed HTTP request attempt " << retryNumber
                    << "; url: " << url << " " << e.what();
                errors << "Retry #" << retryNumber << ": " << e.what() << ". ";
            }

            if (retryNumber < policy_.maxAttempts) {
                std::this_thread::sleep_for(retryInterval);
                retryInterval = std::chrono::milliseconds(std::chrono::milliseconds::rep(
                    retryInterval.count() * policy_.timeoutBackoff));
            }
        }
        throw maps::RuntimeError() << "Reached max retries to make HTTP request"
                << "; url: " << url << " errors: " << errors.str();
    }

private:
    const RetryPolicy policy_;
};

/**
 * Keep date (day, month, year) as strings in right locale
 */
class SimpleDate
{
public:
    static SimpleDate fromTimePoint(common::TimePoint tp);

    std::string day;
    std::string month;
    std::string year;
};

std::string eraseSchema(const std::string& url);

class NotificationEmailParams
{
public:
    explicit NotificationEmailParams(std::map<std::string, std::string> dict);

    NotificationEmailParams(
        std::string blogUrl,
        std::string clubUrl,
        std::string feedbackUrl,
        std::string unsubscribeBaseUrl);

    static NotificationEmailParams fromJson(const std::string& hstore);

    NotificationEmailParams(const NotificationEmailParams& other);
    NotificationEmailParams& operator=(const NotificationEmailParams& other);

    std::map<std::string, std::string> toMap() const;
    Hstore hstore(const pqxx::transaction_base& work) const;

    NotificationEmailParams& addSince(const SimpleDate& since);
    NotificationEmailParams& addTill(const SimpleDate& till);
    NotificationEmailParams& addUserName(const std::string& userName);
    NotificationEmailParams& addCorrectionsCount(size_t correctionsCount);
    NotificationEmailParams& addUid(revision::UserID uid);

private:
    std::map<std::string, std::string> dict_;

    const std::string SINCE = "since_";
    const std::string TILL = "till_";
    const std::string DAY = "day";
    const std::string MONTH = "month";
    const std::string YEAR = "year";

    const std::string USER_NAME = "username";
    const std::string CORRECTIONS_COUNT = "corrections_count";
    const std::string BLOG_URL = "blog_url";
    const std::string CLUB_URL = "club_url";
    const std::string FEEDBACK_URL = "feedback_url";
    const std::string UNSUBSCRIBE_BASE_URL = "unsubscribe_base_url";
    const std::string UID = "uid";
    const std::string UNSUBSCRIBE_URL = "unsubscribe_url";
};

struct UserSocialInfo
{
    Email email;
    boost153::optional<std::string> username;
    revision::UserID uid;
};

} // namespace releases_notification
} // namespace wiki
} // namespace maps