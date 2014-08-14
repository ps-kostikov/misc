#include <boost153/lexical_cast.hpp>
#include <iostream>
#include <string>

namespace boost = boost153;


uint64_t try_parse(std::string str) {
    try {
        return boost::lexical_cast<uint64_t>(str);
    } catch (boost::bad_lexical_cast& ex) {
        std::cout << str << ": " << ex.what() << std::endl;
    }
    return 0;
}

int main()
{
    uint64_t id;
    id = try_parse("232456454545455445454534645656");
    id = try_parse("bb");
    id = try_parse("324bb");
    id = try_parse("3.4");
    id = try_parse("+1");
    id = try_parse("-134");
    id = try_parse("255");
    std::cout << id << std::endl;
    return 0;
}
