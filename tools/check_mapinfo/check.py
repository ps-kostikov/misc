#!/usr/bin/python

import sys
from osgeo import ogr


# data_set_path = '/mnt/datastorage/source/transnavicom/2013/ukraine/20130322/mapinfo/'
# layer_name = 'hydrography'

if len(sys.argv) > 2:
    data_set_path, layer_name = sys.argv[1:3]
else:
    data_set_path, layer_name = sys.argv[1], None


print 'opening data set {0}'.format(data_set_path)
data_set = ogr.Open(data_set_path)


def check_layer(data_set, layer_name):
    print 'checking layer {0}'.format(layer_name)
    layer = data_set.GetLayerByName(layer_name)

    print 'total features = {0}'.format(layer.GetFeatureCount())

    for fid in range(1, layer.GetFeatureCount() + 1):
        try:
            feature = layer.GetFeature(fid)
            geom_ref = feature.GetGeometryRef()
            if not geom_ref.IsValid():
                print 'error in fid {0} wkt = {1}'.format(fid, geom_ref.ExportToWkt())
        except:
            print 'something wrong with fid {0}'.format(fid)


def get_layer_names(data_set):
    return [data_set.GetLayerByIndex(i).GetName()
            for i in range(data_set.GetLayerCount())]


if layer_name is not None:
    check_layer(data_set, layer_name)
else:
    for layer_name in get_layer_names(data_set):
        check_layer(data_set, layer_name)
