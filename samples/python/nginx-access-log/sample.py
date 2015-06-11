import re

line = '[10/Jun/2015:00:00:05 +0300] vds-rdr.maps.yandex.net 2a02:6b8:0:1610::107b "GET /nav?&lang=ru_RU&x=9896&y=5129&z=14&zmin=15&zmax=15&simplification=0&l=vnvmap&geometry=0 HTTP/1.1" 200 "-" "-" "-" 0.000 HIT 998 "-" "/staticrenderer" "map=vnvmap_russia_1cb4ef24bb600cefd7e373f9858b39d374b0e520a15824450a4102f9b5520c04.xml&action=generateProtobuf&lang=ru_RU&y=5129&x=9896&distToSimplify=0&z=14&zmin=15&zmax=15&geometry=0" -'


#[$time_local] $http_host $remote_addr "$request" $status "$http_referer" "$http_user_agent" "$http_cookie" $request_time $upstream_cache_status $bytes_sent "$upstream_response_time" "$uri" "$args" $ssl_session_id

print line
print ''
d = re.match('\[(?P<time_local>.+)\] (?P<http_host>[\w.-]+) (?P<remote_addr>[\w\d:]+) "(?P<request>.*)" (?P<status>\d+) "(?P<http_referer>.*)" "(?P<http_user_agent>.*)" "(?P<http_cookie>.*)" (?P<request_time>[\d\.]+) (?P<upstream_cache_status>\w+) (?P<bytes_sent>\d+) "(?P<upstream_response_time>.*)" "(?P<uri>.*)" "(?P<args>.*)" (?P<ssl_session_id>\.*)', line).groupdict()
for k, v in d.iteritems():
    print k, ":", v