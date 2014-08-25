#include <yandex/maps/yacare.h>

#include <iostream>

YCR_RESPOND_TO("GET sample:/path")
{
    response << "GET";
}

YCR_RESPOND_TO("POST sample:/path")
{
    response << "POST";
}

int main(int /*argc*/, const char** /*argv*/)
{
    INFO() << "Initializing";
    yacare::run();
    INFO() << "Shutting down";
    return 0;
}
