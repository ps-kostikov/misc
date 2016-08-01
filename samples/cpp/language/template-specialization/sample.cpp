#include <iostream>
#include <string>


template<class T>
struct A
{
    std::string v;

    void print() {
        std::cout << "default print: " << v << std::endl;
    }
};

struct Base
{
    std::string v;

    void print() {
        std::cout << "custom print: " << v << std::endl;
    }
};

template<>
struct A<std::string>: public Base
{};

template<>
struct A<double>: public Base
{};

// template<>
// struct A<std::string>
// {
//     std::string v;

//     void print() {
//         std::cout << "custom print: " << v << std::endl;
//     }
// };

// template<>
// struct A<double>: public A<std::string>
// {

// };

int main()
{
    A<int>{"int"}.print();
    Base{"base"}.print();
    A<std::string> s;
    s.v = "string";
    s.print();
    A<double> d;
    d.v = "double";
    d.print();
    return 0;
}
