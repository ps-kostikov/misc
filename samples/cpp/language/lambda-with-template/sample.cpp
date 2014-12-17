#include <iostream>
#include <string>
#include <functional>

struct A
{
    int i;
};

int get(const A& a)
{
    return a.i;
}

template<class C>
void print(const C& c, std::function<int(const C&)> f)
{
    std::cout << f(c) << std::endl;
}

int main()
{
    A a{3};

    print<A>(a, get);

    auto f = [](const A& a) {
        return a.i;
    };
    print<A>(a, f);

    print<A>(a, [](const A& a) {
        return a.i;
    });
    return 0;
}
