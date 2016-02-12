#include <yandex/maps/mms/map.h>
#include <yandex/maps/mms/string.h>

#include <boost/functional/hash.hpp>

#include <cstddef>

struct Tile
{
    size_t x;
    size_t y;
    size_t z;
};

bool operator<(const Tile& left, const Tile& right)
{
    if (left.x != right.x) {
        return left.x < right.x;
    } else if (left.y != right.y) {
        return left.y < right.y;
    }
    return left.z < right.z;
}

struct TileHash
{
    size_t operator()(const Tile& tile) const {
        size_t seed = 0;
        boost::hash_combine(seed, tile.x);
        boost::hash_combine(seed, tile.y);
        boost::hash_combine(seed, tile.z);
        return seed;
    }
};

/*
namespace std {
    template<>
    struct hash<Tile> : public TileHash {}; 
}
*/

struct Address
{
    unsigned long offset;
    size_t size;
};

bool operator==(const Tile& left, const Tile& right)
{
    return (left.x == right.x) and (left.y == right.y) and (left.z == right.z);
}

template<class P>
struct Index {

    Index() {}

    mms::map<P, Tile, Address> tiles;

    template<class A> void traverseFields(A a) const {
        a(tiles);
    }
};
