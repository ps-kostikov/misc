#include <iostream>
#include <type_traits>

int f()
{
    return 1;
}

int g(int n)
{
    return n;
}

struct H
{
    int operator()(int n) {return n;}
};

int main()
{
    int n1 = 1;
    decltype(n1) n2 = n1;
    decltype(f()) n3 = n1;
    // std::result_of<g(int)>::type n4 = n1; // does not work
    std::result_of<H(int)>::type n5 = n1;

    auto i = [](int n) {
        return n;
    };
    std::result_of<decltype(i)(int)>::type n6 = n1;

    std::cout << "n2 = " << n2 << std::endl;
    std::cout << "n3 = " << n3 << std::endl;
    // std::cout << "n4 = " << n4 << std::endl;
    std::cout << "n5 = " << n5 << std::endl;
    std::cout << "n6 = " << n6 << std::endl;

    return 0;
}
