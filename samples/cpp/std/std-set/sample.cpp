#include <iostream>
#include <set>
#include <algorithm>

int main()
{
    std::cout << "hello world" << std::endl;
    std::set<int> s;
    s.insert(1);
    s.insert(2);
    s.insert(3);

    int max = *std::max_element(s.begin(), s.end());
    std::cout << "max = " << max << std::endl;
    return 0;
}
