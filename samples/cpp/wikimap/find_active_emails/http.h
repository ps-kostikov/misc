#pragma once

#include <yandex/maps/http2.h>
#include <yandex/maps/log8.h>

#include <chrono>
#include <map>
#include <thread>

namespace maps {
namespace wiki {
namespace releases_notification {

constexpr int HTTP_STATUS_OK = 200;
constexpr int HTTP_STATUS_SERVER_ERROR = 500;

struct RetryPolicy
{
    size_t maxAttempts;
    std::chrono::milliseconds initialTimeout;
    double timeoutBackoff;

    static RetryPolicy defaultPolicy();
};

class HTTPRetriedPerformer
{
public:
    explicit HTTPRetriedPerformer(RetryPolicy policy)
    : policy_(std::move(policy))
    {}

    http2::Response perform(std::function<http2::Response(void)> doRequest, const http2::URL& url) const
    {
        std::ostringstream errors;
        auto retryInterval = policy_.initialTimeout;
        for (size_t retryNumber = 1; retryNumber <= policy_.maxAttempts; ++retryNumber) {
            try {
                auto response = doRequest();
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


} // namespace releases_notification
} // namespace wiki
} // namespace maps