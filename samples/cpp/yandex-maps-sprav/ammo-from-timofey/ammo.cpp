#include "ammo.h"

#include <yandex/maps/mining/sprav2miningson.h>


namespace model = maps::sprav::model;
namespace algorithms = maps::sprav::algorithms;


MiningsonReader::MiningsonReader(const std::string& filename)
{
    Py_Initialize();
    auto package = boost153::python::import("yandex.maps.mining.miningson");
    reader_ = package.attr("card_reader")(filename);
}

boost153::optional<boost153::python::object>
MiningsonReader::next()
{
    try {
        return reader_.attr("next")();
    } catch (...) {
        return boost153::none;
    }
}

MiningsonWriter::MiningsonWriter(const std::string& filename)
{
    Py_Initialize();
    auto package = boost153::python::import("yandex.maps.mining.miningson");
    writer_ = package.attr("Writer")(filename);
    writer_.attr("__enter__")();   
}

MiningsonWriter::~MiningsonWriter()
{
    writer_.attr("__exit__")(0, 0, 0);
}

void
MiningsonWriter::write(const boost153::python::object& card)
{
    writer_.attr("write")(card);
}


AmmoReader::AmmoReader(const std::string& filename,
                   boost153::optional<size_t> offsetNumber):
    reader_(filename)
{
    if (offsetNumber) {
        for (size_t i = 0; i < *offsetNumber; ++i) {
            reader_.next();
        }
    }
}

boost153::optional<Ammo>
AmmoReader::next()
{
    auto card = reader_.next();
    if (!card) {
        return boost153::none;
    }

    std::string ammoId = boost153::python::extract<std::string>(
        boost153::python::str(card->attr("debug")["ammo_id"]));

    std::vector<model::ID> rightAnswersIds;
    auto rightIdsList = card->attr("debug")["right_ids"];
    for (long i = 0; i < boost153::python::len(rightIdsList); ++i) {
        rightAnswersIds.push_back(
            boost153::python::extract<model::ID>(rightIdsList[i]));
    }

    algorithms::Company algCompany = maps::mining::sprav2miningson::toSpravCompany(*card);
    return Ammo{ammoId, algCompany, rightAnswersIds};
}


AmmoWriter::AmmoWriter(const std::string& filename):
    writer_(filename)
{}

void
AmmoWriter::write(const Ammo& ammo)
{
    auto card = maps::mining::sprav2miningson::toMiningsonCard(ammo.algCompany);
    boost153::python::list rightAnswersIdsList;
    for (auto id: ammo.rightAnswersIds) {
        rightAnswersIdsList.append(id);
    }
    card.attr("debug")["right_ids"] = rightAnswersIdsList;
    card.attr("debug")["ammo_id"] = ammo.id;
    writer_.write(card);
}
