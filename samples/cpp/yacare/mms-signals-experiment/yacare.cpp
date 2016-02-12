#include "common.h"

#include <yandex/maps/yacare.h>

#include <yandex/maps/mms/holder2.h>
#include <yandex/maps/mms/mmap.h>

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
    auto xOpt = getOptionalParam<uint64_t>(request, "x");
    auto yOpt = getOptionalParam<uint64_t>(request, "y");
    auto zOpt = getOptionalParam<uint64_t>(request, "z");
    if (not xOpt or not yOpt or not zOpt) {
        throw yacare::errors::BadRequest();
    }

    Tile tile{*xOpt, *yOpt, *zOpt};
    auto& tiles = (*(*gLayerHolder)).tiles;
    auto it = tiles.find(tile);
    if (it == tiles.end()) {
        throw yacare::errors::NotFound();
    }
    response["Content-Type"] = "image/png";
    response << it->second;
}

int main(int /*argc*/, const char** /*argv*/)
{
    // mms::Mmap mmap("layer.mms", MAP_PRIVATE);
    // gLayerHolder.reset(new mms::Holder2<Layer<mms::Mmapped>>("layer.mms", MAP_PRIVATE | MAP_POPULATE));
    // gLayerHolder.reset(new mms::Holder2<Layer<mms::Mmapped>>("layer.mms", MAP_PRIVATE));
    gLayerHolder.reset(new mms::Holder2<Layer<mms::Mmapped>>("layer.mms", MAP_SHARED));
    
    std::cout << "Total tiles loaded = " << (*(*gLayerHolder)).tiles.size() << std::endl;
    // std::ifstream f(argv[1]);
    // std::string data((std::istreambuf_iterator<char>(f)),
    //                  std::istreambuf_iterator<char>());
    // auto sdfFont = mms::safeCast<Layer<mms::Mmapped>>(data.data(), data.size());

    INFO() << "Initializing";
    yacare::run();
    INFO() << "Shutting down";
    return 0;
}
