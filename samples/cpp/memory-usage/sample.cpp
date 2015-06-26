#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

struct Blob
{
    char data[1024];
};

int main()
{
    std::cout << "hello world" << std::endl;
    // std::vector<Blob> v;
    // v.reserve(20 * 1024 * 1024);
    std::vector<Blob> v1(10 * 1024 * 1024, Blob());
    std::vector<Blob> v2(10 * 1024 * 1024, Blob());
    char c;
    std::cout << "alloc 20G" << std::endl;
    std::cin >> c;
    // std::this_thread::sleep_for(std::chrono::seconds(10));
    // v.resize(10 * 1024 * 1024);
    std::vector<Blob>().swap(v2); 
    // v2.clear();
    std::cout << "resize to 10G" << std::endl;
    std::cin >> c;
    std::cout << "bye" << std::endl;
    // std::this_thread::sleep_for(std::chrono::seconds(10));
    return 0;
}
