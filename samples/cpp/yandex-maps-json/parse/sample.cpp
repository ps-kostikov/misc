#include <yandex/maps/json/builder.h>
#include <yandex/maps/json/value.h>
#include <yandex/maps/json/std.h>

#include <iostream>
#include <string>


namespace mj = maps::json;

// struct K {
    
// };

class A {
public:
    A(int i, std::string s, std::vector<int> a = std::vector<int>()) : i_(i), s_(s), a_(a) {}
    int i() const {return i_;} 
    std::string s() const {return s_;}
    std::vector<int> a() const {return a_;}

    // void json(mj::ObjectBuilder builder) const {
    //     builder["i"] = i_;
    //     builder["s"] = s_;
    // }

    static A fromJson(mj::Value value) {
        std::vector<int> a;
        auto val = value["a"];
        for (auto v = val.begin(); v != val.end(); ++v) {
            a.push_back(v->as<int>());
        }
        return A(
            value["i"].as<int>(),
            value["s"].as<std::string>(),
            a
        );
    }
private:
    int i_;
    std::string s_;
    std::vector<int> a_;
};

// void operator<<(mj::ObjectBuilder builder, const A& a) {

void operator<<(mj::Builder& builder, const A& a) {
    builder << [&a](mj::ObjectBuilder builder) {
        builder["i"] = a.i();
        builder["s"] = a.s();   
        builder["a"] = a.a();
    };
}

std::ostream& operator<<(std::ostream& o, const A& a) {
    o << a.i() << std::endl;
    o << a.s() << std::endl;
    o << "[" << std::endl;
    for (auto i : a.a()) {
        o << i << std::endl;
    }
    o << "]" << std::endl;
    return o;
}

// void operator>>(mj::Value& value, A& a) {

// }

int main()
{
    A a(1, "hello", {1, 2, 3});
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