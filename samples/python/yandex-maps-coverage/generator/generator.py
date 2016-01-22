import collections
import shapely.wkt

from yandex.maps.factory.tile import Tile

import xml
import geometry


RegionInfo = collections.namedtuple("RegionInfo", "geom zmin zmax vendor region map")

def generator(n):
    for i in range(1, n):
        yield i

id_gen = generator(10000)


def generate_tileindex_cov4_ET(layer, region_infos):

    def region_ET(region_info):
        coverage_member_metadata = xml.CVR_EM.CoverageMemberMetaData(
            xml.CVR_EM.id(id_gen.next()),
            xml.CVR_EM.zoomRange({
                "min": str(region_info.zmin),
                "max": str(region_info.zmax)
            }))

        coverage_member_metadata.append(
            xml.CVR_EM.MetaData(
                xml.EM.vendor(region_info.vendor),
                xml.EM.compiled_map_path(region_info.map),
                xml.EM.region_name(region_info.region)
            )
        )

        return xml.YM_EM.GeoObject(
                xml.GML_EM.metaDataProperty(coverage_member_metadata),
                xml.geom_to_gml(geometry.geom_merc_to_geo(region_info.geom)))


    ymaps = xml.YM_EM.ymaps()

    feature_members = xml.GML_EM.featureMembers()
    for region_info in region_infos:
        feature_members.append(region_ET(region_info))

    layer_meta_data = xml.CVR_EM.CoverageMetaData(xml.CVR_EM.id(layer))

    cvr_meta_data = xml.CVR_EM.MetaData()

    layer_meta_data.append(cvr_meta_data)

    ymaps.append(
        xml.YM_EM.GeoObjectCollection(
            xml.GML_EM.metaDataProperty(layer_meta_data),
            feature_members))

    return ymaps


layer_to_region_infos = {
    'map': [
        RegionInfo(
            geom=Tile(0, 0, 1).geom,
            zmin=0,
            zmax=2,
            vendor='yandex',
            region='russia',
            map='map_russia.xml'),
        RegionInfo(
            geom=Tile(1, 1, 1).geom,
            zmin=0,
            zmax=2,
            vendor='navteq',
            region='europe',
            map='map_europe.xml'),

    ],
    'vnvmap': [
        RegionInfo(
            geom=Tile(0, 0, 2).geom,
            zmin=1,
            zmax=2,
            region='russia',
            vendor='yandex',
            map='vnvmap_russia.xml'),
        RegionInfo(
            geom=Tile(1, 0, 2).geom,
            zmin=1,
            zmax=2,
            region='ukraine',
            vendor='yandex',
            map='vnvmap_ukraine.xml'),
    ],
    'vnvmapnt': [
        RegionInfo(
            geom=Tile(0, 0, 2).geom,
            zmin=1,
            zmax=2,
            region='russia',
            vendor='yandex',
            map='vnvmapnt_russia.xml'),
        RegionInfo(
            geom=Tile(1, 0, 2).geom,
            zmin=1,
            zmax=2,
            region='ukraine',
            vendor='yandex',
            map='vnvmapnt_ukraine.xml'),
    ],

}

for layer, region_infos in layer_to_region_infos.iteritems():
    et = generate_tileindex_cov4_ET(layer, region_infos)

    with open("cov4/{0}.xml".format(layer), "w") as coverage_file:
        coverage_file.write(xml.et_to_string(et))
