#include <yandex/maps/http2.h>

#include <iostream>
#include <string>

int main()
{
    maps::http2::Client httpclient;
    // maps::http2::URL url("http://ya.ru");
    maps::http2::URL url("http://ya.ru/index.html");
    maps::http2::Request request(httpclient, "GET", url);
    auto response = request.perform();
    std::cout << "Status: " << response.status() << std::endl;
    std::istreambuf_iterator<char> eos;
    std::string body(std::istreambuf_iterator<char>(response.body()), eos);
    // std::string body;
    // response.body() >> body;
    std::cout << body << std::endl;
    return 0;
}
