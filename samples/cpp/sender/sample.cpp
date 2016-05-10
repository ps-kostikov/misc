#include <yandex/maps/http2.h>
#include <yandex/maps/ssl/ctx.h>
#include <yandex/maps/common/encoder.h>

#include <openssl/evp.h>

#include <string>

// resp = requests.post(
//     'https://test.sender.yandex-team.ru/api/0/nmap/transactional/0EDQ2I02-JPA/send?to_email=pkostikov@yandex-team.ru',
//     auth=('c5d5c3b85ee6484fbdabd5b04324e545', ''),
//     data={
//         'async': 'false'
//     },
//     verify=False
// )

// curl -vs -X POST     -u c5d5c3b85ee6484fbdabd5b04324e545:  -d async=false     'https://test.sender.yandex-team.ru/api/0/nmap/transactional/0EDQ2I02-JPA/send?to_email=pkostikov@yandex-team.ru'

int main()
{

    EVP_add_digest(EVP_sha256());
    EVP_add_digest(EVP_sha512());

    maps::http2::Client httpClient;

    // httpClient.sslContext().enableProtocols(ssl::Context::Protocol::TLSv10);
    // httpClient.sslContext().enableProtocols(ssl::Context::Protocol::SSLv2);
    // httpClient.sslContext().setCiphers("AES128-SHA");
    // httpClient.sslContext().setCiphers("AES256-SHA");
    // httpClient.sslContext().setCiphers("DHE-RSA-AES256-SHA");

    httpClient.sslContext().setCiphers("AES128-SHA");
    maps::http2::URL url("https://test.sender.yandex-team.ru/api/0/nmap/transactional/0EDQ2I02-JPA/send?to_email=pkostikov@yandex-team.ru");
    maps::http2::FormRequest request(httpClient, "POST", url);

    std::string userName = "c5d5c3b85ee6484fbdabd5b04324e545";
    std::string toEncode = userName + ":";
    std::string encoded;
    ytl::base64::code(toEncode.begin(), toEncode.end(), encoded);
    std::string authHeaderValue = "Basic " + encoded;
    std::cout << authHeaderValue << std::endl;
    request.addHeader("Authorization", authHeaderValue);

    request.addParam("args", "{\"name\": \"Pavel\"}");
    request.addParam("async", "false");

    auto response = request.perform();
    std::cout << "status: " << response.status() << std::endl;
    return 0;
}

