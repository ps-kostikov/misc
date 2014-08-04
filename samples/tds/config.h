#pragma once

#include <yandex/maps/xml3/xml.h>

using namespace maps;

/**
 * Structure that holds configuration properties.
 * Properties are read from an xml config file
 */
struct Config
{
    explicit Config(const std::string& confFileName);
    
    typedef struct {
        size_t batchSize;
        // More params to be added here
    } Param;

    const xml3::Doc doc;
    const xml3::Node root;
    const xml3::Node dbSettings;
    std::string templatesPath;
    std::string destPath;
    Param param;

private:
    void readParams();
    
    template <typename T>
    const T getParamValWithDefault
        ( const std::vector<std::string>& nodeChain
        , const T& defVal
        ) const
    {
        xml3::Node n = root;
        for (auto itr = nodeChain.begin(); itr != nodeChain.end(); ++itr) {
            if (n.isNull()) return defVal;
            n = n.node(*itr, true);
        }
        return (n.isNull() ? defVal : n.value<T>(defVal));    
    }
};


