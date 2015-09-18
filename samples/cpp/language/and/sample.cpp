#include <iostream>
#include <vector>

int main()
{
    std::vector<int> v;
    std::cout << "hello world" << std::endl;
    std::cout << v.empty() << std::endl;
    if (!v.empty() && v.front() > 3) {
        std::cout << "hello" << std::endl;
    } else {
        std::cout << "bye" << std::endl;
    }
    return 0;
}
