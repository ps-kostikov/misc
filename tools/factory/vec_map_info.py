#!/usr/bin/python

import sys
import os
import logging

from yandex.maps.factory import db
from yandex.maps.factory.models import vec, vec_funcs, service
from yandex.maps.factory.config import config

logging.basicConfig(level=logging.DEBUG)
session = db.create_session(db.META_DB)


def region_path(region, conf):
    conf_parents = filter(
        lambda p: conf in p.configurations, region.parents)
    assert(len(conf_parents) <= 1)
    if conf_parents:
        return [region] + region_path(conf_parents[0], conf)
    return [region]


def print_info_by_release(layer_design, release):

    def find_region(conf):
        for region in conf.regions:
            for ld in region.layer_designs:
                if ld.id == layer_design.id:
                    return region

    def find_data(release, region):
        region_data = session.query(vec.RegionData)\
            .filter(vec.RegionData.region_id == region.id)\
            .filter(vec.RegionData.release_id == release.id)\
            .all()

        assert(len(region_data) == 1)
        data = session.query(vec.Data).get(region_data[0].data_id)
        return data

    print 'Name:', layer_design.name
    print 'Release:', release.name
    conf = release.configuration
    region = find_region(conf)
    print 'Regions:', ' -> '.join([r.name for r in region_path(region, conf)])
    data = find_data(release, region)
    print 'Data:', data.name

    task = vec_funcs.get_map_compilation_task(
        layer_design.id,
        data.id,
        session,
        service.MapCompilationTask.path != None)
    print 'Compiled map path:', os.path.join(config.compiled_maps_dir, task.path) if task else 'None'


def print_info(layer_design):
    for conf in layer_design.configurations:
        releases = session.query(vec.Release)\
            .filter(vec.Release.configuration_id == conf.id).all()
        for release in releases:
            print_info_by_release(layer_design, release)
        print ''


layer_design_id = int(sys.argv[1])
layer_design = session.query(vec.LayerDesign).get(layer_design_id)
print_info(layer_design)
