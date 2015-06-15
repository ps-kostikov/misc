#include "tiff.h"

#include <boost/filesystem.hpp>

#include <memory>
#include <iostream>


Tiff::Tiff(const std::string& path) : path_(path)
{
    TIFFSetWarningHandler(NULL);
    // libtiff usually fails trying opening absent file
    if (!boost::filesystem::exists(path)) {
        throw TiffException() << "no such file " << path;
    }
    tiff_ = TIFFOpen(path.c_str(), "r");
    if (!tiff_) {
        throw TiffException() << "fail opening tiff file " << path;
    }
    tileDataSize_ = countTileDataSize();
}

size_t
Tiff::countTileDataSize() const
{
    return (tileHeight() * tileWidth() * samplesPerPixel() * bitsPerSample()) / 8;
}

bool
Tiff::isTiled() const
{
    return TIFFIsTiled(tiff_) != 0;
}

unsigned
Tiff::tileSize() const
{
    return TIFFTileSize(tiff_);
}

std::string
Tiff::readTile(unsigned x, unsigned y) const
{
    std::unique_ptr<char> data(new char[tileDataSize_]);
    int readBytes = TIFFReadTile(tiff_, static_cast<void*>(data.get()), x, y, 0, 0);
    if (readBytes <= 0) {
        throw TiffException() << "error reading tile ("
                << x << ", " << y << ") from " << path_;
    }
    return std::string(data.get(), static_cast<unsigned>(readBytes));
}

void
Tiff::checkTile(unsigned x, unsigned y) const
{
    static std::unique_ptr<char> data(new char[tileDataSize_]);
    int readBytes = TIFFReadTile(tiff_, static_cast<void*>(data.get()), x, y, 0, 0);
    if (readBytes <= 0) {
        throw TiffException() << "error reading tile ("
                << x << ", " << y << ") from " << path_;
    }
}

void
Tiff::checkAllTiles() const
{
    for (unsigned i = 0; i < imageWidth(); i += tileWidth()) {
        for (unsigned j = 0; j < imageHeight(); j += tileHeight()) {
            checkTile(i, j);
        }
    }
}

unsigned
Tiff::bitsPerSample() const
{
    return getValue<unsigned>(TIFFTAG_BITSPERSAMPLE);
}

unsigned
Tiff::samplesPerPixel() const
{
    return getValue<unsigned>(TIFFTAG_SAMPLESPERPIXEL);
}

unsigned
Tiff::imageWidth() const
{
    return getValue<unsigned>(TIFFTAG_IMAGEWIDTH);
}

unsigned
Tiff::imageHeight() const
{
    return getValue<unsigned>(TIFFTAG_IMAGELENGTH);
}

unsigned
Tiff::tileWidth() const
{
    return getValue<unsigned>(TIFFTAG_TILEWIDTH);
}

unsigned
Tiff::tileHeight() const
{
    return getValue<unsigned>(TIFFTAG_TILELENGTH);
}

Tiff::~Tiff()
{
    TIFFClose(tiff_);
}

