#include <yandex/maps/yacare.h>

YCR_RESPOND_TO("sample:/json")
{
    response << "{\"a\": 3}";
    response["Content-Type"] = "application/json";
}

YCR_RESPOND_TO("sample:/404")
{
    throw yacare::errors::NotFound() << "not found";
}


int main(int /*argc*/, const char** /*argv*/)
{
    INFO() << "Initializing";
    yacare::run();
    INFO() << "Shutting down";
    return 0;
}