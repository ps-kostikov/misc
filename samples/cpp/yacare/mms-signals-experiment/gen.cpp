#include "common.h"

#include <iostream>
#include <string>
#include <fstream>



int main()
{

    Layer<mms::Standalone> layer;
    std::string data(2019, 'x');

    for (std::string line; std::getline(std::cin, line); ) {
        std::stringstream ss(line);
        size_t x, y, z;
        ss >> x >> y >> z;
        layer.tiles[Tile{x, y, z}] = data;
        // std::cout << line << std::endl;
    }

    std::cout << "number of tiles = " << layer.tiles.size() << std::endl;

    std::ofstream out("layer.mms");
    mms::Writer w(out);
    size_t ofs = mms::safeWrite(w, layer);
    std::cout << "file size = " << ofs << std::endl;
    out.close();

    return 0;
}
