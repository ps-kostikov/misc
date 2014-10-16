# -*- coding: utf-8 -*-
import os
import stat
import shutil
import errno
import time
import base64
import logging
import datetime
import tempfile
import shapely.wkt
import opster

from yandex.maps import tilerenderer


NAV_LAYERS = ("vnvmap", "vnvskl", "vnvmapnt")

logger = logging.getLogger()
compilation_log_dir = 'compilation_log'


def compile_map(design_path, map_path, connection_string, zoom_to_boundary, layer, temp_dir="tmp"):
    def create_compile_options_for_map(layer_name):
        if layer_name in NAV_LAYERS:
            options = tilerenderer.create_compile_options_for_navigator()
        else:
            options = tilerenderer.create_compile_options()
            options.import_attributes = True
            options.store_source_id = True
            options.transform_text_icons = True
        options.use_file_packing = True
        return options

    _, tmp_log_path = tempfile.mkstemp(dir=temp_dir)

    renderer = tilerenderer.TileRenderer(
        tmp_log_path, connection_string, 8)
    logger.debug('Renderer created')

    renderer.open(design_path)
    logger.debug('Map opened')

    zooms = tilerenderer.ZoomLevelsList()
    for zoom, geom in zoom_to_boundary.iteritems():
        zooms.insert(zoom)
        renderer.setBoundaryWkbInBase64(zoom, base64.b64encode(geom.wkb))

    progress = tilerenderer.OperationProgress()
    # options = create_compile_options_for_map('map')
    options = create_compile_options_for_map(layer)

    logger.debug('Start compilation')
    try:
        renderer.compile(map_path, progress, zooms, options)
        while renderer.getState() == tilerenderer.State.Compiling:
            time.sleep(5)
    except Exception, ex:
        logger.exception(ex)
    finally:
        time_str = datetime.datetime.now().strftime('%H:%M_%d.%m.%Y')
        log_file_name = '{time}.log'.format(time=time_str)
        compilation_log = os.path.join(compilation_log_dir,
                                       log_file_name)
        logger.debug("mv %s %s", tmp_log_path, compilation_log)
        if not os.path.exists(compilation_log_dir):
            os.makedirs(compilation_log_dir)
        shutil.move(tmp_log_path, compilation_log)

        # default mode is 600 that is uncomfortable
        os.chmod(compilation_log, stat.S_IWRITE | stat.S_IREAD | stat.S_IRGRP | stat.S_IROTH)

    if renderer.getState() == tilerenderer.State.Failed:
        raise Exception("Compilation process failed {0}".format(
            renderer.getStateDescription()))
    logger.debug('Compilation finished')


@opster.command()
def main(design_path=('d', "", 'design xml path'),
         boundary_path=('b', "", 'boundary wkt path'),
         connection_string=('c', "", 'connection string'),
         zmin=('', 1, "min zoom"),
         zmax=('', 18, "max zoom"),
         layer=('l', 'map', 'layer name')):
    print 'design_path =', design_path
    print 'boundary_path =', boundary_path
    print 'connection_string =', connection_string
    print 'zmin = ', zmin,
    print 'zmax = ', zmax

    with open(boundary_path) as o:
        boundary = shapely.wkt.loads(o.read())
    zoom_to_boundary = dict(
        (i, boundary)
        for i in range(zmin, zmax + 1)
    )

    output_map_path = "map.xml"
    print 'output_map_path =', output_map_path

    compile_map(design_path, output_map_path, connection_string, zoom_to_boundary, layer)

if __name__ == '__main__':
    main()
