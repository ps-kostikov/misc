#include <type_traits>
#include <iostream>

template<class T, typename std::enable_if<std::is_enum<T>::value, bool>::type = true>
void f()
{
    std::cout << "enum" << std::endl;
}

template<class T, typename std::enable_if<!std::is_enum<T>::value, bool>::type = true>
void f() {
    std::cout << "not enum" << std::endl;
}

enum class E
{
};

int main()
{
    f<E>();
    return 1;
}