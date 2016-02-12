#include "common.h"
#include "lru_cache.h"

#include <yandex/maps/mms/mmap.h>
#include <yandex/maps/mms/holder2.h>

#include <yandex/maps/yacare.h>

#include <boost/optional.hpp>

#include <map>
#include <iostream>
#include <fstream>


std::unique_ptr<mms::Holder2<Index<mms::Mmapped>>> gIndexHolder;
std::unique_ptr<std::ifstream> dataFPtr;
LruCache<Tile, std::string, TileHash> cache(1000);

std::string lookup(const Tile& tile)
{
    auto& tileindex = (*(*gIndexHolder)).tiles;
    auto it = tileindex.find(tile);
    if (it == tileindex.end()) {
        return {}; 
    }   
    auto& address = it->second;
    dataFPtr->seekg(address.offset);
    std::string result(address.size, ' ');
    dataFPtr->read(const_cast<char*>(result.data()), address.size);
    return result;
}

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
    auto result = cache.getOrEmplace(tile, lookup);
    if (result->size() == 0) {
        throw yacare::errors::NotFound();
    }
    response["Content-Type"] = "image/png";
    response << *result;
}

int main(int /*argc*/, const char** /*argv*/)
{
    gIndexHolder.reset(new mms::Holder2<Index<mms::Mmapped>>("index.mms", MAP_SHARED));
    std::cout << "Total tiles = " << (*(*gIndexHolder)).tiles.size() << std::endl;
    /*
    std::ifstream indexF("layer.index");
    for (std::string line; std::getline(indexF, line); ) {
        std::stringstream ss(line);
        size_t x, y, z;
        unsigned long offset;
        size_t size;
        ss >> x >> y >> z >> offset >> size;
        tileindex[Tile{x, y, z}] = Address{offset, size};
    }
    std::cout << "number of tiles = " << tileindex.size() << std::endl;
*/
    dataFPtr.reset(new std::ifstream("layer.data"));

    INFO() << "Initializing";
    yacare::run();
    INFO() << "Shutting down";
    return 0;
}
