#!/usr/bin/python

import sys
import re
import collections


uri_to_time = collections.defaultdict(list)


def unify_uri(uri):
    return re.sub('\d+', '<digits>', uri)


def handle_line(line):
    match = re.match('\[(?P<time>.*)\] (?P<host>[\w.]+) (?P<remote>[0-9a-f:.]+) "(?P<request>[^"]+)" (?P<status>\d+) "(?P<referer>[^"]+)" "(?P<user_agent>[^"]+)" "(?P<cookie>[^"]+)" (?P<request_time>[^ ]+) (?P<cache_status>[^ ]+) (?P<bytes_sent>[^ ]+) "(?P<response_time>[^"]+)" "(?P<uri>[^"]+)" "(?P<args>[^"]+)" (?P<session_id>[^ ]+)', line)

    if not match:
        return

    host = match.group('host')
    if host != 'backoffice.maps.yandex.net':
        return

    uri = match.group('uri')
    time = float(match.group('request_time'))
    uri_to_time[unify_uri(uri)].append(time)


for line in sys.stdin:
    handle_line(line.strip())


class Stat(collections.namedtuple("Stat", "avg num min max")):
    def __str__(self):
        return 'Stat(avg={0:6.3f}, num={1}, min={2:6.3f}, max={3:6.3f})'.format(
            self.avg, self.num, self.min, self.max)


def timelist_to_stat(timelist):
    return Stat(
        sum(timelist) / len(timelist),
        len(timelist),
        min(timelist),
        max(timelist)
    )


uri_to_stat = dict(
    (uri, timelist_to_stat(timelist))
    for uri, timelist in uri_to_time.iteritems())


for uri, stat in sorted(uri_to_stat.items(), key=lambda i: i[1].num, reverse=True):
    print uri, stat
