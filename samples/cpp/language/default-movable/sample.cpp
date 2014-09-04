#include <iostream>
#include <string>

class A {
public:
    A(int n): n_(n) {}
    A& operator=(A&& a) noexcept = default;

    int n() const {return n_;}
private:
    int n_;
};

int main()
{
    A a(1);
    a = A(2);
    std::cout << a.n() << std::endl;
    return 0;
}
