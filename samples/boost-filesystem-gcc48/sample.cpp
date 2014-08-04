#include <boost153/filesystem.hpp>
#include <iostream>

namespace fs = boost153::filesystem;

int main()
{
    std::cout << fs::path("data") / fs::path("company") << std::endl;

    fs::path path("sample");
    std::cout << fs::exists(path) << std::endl;
    return 0;
}
