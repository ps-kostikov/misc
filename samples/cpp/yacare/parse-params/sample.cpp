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

template <class T>
T
getParam(const yacare::Request& r, const std::string& name)
{
    auto value = getOptionalParam<T>(r, name);
    if (!value) {
        throw yacare::errors::BadRequest() << " Missing parameter: " << name;
    }
    return *value;
}


// YCR_QUERY_PARAM(c_name, std::string);
// YCR_QUERY_PARAM(c_id, uint64_t);

// YCR_RESPOND_TO("sample:/company", c_name)
// {
//     auto c_opt = getOptionalParam<uint64_t>(request, "c_opt");
//     mj::Builder builder;
//     builder << [&](mj::ObjectBuilder builder) {
//         if (c_opt) {
//             builder["c_opt"] = *c_opt;
//         }
//         builder["c_name"] = c_name;
//     };
//     response << builder.str();
//     response["Content-Type"] = "application/json";
// }

// YCR_RESPOND_TO("sample:/vendor/$/info")
// {
//     std::string vendor = argv[0];

//     mj::Builder builder;
//     builder << [&](mj::ObjectBuilder builder) {
//         builder["vendor"] = vendor;
//     };
//     response << builder.str();
//     response["Content-Type"] = "application/json";
// }


YCR_QUERY_CUSTOM_PARAM((), taskId, boost::optional<int>)
{
    dest = getOptionalParam<int>(request, "task-id");
    return true;
}


YCR_RESPOND_TO("sample:/handler", taskId)
{
    // response << taskId;
    // auto taskId = getOptionalParam<int>(request, "task-id");
    if (taskId) {
        response << *taskId;
    } else {
        response << "no task-id";
    }
}


int main(int /*argc*/, const char** /*argv*/)
{
    INFO() << "Initializing";
    yacare::run();
    INFO() << "Shutting down";
    return 0;
}
