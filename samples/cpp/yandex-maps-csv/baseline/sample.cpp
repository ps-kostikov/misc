#include <yandex/maps/csv.h>

#include <iostream>
#include <string>

namespace csv = maps::csv;

int main()
{
    std::cout << "hello" << std::endl;

    csv::OutputStream csvOut(std::cout);

    csvOut << 1 << 2 << 3 << csv::endl;
    csvOut << "a" << "b" << "c" << csv::endl;
    csvOut << "ha=ha" << "hi,hi" << "ho..,\tN\"" << csv::endl;

    return 0;
}