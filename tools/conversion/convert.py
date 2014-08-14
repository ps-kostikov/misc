#!/usr/bin/python

import sys

from yandex.maps import coordtrans

__all__ = ['get_lonlat', 'get_xy']


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


if __name__ == '__main__':
    ll_max = 361

    arg1, arg2 = map(float, sys.argv[1:3])
    if arg1 > ll_max and arg2 > ll_max:
        # consider that mercator passed
        lon, lat = get_lonlat(arg1, arg2)
        print "lon lat"
        print "{0} {1}".format(lon, lat)
    elif arg1 < ll_max and arg2 < ll_max:
        # consider that lon lat passed
        x, y = get_xy(arg1, arg2)
        print "x y"
        print "{0} {1}".format(x, y)
    else:
        raise Exception("wrong args")
