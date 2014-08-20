#include <boost153/lexical_cast.hpp>
#include <iostream>
#include <string>
#include <map>

namespace boost = boost153;


enum Digit {
    One,
    Two,
    Three
};

std::ostream& operator<<(std::ostream& o, const Digit& d) {
    static std::map<Digit, std::string> digit_to_str{
        {Digit::One, "one"},
        {Digit::Two, "two"},
        {Digit::Three, "three"}
    };

    o << digit_to_str[d];
    return o;
}

std::istream& operator>>(std::istream& i, Digit& d) {
    static std::map<std::string, Digit> str_to_digit{
        {"one", Digit::One},
        {"two", Digit::Two},
        {"three", Digit::Three}
    };

    std::string s;
    i >> s;
    auto f = str_to_digit.find(s);
    if (f == str_to_digit.end()) {
        i.setstate(std::istream::failbit);
    } else {
        d = f->second;
    }
    return i;
}

int main()
{   
    Digit d = Digit::One;
    std::cout << d << std::endl;
    std::cout << boost::lexical_cast<std::string>(d) << std::endl;
    std::cout << boost::lexical_cast<Digit>(std::string("three")) << std::endl;
    std::cout << boost::lexical_cast<Digit>(std::string("to")) << std::endl;
    return 0;
}
