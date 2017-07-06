#include <type_traits>

#include <iostream>
#include <string>
#include <vector>
#include <boost/optional.hpp>


enum class E
{};

class A
{};


template<class T, typename std::enable_if<std::is_same<T, std::string>::value, bool>::type = true>
T f()
{
    std::cout << "string" << std::endl;
    return {};
}


template<class T, typename std::enable_if<std::is_same<T, int>::value, bool>::type = true>
T f()
{
    std::cout << "int" << std::endl;
    return {};
}


template<class T, typename std::enable_if<std::is_enum<T>::value, bool>::type = true>
T f()
{
    std::cout << "enum" << std::endl;
    return static_cast<T>(0);
}

template<class T, typename std::enable_if<std::is_same<T, A>::value, bool>::type = true>
T f()
{
    std::cout << "A" << std::endl;
    return {};
}


template <typename T>
using is_vector = std::is_same<T, std::vector<typename T::value_type>>;

template<class T, typename std::enable_if<is_vector<T>::value, bool>::type = true>
T f()
{
    std::cout << "container" << std::endl;
    f<typename T::value_type>();
    return {};
}


template <typename T>
using is_optional = std::is_same<T, boost::optional<typename T::value_type>>;

template<class T, typename std::enable_if<is_optional<T>::value, bool>::type = true>
T f()
{
    std::cout << "optional" << std::endl;
    f<typename T::value_type>();
    return {};
}


int main()
{
    f<std::string>();
    std::cout << std::endl;

    f<int>();
    std::cout << std::endl;

    f<E>();
    std::cout << std::endl;

    f<A>();
    std::cout << std::endl;

    f<std::vector<int>>();
    std::cout << std::endl;

    f<boost::optional<std::string>>();
    std::cout << std::endl;

    return 1;
}