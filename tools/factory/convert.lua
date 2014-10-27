local old_cpath = package.cpath
package.cpath = "/usr/lib/yandex/maps/satproxy/satproxy.so"
local satproxy = require("satproxy")
package.cpath = old_cpath
satproxy.init("/etc/yandex/maps/satproxy/config.xml")

local x = tonumber(arg[1])
local y = tonumber(arg[2])
local z = tonumber(arg[3])

newQuery, etag, code = satproxy.convert_query("sat", "", x, y, z, "0", "")
if code ~= 200 then
    print ('Not found (code=' .. code .. ")")
end
print ('Read query: http://localhost' ..  newQuery)

nameArg = string.sub(newQuery, string.find(newQuery, 'name=stl.%d+.%d+.%d+.%d+.jpg'))
print ('Write query: http://localhost:9000/elliptics?' .. nameArg .. '&embed_timestamp&timestamp=1')
