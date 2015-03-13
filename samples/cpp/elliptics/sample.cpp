#include <yandex/maps/http/httprequest.h>
#include <yandex/maps/http/http_codes.h>
//#include <yandex/maps/pgpool3/pgpool3.h>

#include <boost153/optional.hpp>
#include <boost153/lexical_cast.hpp>

#include <iostream>
#include <string>
#include <vector>


//struct Path
//{
//    std::vector<std::string> items;
//};

struct Key
{
    std::string groupId;
    std::string path;
};


struct Configuration
{
    std::string host;
    std::string namespaceName;
    std::string authHeader;
    std::string pathPrefix;
};

struct Constants
{
    unsigned readPort = 80;
    unsigned writePort = 1111;
    unsigned httpTimeoutMs = 0;
};

class Elliptics
{
public:
    Elliptics(Configuration configuration, Constants constants):
        configuration_(std::move(configuration)),
        constants_(std::move(constants))
    {}

    Key post(const std::string& /*path*/, const std::string& /*data*/)
    {
        return Key{"", ""};
    }

    boost153::optional<std::string> get(const Key& key)
    {
        maps::http::Requester requester;
        std::string url = (
            "http://" +
            configuration_.host +
            ":" + boost153::lexical_cast<std::string>(constants_.readPort) +
            "/get-" +
            configuration_.namespaceName +
            "/" + key.groupId + "/" + configuration_.pathPrefix + key.path
        );
        std::cout << url << std::endl;
        const auto& request = requester.addGetRequest(url);
        requester.perform(constants_.httpTimeoutMs);
        const maps::http::Response& response = request.response();
        if (response.code() != maps::http::HTTPCodes::OK) {
            return boost153::none;
        }
        return response.data();
    }

    bool del(const Key& key)
    {
        maps::http::Requester requester;
        std::string url = (
            "http://" +
            configuration_.host +
            ":" + boost153::lexical_cast<std::string>(constants_.writePort) +
            "/delete-" +
            configuration_.namespaceName +
            "/" + key.groupId + "/" + configuration_.pathPrefix + key.path
        );
        std::cout << url << std::endl;
        auto& request = requester.addGetRequest(url);
        request.addHeader("Authorization: " + configuration_.authHeader);
        requester.perform(constants_.httpTimeoutMs);
        const maps::http::Response& response = request.response();
        if (response.code() != maps::http::HTTPCodes::OK) {
            return false;
        }
        return true;
    }

private:
    Configuration configuration_;
    Constants constants_;
};

int main()
{
    std::cout << "hello" << std::endl;
    std::string authHeader = "Basic YmFja2E6ZTcyYjhkZDQ2NjgyZjE0NmJiMGZmYmEwZmZiZDgzNjA=";
    Configuration configuration{"storage-int.mdst.yandex.net", "backa", authHeader, ""};
    Constants constants;
    Elliptics e(configuration, constants);
    Key key{"677", "pkostikov/test"};
    auto res = e.get(key);
    if (res) {
        std::cout << "OK; " << *res << std::endl;
    } else {
        std::cout << "FAIL;" << std::endl;
    }

    auto delRes = e.del(key);
    if (delRes) {
        std::cout << "DEL OK;" << std::endl;
    } else {
        std::cout << "DEL FAIL;" << std::endl;
    }


    return 0;
}
