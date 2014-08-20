// #include <boost153/filesystem.hpp>
#include <boost153/date_time/gregorian/gregorian.hpp>
#include <boost153/date_time/posix_time/posix_time.hpp>
#include <iostream>
#include <string>

namespace bg = boost153::gregorian;
namespace bpt = boost153::posix_time;

int main()
{
    std::string s("2001-10-9");
    bg::date date(bg::from_simple_string(s));
    std::cout << bg::to_simple_string(date) << std::endl;

    bpt::ptime t(bg::date(2002, bg::Feb, 1), bpt::hours(11));
    bpt::ptime t2 = bpt::from_time_t(3600);
    std::cout << bpt::to_simple_string(t) << std::endl;
    std::cout << bpt::to_simple_string(t2) << std::endl;
    return 0;
}
