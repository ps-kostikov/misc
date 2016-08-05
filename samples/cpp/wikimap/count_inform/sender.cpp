#include "sender.h"

#include <yandex/maps/log8.h>
#include <yandex/maps/json/builder.h>
#include <yandex/maps/json/std.h>
#include <yandex/maps/ssl/ctx.h>
#include <yandex/maps/common/encoder.h>

#include <boost153/format.hpp>
#include <boost153/lexical_cast.hpp>

#include <openssl/evp.h>

#include <thread>

namespace maps {
namespace wiki {
namespace releases_notification {

Sender::Sender(SenderConfiguration configuration, RetryPolicy policy)
    : configuration_(std::move(configuration))
    , httpPerformer_(HTTPRetriedPerformer<http2::FormRequest>(std::move(policy)))
{
    // FIXME pkostikov: temporary openssl0.9.8 hack
    EVP_add_digest(EVP_sha256());
    EVP_add_digest(EVP_sha512());
}

DrySender::DrySender()
{}

namespace {

std::string userToAuthHeader(const std::string& userId)
{
    // empty password required
    std::string toEncode = userId + ":";
    std::string encoded;
    ytl::base64::code(toEncode.begin(), toEncode.end(), encoded);
    return "Basic " + encoded;
}

} // namespace

void
Sender::send(
    const Email& toEmail,
    const std::map<std::string, std::string>& msg)
{
    http2::Client httpClient;
    // FIXME pkostikov: temporary openssl0.9.8 hack
    httpClient.sslContext().setCiphers("AES128-SHA");
    httpClient.setConnectMethod(
        http2::connect_methods::withTimeout(configuration_.httpTimeout));
    http2::URL url = (boost153::format("https://%1%%2%/api/0/%3%/transactional/%4%/send?to_email=%5%")
        % configuration_.host
        % (configuration_.port ? ":" + std::to_string(*configuration_.port) : "")
        % configuration_.accountSlug
        % configuration_.campaignSlug
        % toEmail
    ).str();
    http2::FormRequest request(httpClient, "POST", url);

    request.addHeader("Authorization", userToAuthHeader(configuration_.userId));

    request.addParam("args", (json::Builder() << msg).str());
    request.addParam("async", "false");

    auto response = httpPerformer_.perform(request, url);

    if (response.status() != HTTP_STATUS_OK) {
        throw maps::RuntimeError() << "unexpected http code: " << response.status()
            << "; POST url: " << url;
    }
}

void
DrySender::send(
    const Email& toEmail,
    const std::map<std::string, std::string>& msg)
{
    INFO() << "imitate sender request with email = " << toEmail
           << " and args = " << (json::Builder() << msg).str();
}


} // namespace releases_notification
} // namespace wiki
} // namespace maps