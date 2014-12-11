#include <yandex/maps/sprav/matching/matching.h>
#include <yandex/maps/sprav/model/storages/revision.h>
#include <yandex/maps/sprav/model/convert.h>
#include <yandex/maps/sprav/common/config.h>
#include <yandex/maps/sprav/common/stopwatch.h>
#include <yandex/maps/sprav/algorithms/datamodel/company.h>
#include <yandex/maps/sprav/algorithms/similarity/similarity.h>

#include <yandex/maps/mining/sprav2miningson.h>

#include <yandex/maps/log.h>
#include <yandex/maps/pgpool2/pgpool.h>
#include <yandex/maps/common/exception.h>
#include <yandex/maps/json/builder.h>
#include <yandex/maps/json/value.h>
#include <yandex/maps/json/std.h>
#include <yandex/maps/geolib3/conversion.h>
#include <yandex/maps/threadpool/threadpool.h>

#include <boost153/program_options.hpp>
#include <boost153/python/object.hpp>
#include <boost153/python.hpp>
#include <boost153/algorithm/string.hpp>
#include <boost153/optional.hpp>

#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <thread>
#include <exception>


namespace json = maps::json;
namespace sprav = maps::sprav;
namespace model = maps::sprav::model;
namespace algorithms = maps::sprav::algorithms;



int main(int argc, char* argv[])
{
    maps::log::setLevel("DEBUG");

    namespace bpo = boost153::program_options;

    bpo::options_description desc("Usage");
    desc.add_options()
        ("help,h", "Print this help message and exit")
        ("miningson,m", bpo::value<std::string>(), "input filename")
    ;

    bpo::variables_map vars;
    bpo::store(
        bpo::parse_command_line(argc, argv, desc),
        vars
    );
    bpo::notify(vars);

    if (vars.count("help") > 0 ||
        vars.count("miningson") == 0)
    {
        std::cout << desc << std::endl;
        return 0;
    }

    std::string miningsonFilename(vars["miningson"].as<std::string>());
    
    Py_Initialize();
    auto package = boost153::python::import("yandex.maps.mining.miningson");
    boost153::python::object reader = package.attr("card_reader")(miningsonFilename);
    boost153::python::object card;
    try {
        card = reader.attr("next")();
        std::cout << "next done" << std::endl;
    } catch (...) {
        std::cout << "next not done" << std::endl;
    }

    std::string cardId;

    try {
        cardId = boost153::python::extract<std::string>(
            card.attr("raw_origin")["artner_card_id"].attr("__str__")());
        std::cout << cardId << std::endl;
    } catch (...) {
        std::cout << "fetch card id fail" << std::endl;
    }

    try {
        card = reader.attr("next")();
        std::cout << "next done" << std::endl;
    } catch (...) {
        std::cout << "next not done" << std::endl;
    }

    return 0;
}
