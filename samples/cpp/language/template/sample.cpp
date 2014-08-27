#include <iostream>
#include <string>

template<class T>
std::string some(const T& t1, const T& t2) {
    return t1 > t2 ? "true" : "false";
}

template<>
std::string some(const double& t1, const double& t2) {
    return t1 > t2 ? "true d" : "false d";
}


// template<class T>
// T getSome() {
//     return T(0);
// }

template<class T>
T getSome();


template<>
double getSome() {
    return 3.14;
}

int main()
{
    // std::cout << some(1, 2) << std::endl;
    // std::cout << some<double>(1., 2.) << std::endl;
    std::cout << getSome<double>() << std::endl;
    // std::cout << getSome<int>() << std::endl;
    return 0;
}
