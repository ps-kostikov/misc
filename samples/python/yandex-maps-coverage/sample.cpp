#include <yandex/maps/coverage5/coverage.h>
#include <yandex/maps/tileutils5/tile.h>
#include <yandex/maps/geolib3/conversion.h>

#include <iostream>
#include <fstream>
#include <string>


namespace mc5 = maps::coverage5;
namespace mt5 = maps::tileutils5;
namespace mg3 = maps::geolib3;

std::ostream& operator<<(std::ostream& out, const mc5::Region& region)
{
    out << "id = " << region.id();
    out << "; meta data = " << region.metaData();
    return out;    
}

mg3::BoundingBox tileToBbox(const mt5::Tile& tile)
{
    auto realBox = tile.realBox();
    mg3::Point2 lt(
            realBox.lt().toMapCS().x(),
            realBox.lt().toMapCS().y());
    mg3::Point2 rb(
            realBox.rb().toMapCS().x(),
            realBox.rb().toMapCS().y());
    // std::cout << rb.x() << std::endl;
    // std::cout << mg3::Mercator2GeoPoint(rb).x() << std::endl;
    return mg3::BoundingBox(mg3::Mercator2GeoPoint(lt), mg3::Mercator2GeoPoint(rb));
}

int main()
{
    std::cout << "hello" << std::endl;
    mc5::Coverage coverage("out.mms.1");
    const auto& layer = coverage["map"];
    
    auto regions = layer.regions(maps::geolib3::Point2(37., 55.), 16);
    for (const auto& region: regions) {
        std::cout << region << std::endl;
    }
    std::cout << std::endl;

    mt5::Tile tile(19808, 10275, 15);
    // tileToBbox(tile);
    auto regions2 = layer.regions(tileToBbox(tile), tile.scale());
    for (const auto& region: regions2) {
        std::cout << region << std::endl;
    }
    std::cout << std::endl;

    return 0;
}
