#include <yandex/maps/xml3/xml.h>

#include <iostream>
#include <string>


namespace mx = maps::xml3;

int main()
{
    std::string rawXml =
R"(<?xml version="1.0" encoding="utf-8"?>
<post obj="backa.pkostikov/test" id="0:64351b2d813d...c8cf05e39956" groups="2" size="9" key="677/pkostikov/test">
<complete addr="93.158.130.172:1025" path="/srv/storage/15/1/data-0.4" group="677" status="0"/>
<complete addr="95.108.223.236:1025" path="/srv/storage/6/2/data-0.4" group="1143" status="0"/>
<written>2</written>
</post>)";

    std::cout << rawXml << std::endl;

    mx::Doc doc(rawXml, mx::Doc::String);
    std::cout << doc.node("/post").attr<std::string>("key") << std::endl;
    return 0;
}
