local old_cpath = package.cpath
package.cpath = "/usr/lib/yandex/maps/satproxy/satproxy.so"
local satproxy = require("satproxy")
package.cpath = old_cpath                                                                                                                                           
satproxy.init("/etc/yandex/maps/satproxy/config.xml")
                                                                                                                                                                        
local x = 307777
local y = 204771
local z = 19

                                                                                                                                                            
newQuery, code = satproxy.convert_query("sat", "", x, y, z, "0")    
print (newQuery)
