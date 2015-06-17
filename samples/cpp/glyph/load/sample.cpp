#include <iostream>
#include <sstream>
#include <fstream>

#include <yandex/maps/fontutils/glyph.h>
#include <yandex/maps/proto/vector-data/glyphs.pb.h>


namespace glyphs = yandex::maps::proto::vector_data::glyphs;


#include <yandex/maps/mms/vector.h>
#include <yandex/maps/mms/map.h>
#include <yandex/maps/mms/string.h>

template<class P>
struct SdfFont {

    SdfFont() {}
    SdfFont(const std::string& fp): fontProto(fp) {}

    mms::string<P> fontProto;
    mms::map<P, size_t, mms::string<P>> glyphsProto;

    template<class A> void traverseFields(A a) const {
        a(fontProto, glyphsProto);
    }
};

template<class Proto, class Data>
Proto parse(const Data& data)
{
    std::istringstream stream(data);
    Proto proto;
    proto.ParseFromIstream(&stream);
    return proto;
}


int main(int /*argc*/, char** argv)
{
    std::ifstream f(argv[1]);
    std::string data((std::istreambuf_iterator<char>(f)),
                     std::istreambuf_iterator<char>());
    auto sdfFont = mms::safeCast<SdfFont<mms::Mmapped>>(data.data(), data.size());

    auto font = parse<glyphs::FontDescription>(sdfFont.fontProto);
    std::cout << "font id = " << font.fontid() << std::endl;
    std::cout << "font xheight = " << font.xheight() << std::endl;
    std::cout << "font margin = " << font.margin() << std::endl;
    std::cout << std::endl;

    for (auto it: sdfFont.glyphsProto) {
        auto glyph = parse<glyphs::Glyph>(it.second);
        std::cout << "glyph id = " << glyph.id() << std::endl;
        std::cout << "glyph size of bitmap = " << glyph.bitmap().size() << std::endl;
        std::cout << "glyph width = " << glyph.width() << std::endl;
        std::cout << "glyph height = " << glyph.height() << std::endl;
        std::cout << "glyph bearingX = " << glyph.bearingx() << std::endl;
        std::cout << "glyph bearingY = " << glyph.bearingy() << std::endl;
        std::cout << "glyph advance = " << glyph.advance() << std::endl;

        std::cout << std::endl;
    }

    return 0;
}
