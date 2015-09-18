#include <yandex/maps/xml3/xml.h>

#include <iostream>
#include <string>
#include <vector>


namespace xml3 = maps::xml3;

int main()
{
    std::vector<std::string> versions{
        "1.0.0",
        "2.1.0",
        "3.0.2"
    };

    xml3::Doc doc(xml3::Doc::create("versions"));
    xml3::Node root = doc.root();
    for (const auto& version: versions) {
        auto versionNode = root.addChild("version", version);
        versionNode.addChild("int", "false");
    }

    std::string xmlStr;
    doc.save(xmlStr);
    std::cout << xmlStr << std::endl;

    std::cout << 3 / double(2) << std::endl;
    return 0;
}
