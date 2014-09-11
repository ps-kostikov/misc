#include <yandex/maps/json/builder.h>
#include <yandex/maps/json/value.h>

#include <iostream>
#include <string>
#include <set>
#include <map>


namespace mj = maps::json;

namespace maps {
namespace json {

template <class T>
void json(const std::set<T>& set, ArrayBuilder builder) {
    for (const auto& v: set) {
        builder << v;
    }
}

}
}


typedef int ID;

struct Obj {
    Obj() {}
    Obj(ID id_) : id(id_) {}
    Obj(const Obj&) = default;
    Obj(Obj&&) = default;
    Obj& operator=(const Obj&) = default;
    Obj& operator=(Obj&&) = default;

    ID id = 0;
};

struct A: public Obj {
    A() {}
    A(ID id, std::string s_, ID b_, std::set<ID> cs_): Obj(id), s(s_), b(b_), cs(cs_) {}
    A(const A&) = default;
    A(A&&) = default;
    A& operator=(const A&) = default;
    A& operator=(A&&) = default;

    std::string s;
    ID b;
    std::set<ID> cs;
};

struct B: public Obj {
    B() {}
    B(ID id, ID c_) : Obj(id), c(c_) {}
    B(const B&) = default;
    B(B&&) = default;
    B& operator=(const B&) = default;
    B& operator=(B&&) = default;

    ID c;
};

struct C: public Obj {
    C() {}
    C(ID id, std::string s_) : Obj(id), s(s_) {}
    C(const C&) = default;
    C(C&&) = default;
    C& operator=(const C&) = default;
    C& operator=(C&&) = default;

    std::string s;
};

void json(const A& a, mj::ObjectBuilder b) {
    b["id"] = a.id;
    b["s"] = a.s;
    b["b"] = a.b;
    b["cs"] = a.cs;
}

template<class T>
T get(ID id);

std::map<ID, A> as;
std::map<ID, B> bs;
std::map<ID, C> cs;

template<>
A get(ID id) {return as[id];}

template<>
B get(ID id) {return bs[id];}

template<>
C get(ID id) {return cs[id];}


template<class T>
void jsonRec(const T&, int depth, mj::ObjectBuilder b);

template<>
void jsonRec<A>(const A&, int, mj::ObjectBuilder);

template<>
void jsonRec<B>(const B&, int, mj::ObjectBuilder);

template<>
void jsonRec<C>(const C&, int, mj::ObjectBuilder);


template<>
void jsonRec(const A& a, int depth, mj::ObjectBuilder bld) {
    bld["id"] = a.id;
    bld["s"] = a.s;
    if (depth > 0) {
        bld["b"] << [&](mj::ObjectBuilder bbld) {
            auto b = get<B>(a.b);
            jsonRec(b, depth - 1, bbld);
        };
    } else {
        bld["b"] = a.b;
    }
    if (depth > 0) {
        bld["cs"] << [&](mj::ArrayBuilder csbld) {
            for (auto cid : a.cs) {
                csbld << [&](mj::ObjectBuilder cbld) {
                    auto c = get<C>(cid);
                    jsonRec<C>(c, depth - 1, cbld);
                };
            }
        };
    } else {
        bld["cs"] = a.cs;
    }
}


template<>
void jsonRec(const B& b, int depth, mj::ObjectBuilder bld) {
    bld["id"] = b.id;
    if (depth > 0) {
        bld["c"] << [&](mj::ObjectBuilder cbld) {
            auto c = get<C>(b.c);
            jsonRec<C>(c, depth - 1, cbld);
        };
    } else {
        bld["c"] = b.c;
    }
}

template<>
void jsonRec(const C& c, int depth, mj::ObjectBuilder bld) {
    (void)depth;
    bld["id"] = c.id;
    bld["s"] = c.s;
}


template<class T>
void jsonRecID(ID, int, mj::ObjectBuilder);


template<>
void jsonRecID<A>(ID, int, mj::ObjectBuilder);
template<>
void jsonRecID<B>(ID, int, mj::ObjectBuilder);
template<>
void jsonRecID<C>(ID, int, mj::ObjectBuilder);

template<>
void jsonRecID<A>(ID id, int depth, mj::ObjectBuilder bld) {
    auto a = get<A>(id);
    bld["id"] = a.id;
    bld["s"] = a.s;
    if (depth > 0) {
        bld["b"] << [&](mj::ObjectBuilder bbld) {
            jsonRecID<B>(a.b, depth - 1, bbld);
        };
    } else {
        bld["b"] = a.b;
    }
    if (depth > 0) {
        bld["cs"] << [&](mj::ArrayBuilder csbld) {
            for (auto cid : a.cs) {
                csbld << [&](mj::ObjectBuilder cbld) {
                    jsonRecID<C>(cid, depth - 1, cbld);
                };
            }
        };
    } else {
        bld["cs"] = a.cs;
    }
}


template<>
void jsonRecID<B>(ID id, int depth, mj::ObjectBuilder bld) {
    auto b = get<B>(id);
    bld["id"] = b.id;
    if (depth > 0) {
        bld["c"] << [&](mj::ObjectBuilder cbld) {
            jsonRecID<C>(b.c, depth - 1, cbld);
        };
    } else {
        bld["c"] = b.c;
    }
}

template<>
void jsonRecID<C>(ID id, int depth, mj::ObjectBuilder bld) {
    auto c = get<C>(id);
    (void)depth;
    bld["id"] = c.id;
    bld["s"] = c.s;
}




int main()
{
    std::cout << "hello" << std::endl;
    A a1{1, "s", 2, std::set<ID>{3, 4}};
    as[a1.id] = a1;
    B b2{2, 3};
    bs[b2.id] = b2;
    C c3{3, "c3"};
    cs[c3.id] = c3;
    C c4{4, "c4"};
    cs[c4.id] = c4;


    mj::Builder builder;

    // builder << [&](mj::ObjectBuilder b) {
    //     jsonRec(a1, 2, b);
    // };

    // builder << [&](mj::ObjectBuilder b) {
    //     jsonRec(c3, 2, b);
    // };

    // builder << [&](mj::ObjectBuilder b) {
    //     jsonRecID<B>(2, 2, b);
    // };

    builder << [&](mj::ObjectBuilder b) {
        jsonRecID<B>(1, 2, b);
    };

    std::cout << builder.str() << std::endl;

    return 0;
}