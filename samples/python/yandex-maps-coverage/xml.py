'''Coverage related functions for xml generation'''
from ytools.xml import ET, DashElementMaker, TYPE_SERIALIZERS, EM
from yandex.maps.utils.common import require

from shapely.geometry import Polygon
from yandex.maps import rtree
from yandex.maps import tileutils5 as tu

# pkostikov@ FIXME: bad dependence.
import geometry


NSMAP = dict(gml="http://www.opengis.net/gml",
             atom="http://www.w3.org/2005/Atom",
             cvr="http://maps.yandex.ru/coverage/2.x",
             xsi="http://www.w3.org/2001/XMLSchema-instance",
             ym="http://maps.yandex.ru/ymaps/1.x")

GML_EM = DashElementMaker(namespace=NSMAP['gml'], nsmap=NSMAP,
        typemap=TYPE_SERIALIZERS)
ATOM_EM = DashElementMaker(namespace=NSMAP['atom'], nsmap=NSMAP,
        typemap=TYPE_SERIALIZERS)
CVR_EM = DashElementMaker(namespace=NSMAP['cvr'], nsmap=NSMAP,
        typemap=TYPE_SERIALIZERS)
YM_EM = DashElementMaker(namespace=NSMAP['ym'], nsmap=NSMAP,
        typemap=TYPE_SERIALIZERS)

GML_EM = DashElementMaker(namespace=NSMAP['gml'], nsmap=NSMAP,
                          typemap=TYPE_SERIALIZERS)


SIGNIFICANT_ZOOM = 16
TILE_SIZE = 256
GEOM_SIMPLIFACTION_SIZE = tu.scale2resolution(SIGNIFICANT_ZOOM) * TILE_SIZE
MIN_SIGNIFICANT_AREA_MERC = GEOM_SIMPLIFACTION_SIZE ** 2


def ensure_geom_valid(geom):
    if geom.is_valid:
        return geom
    # in some cases helps to make geometry valid
    geom = geom.buffer(0)
    if not geom.is_valid:
        raise Exception("invalid geometry {0!r}".format(geom.wkt))
    return geom


def is_significant(geom):
    return geom.area > MIN_SIGNIFICANT_AREA_MERC


def transform_polygon(geom):
    require(geom.type == 'Polygon',
            Exception("Geom should be polygon but {0!r} passed".format(geom.type)))
    if not is_significant(geom):
        return None

    geom = geom.simplify(GEOM_SIMPLIFACTION_SIZE)

    geom = Polygon(geom.exterior,
                   [g for g in geom.interiors
                    if is_significant(Polygon(g.coords))])
    geom = geometry.geom_merc_to_geo(geom)
    return ensure_geom_valid(geom)


def linearring_to_gml(geom):
    return GML_EM.LinearRing(GML_EM.posList(" ".join("%.20f %.20f" % (x, y)
                            for x, y in geom.coords)))


def polygon_to_gml(geom):
    return GML_EM.Polygon(
            GML_EM.exterior(linearring_to_gml(geom.exterior)),
            *[GML_EM.interior(linearring_to_gml(g)) for g in geom.interiors])


def geom_to_gml(geom):
    if geom.geom_type == 'Polygon':
        return polygon_to_gml(geom)
    elif geom.geom_type == 'MultiPolygon':
        return GML_EM.MultiGeometry(
                *[GML_EM.geometryMember(polygon_to_gml(g)) for g in geom.geoms])
    else:
        raise Exception("Unsupported geometry: {0}".format(geom.geom_type))


def region_vendors_ETs(region, vendors_dict):
    yield ATOM_EM.author(
            *[ATOM_EM.name(vendors_dict[vendor_id])
               for vendor_id in region.vendor_ids])


def region_ET(cinfo, geom, vendors_dict, scalable):
    coverage_member_metadata = CVR_EM.CoverageMemberMetaData(
        CVR_EM.zoomRange({"min": str(cinfo.zmin),
                          "max": str(cinfo.zmax)}))
    if scalable:
        coverage_member_metadata.append(CVR_EM.MetaData(EM.scaled("true")))

    for region_vendor_ET in region_vendors_ETs(cinfo, vendors_dict):
        coverage_member_metadata.append(region_vendor_ET)

    return YM_EM.GeoObject(
            GML_EM.metaDataProperty(coverage_member_metadata),
            geom_to_gml(geom))


def generate_coverage4_ET(layer, coverages, vendors_dict, release, scalable, has_hotspots):

    def process_polygon(feature_members, geom):
        geom = transform_polygon(geom)
        if geom:
            feature_members.append(region_ET(cinfo, geom, vendors_dict, scalable))

    ymaps = YM_EM.ymaps()
    feature_members = GML_EM.featureMembers()
    for cinfo in coverages:
        if cinfo.geom.type == 'MultiPolygon':
            map(lambda g: process_polygon(feature_members, g),
                cinfo.geom.geoms)
        else:
            process_polygon(feature_members, cinfo.geom)

    layer_meta_data = \
            CVR_EM.CoverageMetaData(CVR_EM.id(layer), CVR_EM.version(release))

    cvr_meta_data = CVR_EM.MetaData()

    if scalable:
        cvr_meta_data.append(EM.scaled("true"))

    if has_hotspots:
        zoom_range = {"min": "14", "max": "23"}
        cvr_meta_data.append(EM.hotspotZoomRange(zoom_range))

    layer_meta_data.append(cvr_meta_data)

    ymaps.append(
        YM_EM.GeoObjectCollection(
            GML_EM.metaDataProperty(layer_meta_data),
            feature_members))

    return ymaps


def get_bounds_for_rtree(geom):
    bounds = geom.bounds
    minx, miny = geometry.get_lonlat(bounds[0], bounds[1])
    maxx, maxy = geometry.get_lonlat(bounds[2], bounds[3])
    return ((minx, miny), (maxx, maxy))


def build_rtree(cinfos):
    bounds = []
    for cinfo in cinfos:
        for ring in geometry.geometry_rings(cinfo.geom):
            bounds_for_rtree = get_bounds_for_rtree(ring)
            bounds.append(rtree.Leaf(bounds_for_rtree, cinfo))

    builder = rtree.Builder(2, 2)
    tree = builder.buildTree(bounds)
    return tree


def node_ET(node):
    if node is None:
        return ""

    n = EM.n(
        EM.min("%s,%s" % tuple(node.envelope().min)),
        EM.max("%s,%s" % tuple(node.envelope().max)))

    for child in node.childs:
        if child is not None:
            n.append(node_ET(child))

    for leaf in node.leaves:
        data = leaf.data()
        envelope = leaf.envelope()
        leaf_n = EM.n(
            EM.name(),
            EM.geoid(),
            EM.min("%s,%s" % tuple(leaf.envelope().min)),
            EM.max("%s,%s" % tuple(leaf.envelope().max)),
            EM.area("%.3f" % (
                    (envelope.max[0] - envelope.min[0]) *
                    (envelope.max[1] - envelope.min[1]))),
            {"smin": str(data.zmin), "smax": str(data.zmax)})

        for vendor in data.vendor_ids:
            leaf_n.append(EM.vid(vendor))

        n.append(leaf_n)

    return n


def coverage3_ET(layer, cov_rtree, version):
    attrib = dict(xmlns="http://maps.yandex.ru/data/1.0")
    root = ET.Element('coverage', attrib=attrib)
    root.append(EM.layer(node_ET(cov_rtree), type=layer))
    return root


def et_to_string(et):
    return ET.tostring(et, pretty_print=True,
            xml_declaration=True, encoding='utf-8')
