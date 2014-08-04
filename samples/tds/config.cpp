#include "config.h"

#include <yandex/maps/log/log.h>

using std::string;
using namespace maps;

Config::Config(const string& fileName)
    : doc(xml3::Doc::fromFileSafe(fileName))
    , root(doc.node("//export_config"))
    , dbSettings (root.node("database"))
{
    INFO() << "Reading configuration file";
    destPath = root.node("dest").attr<string>("path", "./output");
    templatesPath = root.node("templates").attr<string>("path", "./templates");
    readParams();
    INFO() << "Configuration file has been read";
}

void Config::readParams() {
    param.batchSize =
        getParamValWithDefault<size_t> ({"param", "batch_size"}, 10000);
}

