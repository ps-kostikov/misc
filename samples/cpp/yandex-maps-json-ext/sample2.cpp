#include <yandex/maps/json/builder.h>
#include <yandex/maps/json/value.h>
#include <yandex/maps/json/std.h>


#include <iostream>
#include <string>
#include <vector>


namespace mj = maps::json;


std::string unifyJson(const std::string& json) {
    mj::Value v = mj::Value::fromString(json);
    return (mj::Builder() << v).str();
}


int main()
{
    std::string json1 = R"_({"name": 1})_";
    std::string json2 = "{\"name\":    1  }";


    std::cout << ((unifyJson(json1) == unifyJson(json2)) ? "yes" : "no") << std::endl;
    return 0;
}
