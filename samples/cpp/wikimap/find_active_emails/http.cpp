#include "http.h"

namespace maps {
namespace wiki {
namespace releases_notification {

namespace {

const size_t MAX_REQUESTS_RETRY_DEFAULT = 10;
const std::chrono::milliseconds INITIAL_TIMEOUT_RETRY_DEFAULT = std::chrono::seconds{1};
const double TIMEOUT_BACKOFF_RETRY_DEFAULT = 2.0;

} // namespace

RetryPolicy RetryPolicy::defaultPolicy()
{
    RetryPolicy policy;
    policy.maxAttempts = MAX_REQUESTS_RETRY_DEFAULT;
    policy.initialTimeout = INITIAL_TIMEOUT_RETRY_DEFAULT;
    policy.timeoutBackoff = TIMEOUT_BACKOFF_RETRY_DEFAULT;
    return policy;
}

} // namespace releases_notification
} // namespace wiki
} // namespace maps