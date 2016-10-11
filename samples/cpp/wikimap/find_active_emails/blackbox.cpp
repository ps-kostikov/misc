#include "blackbox.h"

#include <yandex/maps/common/exception.h>
#include <yandex/maps/http2.h>
#include <yandex/maps/log8.h>

#include <thread>

#include <boost153/lexical_cast.hpp>
#include <boost153/format.hpp>

namespace chrono = std::chrono;

namespace maps {
namespace wiki {
namespace releases_notification {

namespace {

const std::string USER_IP_BLACKBOX = "127.0.0.1";

}

BlackboxConfiguration::BlackboxConfiguration(
    const std::string& host,
    const RetryPolicy& retryPolicy,
    const std::chrono::milliseconds& httpTimeout)
: host_(host)
, retryPolicy_(retryPolicy)
, httpTimeout_(httpTimeout)
{
}

const std::string& BlackboxConfiguration::host() const
{
    return host_;
}

const RetryPolicy& BlackboxConfiguration::retryPolicy() const
{
    return retryPolicy_;
}

const std::chrono::milliseconds BlackboxConfiguration::httpTimeout() const
{
    return httpTimeout_;
}

Blackbox::Blackbox(const BlackboxConfiguration& config)
: configuration_(config)
, httpPerformer_(HTTPRetriedPerformer(config.retryPolicy()))
{
}

std::unique_ptr<bb::Response> Blackbox::performInfoRequest(const std::string& infoRequest) const
{

    http2::URL url = (boost153::format("http://%1%/blackbox?%2%")
        % configuration_.host()
        % infoRequest).str();

    auto doRequest = [&] () {
        http2::Client httpClient;
        if (configuration_.httpTimeout() > chrono::milliseconds::zero()) {
            httpClient.setConnectMethod(
                http2::connect_methods::withTimeout(configuration_.httpTimeout()));
        }

        http2::Request req(httpClient, "GET", url);
        return req.perform();
    };

    auto response = httpPerformer_.perform(doRequest, url);
    if (response.status() != HTTP_STATUS_OK) {
        throw maps::RuntimeError() << "unexpected http code: "
            << response.status() << "; GET url: " << url;
    }
    std::ostringstream oss;
    oss << response.body().rdbuf();

    std::string rawXml = oss.str();

    return std::unique_ptr<bb::Response>(bb::InfoResponse(rawXml).release());
}

bool Blackbox::isEmailValid(const Email& email, const UID& uid) const
{
    bb::Options opts;
    opts << bb::OptTestEmail(email);
    auto infoRequest = bb::InfoRequest(
        boost153::lexical_cast<std::string>(uid),
        USER_IP_BLACKBOX,
        opts);

    const auto& response = performInfoRequest(infoRequest);
    bb::EmailList emailList(response.get());
    return not emailList.empty();
}

boost153::optional<UserSocialInfo> Blackbox::userInfo(const UID& uid) const
{
    bb::Options opts;
    opts << bb::optGetDefaultEmail;
    opts << bb::optRegname;
    auto infoRequest = bb::InfoRequest(
        boost153::lexical_cast<std::string>(uid),
        USER_IP_BLACKBOX,
        opts);

    const auto& response = performInfoRequest(infoRequest);
    bb::EmailList emailList(response.get());
    bb::DisplayNameInfo displayName(response.get());

    auto defaultItem = emailList.getDefaultItem();
    boost153::optional<std::string> username;
    if (not displayName.name().empty()) {
        username = displayName.name();
    }
    if (not defaultItem) {
        return boost153::none;
    }
    return UserSocialInfo{defaultItem->address(), username, uid};
}

} // namespace releases_notification
} // namespace wiki
} // namespace maps