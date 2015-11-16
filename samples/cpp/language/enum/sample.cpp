#include <iostream>

enum Type {
    Type1,
    Type2,
    Type3,
};

enum Enum {
    Enum1 = 1,
    Enum2 = 2,
    Enum3 = 3,
};

int main()
{
    std::cout << "hello world" << std::endl;
    std::cout << "Type1: " << Type::Type1 << std::endl;
    std::cout << "Type2: " << Type::Type2 << std::endl;
    std::cout << "Type3: " << Type::Type3 << std::endl;

    std::cout << "Enum1: " << Enum::Enum1 << std::endl;
    std::cout << "Enum2: " << Enum::Enum2 << std::endl;
    std::cout << "Enum3: " << Enum::Enum3 << std::endl;

    return 0;
}
