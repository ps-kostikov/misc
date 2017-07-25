# -*- coding: utf-8 -*-

import sys
import pandas as pd
import numpy as np

import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt

import sqlalchemy


engine = sqlalchemy.create_engine(sys.argv[1])

query = """
    select
        (resolved_at)::date as date,
        count(case when count>0 then 1 else NULL end) / count(1)::float as accepted_bind_percentage
    from (
        select ft.id as id, ft.resolved_at, count(cft.commit_id) as count
        from social.feedback_task ft
            left join social.commit_feedback_task cft on ft.id = cft.feedback_task_id
        where resolved_by is not NULL and resolution != 'rejected'
            and resolved_at > '2017-06-24'::date
            and extract(dow from resolved_at) not in (6, 0)
        group by 1, 2 order by 1 desc
    ) tmp
    group by 1 order by 1 ;

"""
df = pd.read_sql_query(query, con=engine)

plt.plot(df['date'], df['accepted_bind_percentage'])
plt.xlabel('Date')
plt.ylabel('Share of resolved feedbacks with history')
plt.gcf().autofmt_xdate()
plt.savefig('bind_feedback_daily.png')


# query = """
#     select
#         date_part('year', resolved_at) as year,
#         date_part('week', resolved_at) as week,
#         count(case when count>0 then 1 else NULL end) / count(1)::float as accepted_bind_percentage
#     from (
#         select ft.id as id, ft.resolved_at, count(cft.commit_id) as count
#         from social.feedback_task ft
#             left join social.commit_feedback_task cft on ft.id = cft.feedback_task_id
#         where resolved_by is not NULL and resolution != 'rejected'
#         group by 1, 2 order by 1 desc
#     ) tmp
#     group by 1, 2 order by 1, 2;

# """
# df = pd.read_sql_query(query, con=engine)

# plt.plot(df['week'], df['accepted_bind_percentage'])
# plt.xlabel('Week number (from the begining of the year)')
# plt.ylabel('Share of resolved feedbacks with history')
# plt.savefig('bind_feedback_weekly.png')
