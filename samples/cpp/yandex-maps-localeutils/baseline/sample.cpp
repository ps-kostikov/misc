#include <yandex/maps/localeutils/locale.h>

#include <boost/lexical_cast.hpp>

#include <iostream>
#include <string>

int main()
{
    auto locale = boost::lexical_cast<maps::Locale>("en_US");
    std::cout << locale << std::endl;

    try {
        auto wrongLocale = boost::lexical_cast<maps::Locale>("triahah");
        std::cout << wrongLocale << std::endl;       
    } catch (const boost::bad_lexical_cast&) {
        std::cout << "bad cast" << std::endl;
    }
    return 0;
}
