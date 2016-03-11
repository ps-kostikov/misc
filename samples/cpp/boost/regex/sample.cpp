#include <stdlib.h>
#include <boost153/regex.hpp>
#include <string>
#include <iostream>

using namespace boost153;

int main()
{
    std::string input = "postgresql://mapsfactory:mapsfactory@wiki9.maps.dev.yandex.net:5432/mapsfactory";
    boost153::regex rgx("postgresql://(\\w+):(\\w+)@([\\w\\.]+):(\\d+)/(\\w+)");
    boost153::smatch match;
    boost153::regex_match(input, match, rgx);
    for (auto m: match) {
        std::cout << m << std::endl;
    }
    std::cout << std::endl;
    std::cout << "user = " << match[1] << std::endl;
    std::cout << "password = " << match[2] << std::endl;
    std::cout << "host = " << match[3] << std::endl;
    std::cout << "port = " << std::stoi(match[4]) << std::endl;
    std::cout << "dbname = " << match[5] << std::endl;
}