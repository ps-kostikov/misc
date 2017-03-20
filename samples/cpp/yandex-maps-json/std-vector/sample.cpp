#include <yandex/maps/json/builder.h>
#include <yandex/maps/json/value.h>
#include <yandex/maps/json/std.h>

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>


namespace mj = maps::json;


int main()
{
    mj::Builder builder;
    // std::vector<int> v{1, 2, 3};
    std::unordered_map<std::string, std::string> m;
    m["a"] = "AA";
    m["b"] = "BB";
    builder << m;
    // builder << v;
    std::cout << builder.str() << std::endl;

    return 0;
}