#include <yandex/maps/json/builder.h>
#include <yandex/maps/json/value.h>
#include <yandex/maps/json/std.h>

#include <iostream>
#include <string>
#include <vector>


namespace mj = maps::json;


int main()
{
    mj::Builder builder;
    std::vector<int> v{1, 2, 3};
    builder << v;
    std::cout << builder.str() << std::endl;

    return 0;
}