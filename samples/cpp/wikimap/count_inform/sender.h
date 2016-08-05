#pragma once

#include "common.h"

#include <yandex/maps/http2.h>

#include <boost153/optional.hpp>

#include <string>
#include <map>
#include <chrono>

namespace maps {
namespace wiki {
namespace releases_notification {

struct SenderConfiguration
{
    std::string host;
    boost153::optional<unsigned> port;
    std::chrono::milliseconds httpTimeout = std::chrono::milliseconds(0);

    std::string userId;
    std::string accountSlug;
    std::string campaignSlug;
};

class ISender
{
public:
    virtual void send(
        const std::string& toEmail,
        const std::map<std::string, std::string>& msg) = 0;

    virtual ~ISender() {}
};

class Sender: public ISender
{
public:
    Sender(
        SenderConfiguration configuration,
        RetryPolicy policy
        );

    void send(
        const std::string& toEmail,
        const std::map<std::string, std::string>& msg) override;

    virtual ~Sender() override {}

private:
    const SenderConfiguration configuration_;
    const HTTPRetriedPerformer<http2::FormRequest> httpPerformer_;
};

// shaken, not stirred
class DrySender: public ISender
{
public:
    DrySender();

    void send(
        const std::string& toEmail,
        const std::map<std::string, std::string>& msg) override;

    virtual ~DrySender() override {}
};

} // namespace releases_notification
} // namespace wiki
} // namespace maps