#include <iostream>
#include <map>
#include <string>
#include <vector>


int main()
{
    std::cout << "hello world" << std::endl;

    std::map<int, std::string> m;

    m[1] = "aa";
    m[2] = "bb";
    m[3] = "cc";

    for (auto& p: m) {
        std::cout << p.first << ": " << p.second << " size = " << p.second.size() << std::endl;
    }
    std::cout << std::endl;

    std::vector<std::string> v;
    for (auto& p: m) {
        v.emplace_back(std::move(p.second));
        // v.emplace_back(p.second);
    }

    for (auto& p: m) {
        std::cout << p.first << ": " << p.second << " size = " << p.second.size() << std::endl;
    }
    std::cout << std::endl;

    for (const auto& s: v) {
        std::cout << s << std::endl;
    }
    return 0;
}
