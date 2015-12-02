# -*- coding: utf-8 -*-
'''Functions related to geometry
'''

from shapely import geometry as shapely_geometry

from yandex.maps import coordtrans


MERC2GEO = coordtrans.FMercatortoGeodetic()
GEO2MERC = coordtrans.Geodetic2Mercator()


def get_lonlat(x, y):
    g_point = MERC2GEO.Convert(coordtrans.GeorefPoint(x, y))
    lon = coordtrans.Degree(g_point.L(), 1).decimal()
    lat = coordtrans.Degree(g_point.B(), 1).decimal()
    return (lon, lat)


def get_xy(l, b):
    lr = coordtrans.Degree(l).radians()
    br = coordtrans.Degree(b).radians()
    mercPoint = GEO2MERC.Convert(coordtrans.GeodeticPoint(lr, br))
    return mercPoint.x(), mercPoint.y()


def geom_merc_to_geo(geom):
    if geom.geom_type == 'LinearRing':
        return shapely_geometry.polygon.LinearRing(
                [get_lonlat(*coord) for coord in geom.coords])
    if geom.geom_type == 'Polygon':
        return shapely_geometry.Polygon(geom_merc_to_geo(geom.exterior),
                                    map(geom_merc_to_geo, geom.interiors))
    if geom.geom_type == 'MultiPolygon':
        return shapely_geometry.MultiPolygon(map(geom_merc_to_geo, geom))


def get_lonlat_centre(x, y):
    lon, lat = get_lonlat(x, y)
    if lon > 180:
        lon = lon - 360
    if lon < -180:
        lon = lon + 360
    return (lon, lat)


def adjust_geometry(geometry):
    if geometry.type != 'LinearRing':
        raise Exception("logic error - only rings expected here")
    lb_coords = [get_lonlat_centre(*coord) for coord in geometry.coords]
    merc_coords = [get_xy(*coord) for coord in lb_coords]
    return shapely_geometry.polygon.LinearRing(merc_coords)


def iter_geometry_rings(geometry):
    # DO NOT FORGET! Use interiors when move on libcoverage5
    if geometry.type == 'Polygon':
        yield adjust_geometry(geometry.exterior)
    elif geometry.type == 'MultiPolygon':
        for geom in geometry.geoms:
            yield adjust_geometry(geom.exterior)
    else:
        raise Exception("geometry type <%s> is not supported", geometry.type)


def make_earth_polygon(mlon=180):
    mlat = 89
    coords = [
            (-mlon, -mlat),
            (-mlon, mlat),
            (mlon, mlat),
            (mlon, -mlat),
            (-mlon, -mlat)]
    merc_coords = [get_xy(*coord) for coord in coords]
    return shapely_geometry.Polygon(merc_coords)


def split_by_180(geometry):
    # libcoverage{3,4} cannot work with longitudes outside (-180,180) interval.
    # As a workaround, we split the geometry into the part inside earth_polygon
    # (most geometries are contained in it) and the part outside earth_polygon.
    # Further, we process these parts separately.
    # Longitudes are coerced into (-180, 180) interval inside
    # adjust_geometry().
    rest_part = geometry.difference(make_earth_polygon_large())
    main_part = geometry.intersection(make_earth_polygon_small())

    if not rest_part.is_empty:
        yield rest_part
    if not main_part.is_empty:
        yield main_part


MARGIN = 0.000001


def make_earth_polygon_small():
    return make_earth_polygon(180 - MARGIN)


def make_earth_polygon_large():
    return make_earth_polygon(180 + MARGIN)


def geometry_rings(geometry):
    for part in split_by_180(geometry):
        for ring in iter_geometry_rings(part):
            yield ring
