#include <yandex/maps/json/builder.h>
#include <yandex/maps/json/value.h>

#include <iostream>
#include <string>


namespace mj = maps::json;

class A {
public:
    A(int i, std::string s) : i_(i), s_(s) {}
    int i() const {return i_;} 
    std::string s() const {return s_;} 

    // void json(mj::ObjectBuilder builder) const {
    //     builder["i"] = i_;
    //     builder["s"] = s_;
    // }

    static A fromJson(mj::Value value) {
        return A(
            value["i"].as<int>(),
            value["s"].as<std::string>()
        );
    }
private:
    int i_;
    std::string s_;
};

// void operator<<(mj::ObjectBuilder builder, const A& a) {

void operator<<(mj::Builder& builder, const A& a) {
    builder << [&a](mj::ObjectBuilder builder) {
        builder["i"] = a.i();
        builder["s"] = a.s();   
    };
}

std::ostream& operator<<(std::ostream& o, const A& a) {
    o << a.i() << std::endl;
    o << a.s() << std::endl;
    return o;
}

// void operator>>(mj::Value& value, A& a) {

// }

int main()
{
    A a(1, "hello");
    std::cout << a;
    mj::Builder builder;
    builder << a;
    std::cout << builder.str() << std::endl;


    // mj::Value(builder) >> 

    A a2 = A::fromJson(mj::Value::fromString(builder));
    std::cout << a2;

    A a3 = A::fromJson(mj::Value::fromString("{"
        "\"id\":2,"
        "\"s\":\"blah\""
    "}"));
    std::cout << a3;

    return 0;
}