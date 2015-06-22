#!/usr/bin/python

import os
import logging
import shapely
import collections
import opster
import datetime
import StringIO
from subprocess import Popen, PIPE
from sqlalchemy import orm

from yandex.maps import tileutils5
from yandex.maps.factory.geomhelpers import eval_tile_geom

from yandex.maps.factory.models import sat, sat_funcs
from yandex.maps.factory import db


def load_mosaic(name, session):
    return (session.query(sat.Mosaic)
            .filter(sat.Mosaic.name == name)
            .scalar())


def calc_image_status(image):
    if (not os.path.exists(image.tif_path)):
        return "TIF DOES NOT EXIST"
    return "OK"

@opster.command()
def main(name=('n', '', 'mosaic name')):
    logging.getLogger().setLevel(logging.ERROR)

    with db.session_ctx() as session:
        mosaic = load_mosaic(name, session)
        print 'mosaic name', mosaic.name
        source_path_status = 'OK' if (os.path.exists(mosaic.mosaic_source.path)) else 'FAIL'
        print 'source path', mosaic.mosaic_source.path, source_path_status

        print 'images:'
        for image in mosaic.images:
            print '   ', image.zoom, image.tif_path, calc_image_status(image)


if __name__ == '__main__':
    main()
