#include "common.h"

#include <yandex/maps/yacare.h>

#include <yandex/maps/mms/holder2.h>

#include <boost/optional.hpp>

#include <iostream>


std::unique_ptr<mms::Holder2<Layer<mms::Mmapped>>> gLayerHolder;

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


YCR_RESPOND_TO("sample:/tiles")
{
    auto x = *getOptionalParam<uint64_t>(request, "x");
    auto y = *getOptionalParam<uint64_t>(request, "y");
    auto z = *getOptionalParam<uint64_t>(request, "z");

    response << (*(*gLayerHolder)).tiles[Tile{x, y, z}];
    response["Content-Type"] = "image/png";
}

int main(int /*argc*/, const char** /*argv*/)
{
    gLayerHolder.reset(new mms::Holder2<Layer<mms::Mmapped>>("layer.mms"));
    
    // std::ifstream f(argv[1]);
    // std::string data((std::istreambuf_iterator<char>(f)),
    //                  std::istreambuf_iterator<char>());
    // auto sdfFont = mms::safeCast<Layer<mms::Mmapped>>(data.data(), data.size());

    INFO() << "Initializing";
    yacare::run();
    INFO() << "Shutting down";
    return 0;
}
