#include <iostream>
#include <string>
#include <vector>
#include <fstream>


class A
{
public:
    A() {
        std::cout << "constructor" << std::endl;
    }
    ~A() {
        std::cout << "destructor" << std::endl;
    }
 };

int main()
{
    try {
        A a;
        std::cout << "before throw" << std::endl;
        throw "hello";
    } catch (...) {
        std::cout << "catched" << std::endl;
        return 1;
    }
    return 0;
}
