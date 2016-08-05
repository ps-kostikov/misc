#include "common.h"

#include <yandex/maps/common/exception.h>
#include <yandex/maps/json/value.h>

#include <boost153/algorithm/string/predicate.hpp>

#include <sstream>
#include <vector>

namespace maps {
namespace wiki {
namespace releases_notification {

namespace {

const size_t MAX_REQUESTS_RETRY_DEFAULT = 10;
const std::chrono::milliseconds INITIAL_TIMEOUT_RETRY_DEFAULT = std::chrono::seconds{1};
const double TIMEOUT_BACKOFF_RETRY_DEFAULT = 2.0;

const std::map<size_t, std::string> tmMonthToRussianName
{{0, u8"января"},
 {1, u8"февраля"},
 {2, u8"марта"},
 {3, u8"апреля"},
 {4, u8"мая"},
 {5, u8"июня"},
 {6, u8"июля"},
 {7, u8"августа"},
 {8, u8"сентября"},
 {9, u8"октября"},
 {10, u8"ноября"},
 {11, u8"декабря"}
};

}

std::map<std::string, std::string> jsonHstoreToMessage(const Hstore& hstore)
{
    return json::Value::fromString(hstore);
}

RetryPolicy RetryPolicy::defaultPolicy()
{
    RetryPolicy policy;
    policy.maxAttempts = MAX_REQUESTS_RETRY_DEFAULT;
    policy.initialTimeout = INITIAL_TIMEOUT_RETRY_DEFAULT;
    policy.timeoutBackoff = TIMEOUT_BACKOFF_RETRY_DEFAULT;
    return policy;
}

SimpleDate SimpleDate::fromTimePoint(common::TimePoint tp)
{
    std::ostringstream dayStr, monthStr, yearStr;
    time_t timeTp = std::chrono::system_clock::to_time_t(tp);
    struct tm parts;
    REQUIRE(::localtime_r(&timeTp, &parts), "Cannot convert timepoint to tm struct");

    dayStr << parts.tm_mday;
    monthStr << tmMonthToRussianName.at(parts.tm_mon);
    yearStr << parts.tm_year + 1900;
    return {dayStr.str(), monthStr.str(), yearStr.str()};
}

// Sender can evaluate statistics for clicks
// but if and only if correspondent href tag in template contains http
// That's why we keep https:// in email templates and ensure here that no url has https:// prefix
std::string eraseSchema(const std::string& url)
{
    for (const std::string prefix: {"http://", "https://"}) {
        if (boost153::starts_with(url, prefix)) {
            return url.substr(prefix.size());
        }
    }
    return url;
}


NotificationEmailParams::NotificationEmailParams(std::map<std::string, std::string> dict)
: dict_(std::move(dict))
{
}

NotificationEmailParams::NotificationEmailParams(
    std::string blogUrl,
    std::string clubUrl,
    std::string feedbackUrl,
    std::string unsubscribeBaseUrl)
{
    dict_[BLOG_URL] = eraseSchema(std::move(blogUrl));
    dict_[CLUB_URL] = eraseSchema(std::move(clubUrl));
    dict_[FEEDBACK_URL] = eraseSchema(std::move(feedbackUrl));
    dict_[UNSUBSCRIBE_BASE_URL] = eraseSchema(std::move(unsubscribeBaseUrl));
}

NotificationEmailParams::NotificationEmailParams(const NotificationEmailParams& other)
: dict_(other.dict_)
{}

NotificationEmailParams& NotificationEmailParams::operator=(const NotificationEmailParams& other)
{
    if (&other != this) {
        dict_ = other.dict_;
    }
    return *this;
}

NotificationEmailParams NotificationEmailParams::fromJson(const std::string& hstore)
{
    NotificationEmailParams msg(
        static_cast<decltype(NotificationEmailParams::dict_)&&>(json::Value::fromString(hstore)));
    return msg;
}

Hstore NotificationEmailParams::hstore(const pqxx::transaction_base& work) const
{
    std::string result;
    for (const auto& attr : dict_) {
        if (!attr.first.empty() && !attr.second.empty()) {
            if (!result.empty()) {
                result.push_back(',');
            }
            result += (
                "['" + work.esc(attr.first) +
                "','" + work.esc(attr.second) + "']");
        }
    }
    if (result.empty()) {
        return Hstore();
    }
    return Hstore("hstore(ARRAY[" + result + "])");
}

NotificationEmailParams& NotificationEmailParams::addSince(const SimpleDate& since)
{
    dict_[SINCE + DAY] = since.day;
    dict_[SINCE + MONTH] = since.month;
    dict_[SINCE + YEAR] = since.year;
    return *this;
}

NotificationEmailParams& NotificationEmailParams::addTill(const SimpleDate& till)
{
    dict_[TILL + DAY] = till.day;
    dict_[TILL + MONTH] = till.month;
    dict_[TILL + YEAR] = till.year;
    return *this;
}

NotificationEmailParams& NotificationEmailParams::addUserName(const std::string& userName)
{
    dict_[USER_NAME] = userName;
    return *this;
}

NotificationEmailParams& NotificationEmailParams::addCorrectionsCount(size_t correctionsCount)
{
    dict_[CORRECTIONS_COUNT] = boost::lexical_cast<std::string>(correctionsCount);
    return *this;
}

NotificationEmailParams& NotificationEmailParams::addUid(revision::UserID uid)
{
    dict_[UID] = boost::lexical_cast<std::string>(uid);
    return *this;
}

std::map<std::string, std::string> NotificationEmailParams::toMap() const
{
    std::map<std::string, std::string> dictForSender = dict_;
    // replace (unsubscribe_base_url, uid) -> (unsubscribe_base_url + uid)
    dictForSender[UNSUBSCRIBE_URL] = dictForSender[UNSUBSCRIBE_BASE_URL] + dictForSender[UID];
    dictForSender.erase(dictForSender.find(UNSUBSCRIBE_BASE_URL));
    dictForSender.erase(dictForSender.find(UID));
    return dictForSender;
}


} // namespace releases_notification
} // namespace wiki
} // namespace maps