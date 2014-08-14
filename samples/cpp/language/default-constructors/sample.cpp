#include <iostream>
#include <string>
#include <vector>


class B
{
public:
    std::string s;
};

class A
{
public:

    int i;
    std::string s;
    std::vector<int> v;
    std::vector<B> vb;
 };

int main()
{
    A a{1, "s", {1, 2, 4}, {B{"a"}, B{"b"}}};

    std::cout << a.s << std::endl;
    std::cout << a.v[2] << std::endl;
    std::cout << a.vb[1].s << std::endl;

    return 0;
}
