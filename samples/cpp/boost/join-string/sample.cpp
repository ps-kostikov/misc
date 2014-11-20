#include <boost153/algorithm/string/join.hpp>
#include <vector>
#include <iostream>

int main(int, char **)
{
    std::vector<std::string> list;
    list.push_back("Hello");
    list.push_back("World!");

    std::string joined = boost153::algorithm::join(list, ", ");
    std::cout << joined << std::endl;
    return 0;
}