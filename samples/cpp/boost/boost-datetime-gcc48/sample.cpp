// #include <boost153/filesystem.hpp>
#include <boost153/date_time/gregorian/gregorian.hpp>
#include <iostream>
#include <string>

namespace bg = boost153::gregorian;

int main()
{
    std::string s("2001-10-9");
    bg::date date(bg::from_simple_string(s));
    std::cout << bg::to_simple_string(date) << std::endl;
    return 0;
}
