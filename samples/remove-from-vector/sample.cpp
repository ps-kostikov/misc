#include <iostream>
#include <algorithm>
#include <string>
#include <vector>


struct A
{
    A(const std::string& s) : s(s) {}
    std::string s;
};

int main()
{
    std::vector<A> va{A("1"), A("2"), A("2"), A("3")};
    std::cout << va.size() << std::endl;

    va.erase(
        std::remove_if(
            va.begin(),
            va.end(),
            [](const A& a) -> bool {
                return a.s == "2";
            }
        ),
        va.end()
    );
    std::cout << va.size() << std::endl;

    return 0;
}