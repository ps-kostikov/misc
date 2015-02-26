#include <iostream>
#include <chrono>
#include <ctime>

int main()
{
    auto tp = std::chrono::system_clock::from_time_t(1424947270);
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::cout << std::ctime(&tt) << std::endl;
    return 0;
}
