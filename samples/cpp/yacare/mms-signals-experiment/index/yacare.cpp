#include "common.h"

#include <map>
#include <sstream>
#include <iostream>
#include <string>
#include <fstream>
#include <memory>

std::map<Tile, Address> index;
std::unique_ptr<std::ifstream> dataFPtr;

std::string lookup(const Tile& tile)
{
    auto it = index.find(tile);
    if (it == index.end()) {
        return {};
    }
    auto& address = it->second;
    dataFPtr->seekg(address.offset);
    std::string result(address.size, ' ');
    dataFPtr->read(const_cast<char*>(result.data()), address.size);
    return result;
}

int main()
{

    std::ifstream indexF("layer.index");
    for (std::string line; std::getline(indexF, line); ) {
        std::stringstream ss(line);
        size_t x, y, z;
        unsigned long offset;
        size_t size;
        ss >> x >> y >> z >> offset >> size;
        index[Tile{x, y, z}] = Address{offset, size};
    }
    std::cout << "number of tiles = " << index.size() << std::endl;

    dataFPtr.reset(new std::ifstream("layer.data"));

    std::cout << lookup(Tile{4, 5, 7}) << std::endl;

    return 0;
}
