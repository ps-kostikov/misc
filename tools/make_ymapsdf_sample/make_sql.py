#!/usr/bin/python

from collections import namedtuple

# Moscow
# lon_min, lat_min = 37.45104, 55.603821
# lon_max, lat_max = 37.522988, 55.639959

# India
# lon_min, lat_min = 72.943238, 18.676782
# lon_max, lat_max = 73.206910, 18.781662
lon_min, lat_min = 72.667620, 18.722290
lon_max, lat_max = 73.315813, 19.390878


input_schema = 'ymapsdf_navteq_india_20120701'
output_schema = 'ymapsdf_navteq_india_20500701'

srid = 4326


def simple_copy_sql(table):
    return """
INSERT INTO {output_schema}.{table}
(SELECT * FROM {input_schema}.{table}
);""".format(**dict(locals(), **globals()))


def filter_by_shape_sql(table):
    return """
INSERT INTO {output_schema}.{table}
(SELECT * FROM {input_schema}.{table}
    WHERE st_contains(
        st_setsrid(
            makebox2d(makepoint({lon_min}, {lat_min}), makepoint({lon_max}, {lat_max})), {srid}),
        shape)
);""".format(**dict(locals(), **globals()))


ForeignKey = namedtuple("ForeignKey", "from_table from_column to_table to_column")


def filter_by_foreign_keys_sql(table, fkeys):
    for fkey in fkeys:
        assert(table == fkey.from_table)

    def fkey_to_condition(fkey):
        return "(EXISTS (SELECT 1 FROM {output_schema}.{to_table} "\
                "WHERE {to_column} = {from_table}.{from_column}))".format(
                output_schema=output_schema,
                to_table=fkey.to_table,
                to_column=fkey.to_column,
                from_table=fkey.from_table,
                from_column=fkey.from_column)

    conditions = ' AND \n'.join(map(fkey_to_condition, fkeys))
    return """
INSERT INTO {output_schema}.{table}
(SELECT * FROM {input_schema}.{table} {table}
    WHERE {conditions}
);""".format(**dict(locals(), **globals()))


def filter_by_link_table_sql(table, built_table_fkey, table_fkey):
    assert(table == table_fkey.to_table)
    assert(built_table_fkey.from_table == table_fkey.from_table)

    return """
INSERT INTO {output_schema}.{table}
(SELECT * FROM {input_schema}.{table} {table}
    WHERE EXISTS (SELECT 1
        FROM {output_schema}.{built_table} {built_table}, {input_schema}.{link_table} {link_table}
        WHERE {link_table}.{lt_c} = {table}.{t_c} AND {link_table}.{lb_c} = {built_table}.{b_c})
);""".format(
        output_schema=output_schema,
        input_schema=input_schema,
        table=table,
        built_table=built_table_fkey.to_table,
        link_table=table_fkey.from_table,
        lt_c=table_fkey.from_column,
        t_c=table_fkey.to_column,
        lb_c=built_table_fkey.from_column,
        b_c=built_table_fkey.to_column)


def fkey_name(fkey):
    return '{table}_{column}_fkey'.format(table=fkey.from_table, column=fkey.from_column)


def drop_fkey_sql(fkey):
    return 'ALTER TABLE {output_schema}.{table} DROP CONSTRAINT {fkey_name};'.format(
            output_schema=output_schema,
            table=fkey.from_table,
            fkey_name=fkey_name(fkey))


def restore_fkey_sql(fkey):
    return """
UPDATE {output_schema}.{from_table} SET {from_column}=NULL
    WHERE NOT EXISTS (SELECT 1
        FROM {output_schema}.{to_table} WHERE {to_column} = {from_table}.{from_column}
);
ALTER TABLE {output_schema}.{from_table} ADD FOREIGN KEY
    ({from_column}) REFERENCES {output_schema}.{to_table} ({to_column});
""".format(
        output_schema=output_schema,
        from_table=fkey.from_table,
        from_column=fkey.from_column,
        to_table=fkey.to_table,
        to_column=fkey.to_column)

# print simple_copy_sql('access')

# print simple_copy_sql('ft_type')


print filter_by_shape_sql('node')

print filter_by_foreign_keys_sql('edge',
        fkeys=[ForeignKey('edge', 'f_node_id', 'node', 'node_id'),
               ForeignKey('edge', 't_node_id', 'node', 'node_id')])


face_egde_to_edge = ForeignKey('face_edge', 'edge_id', 'edge', 'edge_id')
face_edge_to_face = ForeignKey('face_edge', 'face_id', 'face', 'face_id')
print filter_by_link_table_sql('face',
        built_table_fkey=face_egde_to_edge,
        table_fkey=face_edge_to_face)

print filter_by_foreign_keys_sql('face_edge',
        fkeys=[face_egde_to_edge])

bld_face_to_face = ForeignKey('bld_face', 'face_id', 'face', 'face_id')
bld_face_to_bld = ForeignKey('bld_face', 'bld_id', 'bld', 'bld_id')
print filter_by_link_table_sql('bld',
        built_table_fkey=bld_face_to_face,
        table_fkey=bld_face_to_bld)

print filter_by_foreign_keys_sql('bld_face',
        fkeys=[bld_face_to_face])

bld_geom_to_bld = ForeignKey('bld_geom', 'bld_id', 'bld', 'bld_id')
print filter_by_foreign_keys_sql('bld_geom',
        fkeys=[bld_geom_to_bld])

ad_face_to_face = ForeignKey('ad_face', 'face_id', 'face', 'face_id')
ad_face_to_ad = ForeignKey('ad_face', 'ad_id', 'ad', 'ad_id')
ad_to_ad = ForeignKey('ad', 'p_ad_id', 'ad', 'ad_id')
print drop_fkey_sql(ad_to_ad)
print filter_by_link_table_sql('ad',
        built_table_fkey=ad_face_to_face,
        table_fkey=ad_face_to_ad)
print restore_fkey_sql(ad_to_ad)

print filter_by_foreign_keys_sql('ad_face',
        fkeys=[ad_face_to_face])

print filter_by_foreign_keys_sql('ad_dpcb',
        fkeys=[ForeignKey('ad_dpcb', 'ad_id', 'ad', 'ad_id')])

print filter_by_foreign_keys_sql('ad_dpc',
        fkeys=[ForeignKey('ad_dpc', 'ad_id', 'ad', 'ad_id')])

print filter_by_foreign_keys_sql('ad_dpu',
        fkeys=[ForeignKey('ad_dpu', 'ad_id', 'ad', 'ad_id')])

print filter_by_foreign_keys_sql('ad_excl',
        fkeys=[ForeignKey('ad_excl', 't_ad_id', 'ad', 'ad_id'),
               ForeignKey('ad_excl', 'e_ad_id', 'ad', 'ad_id')])

print filter_by_foreign_keys_sql('locality',
        fkeys=[ForeignKey('locality', 'ad_id', 'ad', 'ad_id')])

print filter_by_foreign_keys_sql('ad_nm',
        fkeys=[ForeignKey('ad_nm', 'ad_id', 'ad', 'ad_id')])

print filter_by_foreign_keys_sql('ad_geom',
        fkeys=[ForeignKey('ad_geom', 'ad_id', 'ad', 'ad_id')])

print filter_by_foreign_keys_sql('ad_center',
        fkeys=[ForeignKey('ad_center', 'ad_id', 'ad', 'ad_id'),
               ForeignKey('ad_center', 'node_id', 'node', 'node_id')])


ft_edge_to_edge = ForeignKey('ft_edge', 'edge_id', 'edge', 'edge_id')
ft_edge_to_ft = ForeignKey('ft_edge', 'ft_id', 'ft', 'ft_id')
ft_to_ft = ForeignKey('ft', 'p_ft_id', 'ft', 'ft_id')
print drop_fkey_sql(ft_to_ft)
print filter_by_link_table_sql('ft',
        built_table_fkey=ft_edge_to_edge,
        table_fkey=ft_edge_to_ft)
print restore_fkey_sql(ft_to_ft)

print filter_by_foreign_keys_sql('ft_edge',
        fkeys=[ft_edge_to_edge])

print filter_by_foreign_keys_sql('ft_geom',
        fkeys=[ForeignKey('ft_geom', 'ft_id', 'ft', 'ft_id')])

print filter_by_foreign_keys_sql('ft_nm',
        fkeys=[ForeignKey('ft_nm', 'ft_id', 'ft', 'ft_id')])

print filter_by_foreign_keys_sql('ft_center',
        fkeys=[ForeignKey('ft_center', 'node_id', 'node', 'node_id'),
               ForeignKey('ft_center', 'ft_id', 'ft', 'ft_id')])

print filter_by_foreign_keys_sql('ft_face',
        fkeys=[ForeignKey('ft_face', 'ft_id', 'ft', 'ft_id'),
               ForeignKey('ft_face', 'face_id', 'face', 'face_id')])


print filter_by_foreign_keys_sql('addr',
        fkeys=[ForeignKey('addr', 'node_id', 'node', 'node_id'),
               ForeignKey('addr', 'rd_id', 'rd', 'rd_id'),
               ForeignKey('addr', 'ad_id', 'ad', 'ad_id')])

print filter_by_foreign_keys_sql('addr_nm',
        fkeys=[ForeignKey('addr_nm', 'addr_id', 'addr', 'addr_id')])

print filter_by_foreign_keys_sql('addr_arr',
        fkeys=[ForeignKey('addr_arr', 'addr_id', 'addr', 'addr_id'),
               ForeignKey('addr_arr', 'node_id', 'node', 'node_id')])


print filter_by_shape_sql('rd_jc')

print filter_by_foreign_keys_sql('rd_el',
        fkeys=[ForeignKey('rd_el', 'f_rd_jc_id', 'rd_jc', 'rd_jc_id'),
               ForeignKey('rd_el', 't_rd_jc_id', 'rd_jc', 'rd_jc_id')])

rd_rd_el_to_rd_el = ForeignKey('rd_rd_el', 'rd_el_id', 'rd_el', 'rd_el_id')
rd_rd_el_to_rd = ForeignKey('rd_rd_el', 'rd_id', 'rd', 'rd_id')
print filter_by_link_table_sql('rd',
        built_table_fkey=rd_rd_el_to_rd_el,
        table_fkey=rd_rd_el_to_rd)

print filter_by_foreign_keys_sql('rd_rd_el',
        fkeys=[rd_rd_el_to_rd_el])

rd_nm_to_rd = ForeignKey('rd_nm', 'rd_id', 'rd', 'rd_id')
print filter_by_foreign_keys_sql('rd_nm',
        fkeys=[rd_nm_to_rd])

print filter_by_foreign_keys_sql('rd_geom',
        fkeys=[ForeignKey('rd_geom', 'rd_id', 'rd', 'rd_id')])

print filter_by_foreign_keys_sql('rd_center',
        fkeys=[ForeignKey('rd_center', 'rd_id', 'rd', 'rd_id'),
               ForeignKey('rd_center', 'node_id', 'node', 'node_id')])

print filter_by_foreign_keys_sql('rd_ad',
        fkeys=[ForeignKey('rd_ad', 'rd_id', 'rd', 'rd_id'),
               ForeignKey('rd_ad', 'ad_id', 'ad', 'ad_id')])


print filter_by_foreign_keys_sql('cond_rd_seq',
        fkeys=[ForeignKey('cond_rd_seq', 'rd_jc_id', 'rd_jc', 'rd_jc_id'),
               ForeignKey('cond_rd_seq', 'rd_el_id', 'rd_el', 'rd_el_id')])

print filter_by_foreign_keys_sql('cond',
        fkeys=[ForeignKey('cond', 'cond_seq_id', 'cond_rd_seq', 'cond_seq_id')])

print filter_by_foreign_keys_sql('cond_dt',
        fkeys=[ForeignKey('cond_dt', 'cond_id', 'cond', 'cond_id')])
