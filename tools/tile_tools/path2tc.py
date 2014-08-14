#!/usr/bin/python

import sys

from yandex.maps import tileutils5

tile = tileutils5.path2tile(sys.argv[1])
print '%s %s %s' % (tile.coord().x(), tile.coord().y(), tile.scale())
