#include <iostream>
#include <map>
#include <string>
#include <vector>



int main()
{
    std::cout << "hello world" << std::endl;

    std::map<std::string, std::vector<std::string>> m;

    m["A"].push_back("a");
    m["A"].push_back("b");
    m["A"].push_back("c");

    std::for_each(m.begin(), m.end(), [](const std::pair<std::string, std::vector<std::string>>& p){
        std::cout << p.first << ": ";
        for (const auto& s: p.second) {
            std::cout << s << " ";
        }
        std::cout << std::endl;
    });
    return 0;
}
