#include <boost153/locale.hpp>

#include <vector>
#include <locale>
#include <iostream>

void split(const std::string& text, const std::string& loc)
{
    using namespace boost153::locale::boundary;
    boost153::locale::generator gen;

    // Create mapping of text for token iterator using global locale.
    ssegment_index map(word, text.begin(), text.end(), gen(loc)); 
    // ssegment_index map(word, text.begin(), text.end(), std::locale(loc)); 
    // ssegment_index map(word, text.begin(), text.end(), std::locale("en_US.UTF-8")); 

    map.rule(boost153::locale::boundary::word_any);

    // Print all "words" -- chunks of word boundary
    for(ssegment_index::iterator it = map.begin(), e = map.end(); it != e; ++it) {
        std::cout << *it << " (len = " << it->length() << ")" << std::endl;
    }
    std::cout << std::endl;
}

int main(int, char **)
{
    // std::string locale = "en.UTF-8";
    // std::string locale = "en_US.UTF-8";
    std::string locale = "UTF-8";
    // std::string locale = "ru_RU.UTF-8";
    // std::string locale = "blah.UTF-8";
    // std::string locale = ".UTF-8";
    // std::string locale = "en_US";
    split(
        "To be or not to be, that is the question. isn't ",
        locale
    );
    split(
        u8"Психиатрическ'ая больница №1 (3к)",
        locale
    );
    split(
        u8"مستشفى الطب النفسي №1 (3K)",
        locale
    );
    split(
        u8"~Ёмоза-Йк",
        locale
    );
    return 0;
}