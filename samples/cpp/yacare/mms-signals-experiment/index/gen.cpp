#include "common.h"

#include <map>
#include <sstream>
#include <iostream>
#include <string>
#include <fstream>


int main()
{
    std::map<Tile, Address> index;
    unsigned long currentPos = 0;

    std::ofstream indexF("layer.index");
    std::ofstream dataF("layer.data");
    const size_t size = 2019;

    for (std::string line; std::getline(std::cin, line); ) {
        std::stringstream ss(line);
        size_t x, y, z;
        ss >> x >> y >> z;

        index[Tile{x, y, z}] = Address{currentPos, size};
        indexF << x << " " << y << " " << z << " " << currentPos << " " << size << std::endl;

        std::stringstream headSS;
        headSS << x << " " << y << " " << z << " ";
        auto head = headSS.str();
        dataF << head << std::string(size - head.size(), 'x');

        currentPos += size;
    }

    std::cout << "number of tiles = " << index.size() << std::endl;


    return 0;
}
