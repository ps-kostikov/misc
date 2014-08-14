#!/usr/bin/python

import sys

from yandex.maps import tileutils5

x, y, z = map(int, sys.argv[1:4])

tile_coord = tileutils5.TileCoord(x, y)
tile = tileutils5.Tile(tile_coord, z)
print tileutils5.tile2path(tile).full()
