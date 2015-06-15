#pragma once

#include <yandex/maps/common/exception.h>
#include <boost/noncopyable.hpp>

#include <tiffio.h>
#include <mutex>
#include <string>


class TiffException : public maps::RuntimeError {
public:
    TiffException() : RuntimeError()
    {}

    explicit TiffException(const std::string& what)
        : RuntimeError(what)
    {}
};

/*
    class Tiff encapsulates handling files in tiff format via libtiff
*/
class Tiff: boost::noncopyable
{
public:
    Tiff(const std::string& path);
    ~Tiff();

    bool isTiled() const;
    unsigned tileSize() const;
    std::string getTile(unsigned x, unsigned y) const;

    unsigned bitsPerSample() const;
    unsigned samplesPerPixel() const;
    unsigned imageWidth() const;
    unsigned imageHeight() const;
    unsigned tileWidth() const;
    unsigned tileHeight() const;

    std::string path() const {return path_;}

    void checkAllTiles() const;
    void checkTile(unsigned x, unsigned y) const;
    std::string readTile(unsigned x, unsigned y) const;

private:
    size_t countTileDataSize() const;

    template<typename T>
    T getValue(ttag_t tag) const
    {
        T result;
        TIFFGetField(tiff_, tag, &result);
        return result;
    }

    std::string path_;
    TIFF *tiff_;
    size_t tileDataSize_;

};
