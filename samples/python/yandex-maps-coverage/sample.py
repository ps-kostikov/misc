import collections
import shapely.wkt

import xml
import geometry


Map = collections.namedtuple("Map", "geom zmin zmax vendor map")


def region_ET(map_, geom):
    coverage_member_metadata = xml.CVR_EM.CoverageMemberMetaData(
        xml.CVR_EM.zoomRange({"min": str(map_.zmin),
                          "max": str(map_.zmax)}))
    # if scalable:
    #     coverage_member_metadata.append(CVR_EM.MetaData(EM.scaled("true")))

    coverage_member_metadata.append(
        xml.CVR_EM.MetaData(
            xml.EM.vendor(map_.vendor),
            xml.EM.map(map_.map)
        )
    )

    # for region_vendor_ET in region_vendors_ETs(cinfo, vendors_dict):
    #     coverage_member_metadata.append(region_vendor_ET)

    return xml.YM_EM.GeoObject(
            xml.GML_EM.metaDataProperty(coverage_member_metadata),
            xml.geom_to_gml(geom))


def generate_tileindex_cov4_ET(layer, maps):

    # def process_polygon(feature_members, map_, geom):
    #     geom = xml.transform_polygon(geom)
    #     if geom:
    #         feature_members.append(region_ET(map_, geom))

    ymaps = xml.YM_EM.ymaps()
    feature_members = xml.GML_EM.featureMembers()
    for map_ in maps:
        feature_members.append(region_ET(map_, geometry.geom_merc_to_geo(geom)))


        # if map_.geom.type == 'MultiPolygon':
        #     map(lambda g: process_polygon(feature_members, map_, g),
        #         map_.geom.geoms)
        # else:
        #     process_polygon(feature_members, map_, map_.geom)

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

et = generate_tileindex_cov4_ET(
    'map',
    [Map(
        geom=geom,
        zmin=0,
        zmax=18,
        vendor='yandex',
        map='map_russia_1111.xml'
    )]
)

with open("out.xml", "w") as coverage_file:
    coverage_file.write(xml.et_to_string(et))
