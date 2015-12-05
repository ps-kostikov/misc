import collections
import shapely.wkt

import xml
import geometry


RegionInfo = collections.namedtuple("RegionInfo", "geom zmin zmax vendor map")


def generate_tileindex_cov4_ET(layer, region_infos):

    def region_ET(region_info):
        coverage_member_metadata = xml.CVR_EM.CoverageMemberMetaData(
            xml.CVR_EM.zoomRange({
                "min": str(region_info.zmin),
                "max": str(region_info.zmax)
            }))

        coverage_member_metadata.append(
            xml.CVR_EM.MetaData(
                xml.EM.vendor(region_info.vendor),
                xml.EM.map(region_info.map)
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


with open('russia.4268.wkt') as f:
    geom = shapely.wkt.loads(f.read())

layer_to_region_infos = {
    'map': [RegionInfo(
        geom=geom,
        zmin=0,
        zmax=18,
        vendor='yandex',
        map='map_russia_1111.xml'
    )],
    'skl': [RegionInfo(
        geom=geom,
        zmin=1,
        zmax=14,
        vendor='navteq',
        map='skl_europe_222.xml'
    )],
}

for layer, region_infos in layer_to_region_infos.iteritems():
    et = generate_tileindex_cov4_ET(layer, region_infos)

    with open("cov4/{0}.xml".format(layer), "w") as coverage_file:
        coverage_file.write(xml.et_to_string(et))
