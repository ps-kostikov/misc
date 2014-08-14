#!/usr/bin/python

import sys

from yandex.maps import tileutils5
from yandex.maps import coordtrans


class CoordConverter:
    def __init__(self):
        self.__merc2geo = coordtrans.FMercatortoGeodetic()
        self.__geo2merc = coordtrans.Geodetic2Mercator()

    def get_lonlat(self, x, y):
        g_point = self.__merc2geo.Convert(coordtrans.GeorefPoint(x, y))
        lon = coordtrans.Degree(g_point.L(), 1).decimal()
        lat = coordtrans.Degree(g_point.B(), 1).decimal()
        return (lon, lat)

    def get_xy(self, l, b):
        lr = coordtrans.Degree(l).radians()
        br = coordtrans.Degree(b).radians()
        mercPoint = self.__geo2merc.Convert(coordtrans.GeodeticPoint(lr, br))
        return mercPoint.x(), mercPoint.y()


__coord_converter = CoordConverter()


def get_lonlat(x, y):
    return __coord_converter.get_lonlat(x, y)


def get_xy(l, b):
    return __coord_converter.get_xy(l, b)


def tile_from_path(path):
    return tileutils5.path2tile(path)


def tile_from_coords(x, y, z):
    tc = tileutils5.TileCoord(x, y)
    return tileutils5.Tile(tc, z)


def tile_from_lonlat(lon, lat, z):
    return tileutils5.path2tile(
            tileutils5.lonlatToPath(lon, lat, z))


def print_tile_info(tile):
    print '%s %s %s' % (tile.coord().x(), tile.coord().y(), tile.scale())
    geom = tile.realGeom()
    x, _, y, __ = geom.envelope()
    lon, lat = get_lonlat(x, y)
    print "lon, lat = ", lon, lat

if len(sys.argv) == 2:
    tile = tile_from_path(sys.argv[1])

elif sys.argv[1].find('.') != -1:
    lon, lat = map(float, sys.argv[1:3])
    z = int(sys.argv[3])
    tile = tile_from_lonlat(lon, lat, z)
else:
    x, y, z = map(int, sys.argv[1:])
    tile = tile_from_coords(x, y, z)

print_tile_info(tile)
