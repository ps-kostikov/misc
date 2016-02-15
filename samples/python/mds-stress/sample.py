#!/usr/bin/python

import time
import collections
import multiprocessing

from yandex.maps.factory import mds
from yandex.maps.factory.config import config



def gen_data():
    return 'c' * 10 * 1024


n = 2000
threads_num = 16


RetryPolicy = collections.namedtuple('RetryPolicy', 'retry_number wait_seconds, wait_factor')

# auth = config.externals.mds.namespaces._asdict()['sat']
# engine = mds.MdsEngine(
#     schema=config.externals.mds.schema,
#     host=config.externals.mds.host,
#     read_port=config.externals.mds.read_port,
#     write_port=config.externals.mds.write_port,
#     auth_header=auth.auth_header,
#     namespace=auth.namespace,
#     subdir=auth.subdir)
# retry_policy = config.externals.mds.retry_policy


engine = mds.MdsEngine(
    schema='http',
    host='storage-int.mdst.yandex.net',
    # host='storage-int.mds.yandex.net',
    read_port=80,
    write_port=1111,
    auth_header='Basic bWFwcy1mYWN0b3J5LXNhdDphYTM4YTNhZjQyOTg4YjdkZDNhNzViNTMzYzUyMzIxMw==',
    # auth_header='Basic bWFwcy1mYWN0b3J5LXNhdDplNjNhODU3NGMxMzYzNTlhOWU3Yjc2Yjg2YjY5YmY0ZA==',
    namespace='maps-factory-sat',
    subdir='pkostikov'
)
retry_policy = RetryPolicy(
    retry_number=20,
    wait_seconds=0.01,
    wait_factor=2
)


storage = mds.MdsStorage(engine, retry_policy)
# storage = mds.MdsStorage.from_config(config.externals.mds, 'sat')


keys = []

insert_begin = time.time()

def do_insert(i):
    data = gen_data()
    key = storage.write_data('stress_test_pkostikov_{0}'.format(i), data)
    keys.append(key)

insert_pool = multiprocessing.pool.ThreadPool(threads_num)
insert_pool.map(do_insert, range(n))

# for i in range(n):
#     do_insert(i)


insert_time = time.time() - insert_begin
print 'insert time', insert_time
print 'insert rps', n / float(insert_time)


with open('keys.txt', 'w') as f:
    for key in keys:
        print >>f, key


delete_begin = time.time()


def do_delete(key):
    storage.delete(key)

delete_pool = multiprocessing.pool.ThreadPool(threads_num)
delete_pool.map(do_delete, keys)

# for key in keys:
#     do_delete(key)

delete_time = time.time() - delete_begin
print 'delete time', delete_time
print 'delete rps', n / float(delete_time)
