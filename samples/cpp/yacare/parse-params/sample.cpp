#include <yandex/maps/yacare.h>
#include <yandex/maps/json/builder.h>

#include <boost/optional.hpp>

#include <string>
#include <iostream>

namespace mj = maps::json;


template <class T>
boost::optional<T>
getOptionalParam(const yacare::Request& r, const std::string& name)
{
    std::string strParam = r.input()[name];
    if (strParam.empty()) {
        return boost::none;
    }
    try {
        return boost::lexical_cast<T>(strParam);
    } catch (const boost::bad_lexical_cast&) {
        throw yacare::errors::BadRequest() << " Invalid parameter: " << name << " : " << strParam;
    }
}


YCR_QUERY_PARAM(c_name, std::string);
YCR_QUERY_PARAM(c_id, uint64_t);

YCR_RESPOND_TO("sample:/company", c_name)
{
    auto c_opt = getOptionalParam<uint64_t>(request, "c_opt");
    mj::Builder builder;
    builder << [&](mj::ObjectBuilder builder) {
        if (c_opt) {
            builder["c_opt"] = *c_opt;
        }
        builder["c_name"] = c_name;
    };
    response << builder.str();
    response["Content-Type"] = "application/json";
}

int main(int /*argc*/, const char** /*argv*/)
{
    INFO() << "Initializing";
    yacare::run();
    INFO() << "Shutting down";
    return 0;
}
