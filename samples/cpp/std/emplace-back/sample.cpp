#include <iostream>
#include <vector>
#include <string>


struct A {
    std::string s;
    int i;
};

int main()
{
    std::cout << "hello world" << std::endl;
    std::vector<A> v;
    
    v.push_back(A{"a1", 1});

    // v.emplace_back("a2", 2);

    v.emplace_back(A{"a3", 3});
    
    return 0;
}
