#include <iostream>
#include <chrono>
#include <ctime>


typedef std::chrono::time_point<
    std::chrono::system_clock,
    std::chrono::nanoseconds
> TimePoint;

template<class TimePointType>
void print(const TimePointType& tp)
{
    std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    std::cout << std::ctime(&tt) << std::endl;
}

int main()
{
    auto tp = std::chrono::system_clock::from_time_t(1424947270);

    print(tp);
    // std::time_t tt = std::chrono::system_clock::to_time_t(tp);
    // std::cout << std::ctime(&tt) << std::endl;


    TimePoint nowTP = std::chrono::system_clock::now();
    print(nowTP);
    print(nowTP - std::chrono::hours(24));

    print(std::min(nowTP, nowTP + std::chrono::hours(24)));
    return 0;
}
