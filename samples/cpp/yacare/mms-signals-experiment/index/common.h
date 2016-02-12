#include <yandex/maps/mms/map.h>
#include <yandex/maps/mms/string.h>

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

struct Address
{
    unsigned long offset;
    size_t size;
};

template<class P>
struct Index {

    Index() {}

    mms::map<P, Tile, Address> tiles;

    template<class A> void traverseFields(A a) const {
        a(tiles);
    }
};
