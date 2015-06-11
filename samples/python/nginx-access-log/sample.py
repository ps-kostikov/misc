import re

line = '[10/Jun/2015:00:00:05 +0300] vds-rdr.maps.yandex.net 2a02:6b8:0:1610::107b "GET /nav?&lang=ru_RU&x=9896&y=5129&z=14&zmin=15&zmax=15&simplification=0&l=vnvmap&geometry=0 HTTP/1.1" 200 "-" "-" "-" 0.000 HIT 998 "-" "/staticrenderer" "map=vnvmap_russia_1cb4ef24bb600cefd7e373f9858b39d374b0e520a15824450a4102f9b5520c04.xml&action=generateProtobuf&lang=ru_RU&y=5129&x=9896&distToSimplify=0&z=14&zmin=15&zmax=15&geometry=0" -'


line = '[10/Jun/2015:00:00:01 +0300] vec.tiles.maps.yandex.net ::ffff:95.108.231.227 "GET /tiles?l=mapj&x=48&y=21&z=6&zdata=9&v=4.34.3 HTTP/1.1" 404 "-" "-" "-" 0.001 MISS 322 "0.001" "/staticrenderer" "y=21&x=48&action=hotspots&z=6&zdata=9&map=map_asia_pacific_62e852a3e787706a6b2ae38a87376936da0e37de5a911bcd3a1e568e60e59542.xml,map_russia_7161fd04c896ba0b5dd1d5b95bc171c7223489af191aba024db74e87f3d7cc47.xml,map_world_8991022c487ce88eee730d79b5addacfed706d0b849deaf6245c54c86d7cd6f7.xml" -'


line = '[10/Jun/2015:00:00:01 +0300] pvec.tiles.maps.yandex.net ::ffff:95.108.144.10 "GET /?l=pmap&v=1429650000&x=19139&y=10562&z=15&lang=ru_RU&scale=1 HTTP/1.1" 200 "-" "-" "-" 0.008 - 8325 "0.008" "/wiki_staticrenderer" "action=render&map=pmap_1429650000.xml&compressionLevel=5&x=19139&y=10562&z=15&scale=1" -'

#[$time_local] $http_host $remote_addr "$request" $status "$http_referer" "$http_user_agent" "$http_cookie" $request_time $upstream_cache_status $bytes_sent "$upstream_response_time" "$uri" "$args" $ssl_session_id

print line
print ''
d = re.match('\[(?P<time_local>.+)\] (?P<http_host>[\w.-]+) (?P<remote_addr>[\w\d:\.]+) "(?P<request>.*)" (?P<status>\d+) "(?P<http_referer>.*)" "(?P<http_user_agent>.*)" "(?P<http_cookie>.*)" (?P<request_time>[\d\.]+) (?P<upstream_cache_status>[\w-]+) (?P<bytes_sent>\d+) "(?P<upstream_response_time>.*)" "(?P<uri>.*)" "(?P<args>.*)" (?P<ssl_session_id>\.*)', line).groupdict()

for k, v in d.iteritems():
    print k, ":", v