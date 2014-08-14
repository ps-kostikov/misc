#include <yandex/maps/yacare.h>

#include <boost153/filesystem.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <iostream>
#include <fstream>

namespace fs = boost153::filesystem;

YCR_RESPOND_TO("sample:/*")
{
    std::string pathInfo = request.env("PATH_INFO");
    boost::replace_all(pathInfo, "/", "_");
    fs::path path = fs::path("content") / fs::path(pathInfo);
    if (fs::exists(path)) { 
        std::ifstream t(path.string());
        std::string json((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
        response << json;
        response["Content-Type"] = "application/json";
    } else {
        throw yacare::errors::NotFound() << "not found";
    }
}


int main(int /*argc*/, const char** /*argv*/)
{
    INFO() << "Initializing";
    yacare::run();
    INFO() << "Shutting down";
    return 0;
}
