#pragma once

// FIXME pkostikov: rename to ammo.h (or io.h or ammo_io.h)

#include <yandex/maps/sprav/model/storages/revision.h>
#include <yandex/maps/sprav/algorithms/datamodel/company.h>

#include <yandex/maps/common/exception.h>

#include <boost153/python/object.hpp>
#include <boost153/python.hpp>
#include <boost153/optional.hpp>

#include <string>
#include <vector>


class MiningsonReader
{
public:
    MiningsonReader(const std::string& filename);
    boost153::optional<boost153::python::object> next();

private:
    boost153::python::object reader_;
};

class MiningsonWriter
{
public:
    MiningsonWriter(const std::string& filename);
    ~MiningsonWriter();
    void write(const boost153::python::object& card);

private:
    boost153::python::object writer_;
};

struct Ammo
{
    std::string id;
    maps::sprav::algorithms::Company algCompany;
    std::vector<maps::sprav::model::ID> rightAnswersIds;
};

typedef std::vector<Ammo> Ammos;

class AmmoReader
{
public:
    AmmoReader(const std::string& filename,
                 boost153::optional<size_t> offsetNumber);

    boost153::optional<Ammo> next();

private:
    MiningsonReader reader_;
};


class AmmoWriter
{
public:
    AmmoWriter(const std::string& filename);
    void write(const Ammo& ammo);

private:
    MiningsonWriter writer_;
};
