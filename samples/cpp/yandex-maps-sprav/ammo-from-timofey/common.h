#pragma once

#include <yandex/maps/sprav/model/storages/revision.h>

#include <yandex/maps/pgpool2/pgpool.h>
#include <boost153/optional.hpp>

#include <vector>
#include <map>
#include <type_traits>


struct ReadSessionHolder {
    ReadSessionHolder(maps::pgpool2::ConnectionMgr& connMgr) :
        transaction(connMgr.readTransaction()),
        storage(new maps::sprav::model::RevisionStorage(*transaction)),
        session(storage, maps::sprav::model::Session::CachingPolicy::CacheAll)
    {}

    maps::pgpool2::PgTransaction transaction;
    std::shared_ptr<maps::sprav::model::RevisionStorage> storage;
    maps::sprav::model::Session session;
};

template<class T>
double calculateAverage(const std::vector<T>& values)
{
    double sum = 0.;
    if (values.empty()) {
        return sum;
    }
    for (auto value: values) {
        sum += value;
    }
    return sum / values.size();
}

template<class Container, class ValueFromElem>
double calculateAverage(const Container& elems, ValueFromElem f)
{
    typedef typename Container::value_type ContainerValueType;
    typedef typename std::result_of<ValueFromElem(const ContainerValueType&)>::type ValueType;
    std::vector<ValueType> values;
    for (const auto& elem: elems) {
        values.push_back(f(elem));
    }
    return calculateAverage(values);
}


template<class T>
double calculateMedian(const std::vector<T>& values)
{
    if (values.empty()) {
        return 0;
    }
    std::vector<T> sortedValues(values.begin(), values.end());
    std::sort(sortedValues.begin(), sortedValues.end());
    if (sortedValues.size() % 2) {
        return sortedValues[sortedValues.size() / 2];
    } else {
        return (sortedValues[sortedValues.size() / 2 - 1] +
            sortedValues[sortedValues.size() / 2]) / 2.;
    }
}

template<class Container, class ValueFromElem>
double calculateMedian(const Container& elems, ValueFromElem f)
{
    typedef typename Container::value_type ContainerValueType;
    typedef typename std::result_of<ValueFromElem(const ContainerValueType&)>::type ValueType;
    std::vector<ValueType> values;
    for (const auto& elem: elems) {
        values.push_back(f(elem));
    }
    return calculateMedian(values);
}