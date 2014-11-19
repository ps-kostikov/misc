#include <boost153/algorithm/string.hpp>

#include <vector>
#include <iostream>
#include <string>


int main()
{
    std::vector<std::string> parts;
    boost153::split(parts, "A; B ;C", boost153::is_any_of(";"));
    // boost153::split(parts, "", boost153::is_any_of(";"));
    std::cout << "size of parts = " << parts.size() << std::endl;
    for (auto s: parts) {
        std::cout << s << std::endl;
    }
    return 0;
}