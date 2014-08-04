#include <yandex/maps/json/builder.h>
#include <yandex/maps/json/value.h>
#include <yandex/maps/json/std.h>

#include <boost153/date_time/gregorian/gregorian.hpp>


#include <iostream>
#include <string>
#include <vector>


namespace mj = maps::json;
namespace bg = boost153::gregorian;


struct A
{
    enum class Type {
        Type1,
        Type2,
    };
    int i;
    Type t;
    bg::date d;
};

void json(const bg::date& d, mj::VariantBuilder b) {
    b << bg::to_simple_string(d);
}

void json(const A::Type& t, mj::VariantBuilder b) {
    switch (t) {
        case A::Type::Type1: b << "type1"; break;
        case A::Type::Type2: b << "type2"; break;
    };
}

void json(const A& a, mj::ObjectBuilder b) {
    b["i"] = a.i;
    b["t"] = a.t;
    b["d"] = a.d;
}


int main()
{
    A a{2, A::Type::Type1, bg::from_simple_string("2001-10-9")};
    mj::Builder builder;
    builder << a;
    std::cout << builder.str() << std::endl;

    return 0;
}
