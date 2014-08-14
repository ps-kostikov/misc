#!/usr/bin/python

schema = 'ymapsdf_navteq_india_20500701'

tables = [
    'cond_dt',
    'cond',
    'cond_rd_seq',
    'rd_ad',
    'rd_center',
    'rd_geom',
    'rd_nm',
    'rd_rd_el',
    'rd',
    'rd_el',
    'rd_jc',
    'addr_arr',
    'addr_nm',
    'addr',
    'ft_face',
    'ft_center',
    'ft_nm',
    'ft_geom',
    'ft_edge',
    'ft',
    'ad_center',
    'ad_geom',
    'ad_nm',
    'locality',
    'ad_excl',
    'ad_dpu',
    'ad_dpc',
    'ad_dpcb',
    'ad_face',
    'ad',
    'bld_geom',
    'bld_face',
    'bld',
    'face_edge',
    'face',
    'edge',
    'node',
    'ft_type',
    'access',
]

const_tables = [
    'ft_type',
    'access',
]

for table in tables:
    if table in const_tables:
        continue
    print 'TRUNCATE TABLE {0}.{1} CASCADE;'.format(schema, table)