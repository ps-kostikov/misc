#include <yandex/maps/sprav/matching/matching.h>
#include <yandex/maps/sprav/model/storages/revision.h>
#include <yandex/maps/sprav/model/convert.h>
#include <yandex/maps/sprav/common/config.h>
#include <yandex/maps/sprav/algorithms/datamodel/company.h>
#include <yandex/maps/sprav/algorithms/similarity/similarity.h>

#include <yandex/maps/mining/sprav2miningson.h>

#include <yandex/maps/pgpool2/pgpool.h>
#include <yandex/maps/common/exception.h>
#include <yandex/maps/json/builder.h>
#include <yandex/maps/json/value.h>
#include <yandex/maps/json/std.h>
#include <yandex/maps/geolib3/conversion.h>

#include <boost153/program_options.hpp>
#include <boost153/python/object.hpp>
#include <boost153/python.hpp>
#include <boost153/algorithm/string.hpp>

#include <fstream>
#include <string>
#include <vector>
#include <map>


namespace json = maps::json;
namespace sprav = maps::sprav;
namespace model = maps::sprav::model;
namespace algorithms = maps::sprav::algorithms;


namespace internal {

inline std::string formatCurrentPythonException()
{
    PyObject* exceptionTypePtr;
    PyObject* exceptionValuePtr;
    PyObject* tracebackPtr;
    PyErr_Fetch(&exceptionTypePtr, &exceptionValuePtr, &tracebackPtr);
    boost153::python::handle<> exceptionType(exceptionTypePtr);
    boost153::python::handle<> exceptionValue(boost153::python::allow_null(exceptionValuePtr));
    boost153::python::handle<> traceback(boost153::python::allow_null(tracebackPtr));

    const auto messageLines = boost153::python::import("traceback").attr("format_exception")(
        exceptionType, exceptionValue, traceback);
    return boost153::python::extract<std::string>(boost153::python::str().join(messageLines));
}

} // namespace internal

template <typename F>
inline void wrapPythonExceptions(F f)
{
    try {
        f();
    } catch (const boost153::python::error_already_set&) {
        const std::string pythonException = internal::formatCurrentPythonException();
        std::cout << "Python error occurred:\n" << pythonException << std::endl;
    }
}

algorithms::Company
companyFromFile(const std::string& filename)
{
    auto package = boost153::python::import("yandex.maps.mining.miningson");
    auto reader = package.attr("card_reader")(filename);
    auto card = reader.attr("next")();
    return maps::mining::sprav2miningson::toSpravCompany(card);
}

int main(int argc, char* argv[])
{

    namespace bpo = boost153::program_options;

    bpo::options_description desc("Usage");
    desc.add_options()
        ("help,h", "Print this help message and exit")
        ("card,c", bpo::value<std::string>(), "one card filename")
        ("another_card,a", bpo::value<std::string>(), "another card filename")
    ;

    bpo::variables_map vars;
    bpo::store(
        bpo::parse_command_line(argc, argv, desc),
        vars
    );
    bpo::notify(vars);

    if (vars.count("help") > 0 || vars.count("card") == 0 || vars.count("another_card") == 0) {
        std::cout << desc << std::endl;
        return 0;
    }

    std::string cardFilename1(vars["card"].as<std::string>());
    std::string cardFilename2(vars["another_card"].as<std::string>());

    Py_Initialize();

    boost153::python::register_exception_translator<maps::mining::sprav2miningson::ConversionFailedError>(
        [](const maps::mining::sprav2miningson::ConversionFailedError& exc) {
            const std::string message =
                exc.attrs_.count("pythonException") ? exc.attrs_.at("pythonException") : exc.what();
            PyErr_SetString(PyExc_ValueError, message.c_str());
        });

    wrapPythonExceptions([&]() {
        auto company1 = companyFromFile(cardFilename1);
        auto company2 = companyFromFile(cardFilename2);
        auto similarity = algorithms::calculateSimilarity(
            company1,
            company2);
        std::cout << "similarity = " << similarity << std::endl;
        for (double recall: {0.1, 0.25, 0.5, 0.75, 0.9}) {
            std::cout << "recall = " << recall
                << " sim = " << algorithms::similarityThresholdByClusterRecall(
                    company1,
                    company2, recall) << std::endl;
        }
        for (double presision: {0.1, 0.25, 0.5, 0.75, 0.9}) {
            std::cout << "presision = " << presision
                << " sim = " << algorithms::similarityThresholdByClusterPrecision(
                    company1,
                    company2, presision) << std::endl;
        }

    });

    return 0;
}
