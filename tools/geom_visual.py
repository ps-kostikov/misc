import sys
import shapely.wkt

wkt = sys.argv[1]
print wkt

geom = shapely.wkt.loads(wkt)


def geom_to_points(geom):
    return [c for c in geom.exterior.coords]


def points_to_url(points):
    def point_to_str(p):
        return '{0},{1}'.format(p[0], p[1])

    return 'http://static-maps.yandex.ru/1.x/?l=map&pl={0}'.format(
        ','.join(map(point_to_str, points)))


print points_to_url(geom_to_points(geom))
