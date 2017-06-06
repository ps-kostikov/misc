import sys
import datetime
import pandas as pd
import numpy as np

import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt

import sqlalchemy



engine = sqlalchemy.create_engine(sys.argv[1])

query = """
    select id, created_at, resolved_at, closed_at 
    from social.feedback_task
"""
df = pd.read_sql_query(query, con=engine)

new_ages_df = df.iloc[:].copy()
resolved_ages_df = df.iloc[:].copy()

def start_datetime():
    tmp = new_ages_df['created_at'].min() + datetime.timedelta(days=1)
    return datetime.datetime(year=tmp.year, month=tmp.month, day=tmp.day, tzinfo=tmp.tz)

tp = start_datetime()
max_date = new_ages_df['closed_at'].max()
while tp < max_date:
    col_name = tp.strftime('%m-%d')

    new_ages_df[col_name] = tp - new_ages_df[(new_ages_df['created_at'] <= tp) & (new_ages_df['resolved_at'] > tp)]['created_at']
    new_ages_df[col_name] = new_ages_df[col_name].apply(lambda td: td.total_seconds() / (60. * 60 * 24))

    resolved_ages_df[col_name] = tp - resolved_ages_df[(resolved_ages_df['resolved_at'] <= tp) & (resolved_ages_df['closed_at'] > tp)]['resolved_at']
    resolved_ages_df[col_name] = resolved_ages_df[col_name].apply(lambda td: td.total_seconds() / (60. * 60 * 24))

    tp += datetime.timedelta(days=1)


new_ages_df = new_ages_df.iloc[:,6:]
new_ages_df.quantile([.5, .85, .95]).fillna(0.).T.plot(rot=90).get_figure().savefig('new_ages_quantiles.png')
new_ages_df.count().fillna(0).T.plot(rot=90).get_figure().savefig('new_count.png')


resolved_ages_df = resolved_ages_df.iloc[:,6:]
resolved_ages_df.quantile([.5, .85, .95]).fillna(0.).T.plot(rot=90).get_figure().savefig('resolved_ages_quantiles.png')
resolved_ages_df.count().fillna(0).T.plot(rot=90).get_figure().savefig('resolved_count.png')
