#include "common.h"

#include <map>
#include <sstream>
#include <iostream>
#include <string>
#include <fstream>


int main()
{
    unsigned long currentPos = 0;

    Index<mms::Standalone> index;
    std::ofstream dataF("layer.data");
    const size_t size = 2019;

    for (std::string line; std::getline(std::cin, line); ) {
        std::stringstream ss(line);
        size_t x, y, z;
        ss >> x >> y >> z;

        index.tiles[Tile{x, y, z}] = Address{currentPos, size};

        std::stringstream headSS;
        headSS << x << " " << y << " " << z << " ";
        auto head = headSS.str();
        dataF << head << std::string(size - head.size(), 'x');

        currentPos += size;
    }

    std::cout << "number of tiles = " << index.tiles.size() << std::endl;

    std::ofstream out("index.mms");
    mms::Writer w(out);
    size_t ofs = mms::safeWrite(w, index);
    std::cout << "index file size = " << ofs << std::endl;
    out.close();


    return 0;
}
