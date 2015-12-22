#include <iostream>
#include <boost/ptr_container/ptr_map.hpp>
#include <string>

class A
{
public:
    explicit A(int i): i_(i) {}
private:
    int i_;
};


int main()
{
    boost::ptr_map<int, A> map;
    // map.insert(1, new A(3)); // doesn't compile
    int i = 1;
    map.insert(i, new A(3));
    return 0;
}
