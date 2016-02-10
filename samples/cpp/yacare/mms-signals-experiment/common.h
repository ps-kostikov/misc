#include <yandex/maps/mms/map.h>
#include <yandex/maps/mms/string.h>

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

template<class P>
struct Layer {

    Layer() {}

    mms::map<P, Tile, mms::string<P>> tiles;

    template<class A> void traverseFields(A a) const {
        a(tiles);
    }
};
