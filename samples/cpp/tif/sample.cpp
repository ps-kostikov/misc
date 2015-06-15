#include "tiff.h"

#include <iostream>

int main(int /*argc*/, char** argv)
{
    std::cout << "checking tif " << argv[1] << std::endl;
    Tiff tiff(argv[1]);
    tiff.checkAllTiles();
    return 0;
}
