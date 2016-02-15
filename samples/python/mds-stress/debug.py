#!/usr/bin/python

import time
import collections
import multiprocessing
import logging

from yandex.maps.factory.config import config

import mds


logging.getLogger().setLevel(logging.DEBUG)
logging.info("hello")

def gen_data():
    return 'c' * 10 * 1024


n = 1000
threads_num = 8


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
    retry_number=10,
    wait_seconds=0.01,
    wait_factor=2
)


storage = mds.MdsStorage(engine, retry_policy)
# storage = mds.MdsStorage.from_config(config.externals.mds, 'sat')

task_id = int(time.time())

begin = time.time()

def do_insert(i):
    data = gen_data()
    key = storage.write_data('stress_test_pkostikov_{0}_{1}'.format(i, task_id), data)
    # print 'get key', key
    storage.delete(key)
    # print 'deleted key', key

insert_pool = multiprocessing.pool.ThreadPool(threads_num)
insert_pool.map(do_insert, range(n))

total_time = time.time() - begin
print 'time', total_time
print 'rps', n / float(total_time)

