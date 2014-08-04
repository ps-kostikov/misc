#include <iostream>
#include <algorithm>
#include <vector>
#include <map>

class A
{
public:
	virtual int f() = 0;
	virtual ~A() {}
};


class B: public A
{
public:
	int f() {return 1;}
	~B() {}
};


int main()
{
	B b;
	std::cout << b.f() << std::endl;
}
