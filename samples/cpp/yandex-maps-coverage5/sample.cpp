#include <yandex/maps/xml3/xml.h>
#include <yandex/maps/coverage5/coverage.h>
#include <yandex/maps/tileutils5/tile.h>
#include <yandex/maps/geolib3/bounding_box.h>
#include <yandex/maps/geolib3/transform.h>

#include <iostream>
#include <string>


maps::geolib3::BoundingBox geodeticBox(const maps::tileutils5::Tile& tile)
{
    auto mercatorBox = tile.mercatorBox();
    maps::geolib3::Point2 mercatorLt(mercatorBox.lt().x(), mercatorBox.lt().y());
    maps::geolib3::Point2 mercatorRb(mercatorBox.rb().x(), mercatorBox.rb().y());

    maps::geolib3::SimpleGeometryTransform2 transform(std::unique_ptr<maps::geolib3::GeodeticToMercatorTransform2>(
        new maps::geolib3::GeodeticToMercatorTransform2())
    );

    return maps::geolib3::BoundingBox(
        transform(mercatorLt, maps::geolib3::TransformDirection::Backward),
        transform(mercatorRb, maps::geolib3::TransformDirection::Backward)
    );
}

int main()
{
    maps::coverage5::Coverage coverage("data");
    auto& layer = coverage["map"];

    maps::tileutils5::Tile tile(maps::tileutils5::TileCoord(8, 5), 4);

    // auto regions = layer.regions(maps::geolib3::Point2(12.1615, 49.5701), 4);
    auto regions = layer.regions(geodeticBox(tile), tile.scale());
    for (auto& region: regions) {
        std::cout << region.id() << ": " << region.metaData() << std::endl;
        maps::xml3::Doc doc(region.metaData(), maps::xml3::Doc::String);

        doc.addNamespace("cvr", "http://maps.yandex.ru/coverage/2.x");

        std::cout << doc.node("//cvr:MetaData/compiled-map-path").value<std::string>() << std::endl;
    }
    return 0;
}
