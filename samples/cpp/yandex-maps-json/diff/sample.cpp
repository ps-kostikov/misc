#include <yandex/maps/json/builder.h>
#include <yandex/maps/json/value.h>
#include <yandex/maps/json/std.h>


#include <iostream>
#include <string>
#include <vector>
#include <exception>


namespace mj = maps::json;


std::string unifyJson(const std::string& json) {
    mj::Value v = mj::Value::fromString(json);
    return (mj::Builder() << v).str();
}

void assertTrue(bool condition, std::string msg) {
    if (!condition) {
        throw std::runtime_error(msg);
    }
}

void assertJsonEqual(const mj::Value& v1, const mj::Value& v2);

void assertJsonObjectsEqual(const mj::Value& v1, const mj::Value& v2) {
    assertTrue(v1.size() == v2.size(), "size mismatch");
    for (auto f : v1.fields()) {
        assertTrue(v2.hasField(f), "field absence");
        assertJsonEqual(v1[f], v2[f]);
    }
}

void assertJsonArraysEqual(const mj::Value& v1, const mj::Value& v2) {
    assertTrue(v1.size() == v2.size(), "size mismatch");
    for (auto f1 = v1.begin(), f2 = v2.begin(); f1 != v1.end(); ++f1, ++f2) {
        assertJsonEqual(*f1, *f2);
    }
}

void assertJsonEqual(const mj::Value& v1, const mj::Value& v2) {
    if (v1.isObject() && v2.isObject()) {
        assertJsonObjectsEqual(v1, v2);
    } else if (v1.isArray() && v2.isArray()) {
        assertJsonArraysEqual(v1, v2);
    } else if (v1.isString() and v2.isString()) {
        assertTrue(v1.as<std::string>() == v2.as<std::string>(), "strings are not equal");
    } else if (v1.isInteger() and v2.isInteger()) {
        assertTrue(v1.as<int64_t>() == v2.as<int64_t>(), "integers are not equal");
    } else if (v1.isFloating() and v2.isFloating()) {
        assertTrue(v1.as<double>() == v2.as<double>(), "doubles are not equal");
    } else if (v1.isBool() and v2.isBool()) {
        assertTrue(v1.as<bool>() == v2.as<bool>(), "bools are not equal");
    } else if (v1.isNull() and v2.isNull()) {
        return;
    } else {
        assertTrue(false, "type mismatch");
    }
}

void assertJsonEqual(const std::string& json1, const std::string& json2) {
    mj::Value v1 = mj::Value::fromString(json1);
    mj::Value v2 = mj::Value::fromString(json2);
    assertJsonEqual(v1, v2);
}

int main()
{
    std::string json1 = "{ \
        \"i\": 10, \
        \"f\": 11.1, \
        \"s\": \"a string\", \
        \"o\": { \
            \"s2\": \"second string\", \
            \"b\": true \
        }, \
        \"a\": [ \
            { \"s3\": \"a[0].s3\" }, \
            { \"s3\": \"a[1].s3\" } \
        ], \
        \"d\": { \
            \"k1\": { \"s4\": \"d[k1].s4\" }, \
            \"k2\": { \"s4\": \"d[k2].s4\" } \
        } \
    }";
    std::string json2 = "{ \
        \"i\": 10, \
        \"f\": 11.1, \
        \"s\": \"a string\", \
        \"o\": { \
            \"s2\": \"second string\", \
            \"b\": true \
        }, \
        \"a\": [ \
            { \"s3\": \"a[0].s3\" }, \
            { \"s3\": \"a[1].s3\" } \
        ], \
        \"d\": { \
            \"k1\": { \"s4\": \"d[k1].s4\" }, \
            \"k2\": { \"s4\": \"d[k2].s4\" } \
        }, \
        \"m\": 1 \
    }";


    // std::cout << ((unifyJson(json1) == unifyJson(json2)) ? "yes" : "no") << std::endl;
    try {
        assertJsonEqual(json1, json2);
    } catch (std::exception& ex) {
        std::cout << ex.what() << std::endl;
    }
    return 0;
}
