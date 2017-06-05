import sys
import pandas as pd
import numpy as np

import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt

import sqlalchemy


engine = sqlalchemy.create_engine(sys.argv[1])

query = """
    select ft.id as id, count(cft.commit_id) as count
    from social.feedback_task ft left join social.commit_feedback_task cft
    on ft.id = cft.feedback_task_id
    where closed_by is not NULL and resolution != 'rejected'
    group by 1 order by 1 desc
"""
df = pd.read_sql_query(query, con=engine)

threshold = 10
df.loc[df['count'] > threshold, 'count'] = threshold
print(df['count'].value_counts())

counts, bins, patche = plt.hist(df['count'], bins=10, range=(0, threshold))
plt.xticks(bins)
plt.xlabel('Number of commits')
plt.ylabel('Number of accepted closed feedback tasks')
plt.savefig('feedback_commits_distribution.png')
