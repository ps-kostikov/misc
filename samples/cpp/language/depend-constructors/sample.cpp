#include <iostream>
#include <string>
#include <vector>


class A
{
public:

    A() : A("hoho") {}
    A(const std::string& s) : s(s) {}

    std::string s;
};

int main()
{
    A a("bb");
    A a1;
    std::cout << a.s << std::endl;
    std::cout << a1.s << std::endl;

    return 0;
}
