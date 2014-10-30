#include <iostream>
#include <string>

template<class ToType, class FromType>
ToType convert(const FromType& f)
{
    std::cout << "convert from " << f << std::endl;
    return ToType();
}

template<>
double convert<double, std::string>(const std::string& s)
{
    std::cout << "special convert from " << s << std::endl;
    return 3.;
}

int main()
{
    convert<int, std::string>("haha");
    convert<double>("hihi");
    return 0;
}
