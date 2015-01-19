#include <iostream>
#include <fstream>


int main()
{
    std::ifstream is("sample.txt");
    int i1, i2;
    std::string s1, s2;
    while(is >> i1 >> i2 >> s1 >> s2) {
        std::cout << i1 << " " << i2 << " " << s1 << " " << s2 << std::endl;
    }

    return 0;
}
