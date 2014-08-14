#include <iostream>
#include <map>
#include <string>
#include <algorithm>


void print(const std::map<int, std::string>& m) {
    std::for_each(m.begin(), m.end(), [](const std::pair<int, std::string>& p){
        std::cout << p.first << ": " << p.second << std::endl;
    });
    std::cout << std::endl;
}

int main()
{
    std::cout << "hello world" << std::endl;

    std::map<int, std::string> m;
    m[1] = "a";
    m[2] = "b";
    m[3] = "c";

    print(m);

    m.erase(2);
    print(m);
    return 0;
}
