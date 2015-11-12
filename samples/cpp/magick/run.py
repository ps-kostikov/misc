import sys
import logging
import collections

from yandex.maps.analyzer.api.mr_holder import MrHolder


logging.basicConfig(stream=sys.stdout, level=logging.DEBUG)


class MRLogger:

    def log(self, msg, data=None):
        logging.debug("mapreduce: %s %s", msg, str(data))

with MrHolder(logger=MRLogger(), default_jobcount=1, regular_binaries=['./sample', './xc:#000000000000']) as mr:
    tmp_in = mr.temp()
    Record = collections.namedtuple('Record', 'key subkey value')
    records = [
        Record("key1", "subkey1", "value1"),
        Record("key2", "subkey2", "value2"),
    ]
    mr.writerecords(records, tmp_in)
    tmp_out = mr.map('sample', tmp_in)

