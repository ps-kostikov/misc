#include <iostream>
#include <algorithm>
#include <vector>
#include <map>

class A
{
public:
	int i;

	A(int i): i(i) {}

    bool operator==(const A& other) const {return i == other.i;}

};


class B
{
public:
	int i;

	B(int i): i(i) {}

    bool operator==(const B& other) const = default;

};


int main()
{
	A a(1);
	A a_eq(1);

	std::cout << ((a == a_eq) ? "eq" : "not eq") << std::endl;
	
	B b(1);
	B b_eq(1);

	std::cout << ((b == b_eq) ? "eq" : "not eq") << std::endl;

    return 0;
}
