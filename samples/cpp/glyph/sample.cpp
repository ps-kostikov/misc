#include <iostream>
#include <sstream>
#include <fstream>

#include <yandex/maps/fontutils/glyph.h>
#include <yandex/maps/proto/vector-data/glyphs.pb.h>


namespace fu = maps::fontutils;

#include <yandex/maps/mms/vector.h>
#include <yandex/maps/mms/map.h>
#include <yandex/maps/mms/string.h>

template<class P>
struct SDFFont {
    // typedef mms::map<P, mms::string<P>, int, IntCmp> Map;
    // typedef mms::set<P, mms::string<P>, IntCmp > Set;
    // typedef mms::vector<P, mms::string<P> > Vector;

    SDFFont() {}
    SDFFont(const std::string& fp): fontProto(fp) {}

    mms::string<P> fontProto;
    mms::map<P, size_t, mms::string<P>> glyphsProto;

    template<class A> void traverseFields(A a) const {
        a(fontProto, glyphsProto);
    }
};



int main()
{
    std::cout << "hello world" << std::endl;

    std::string path = "/usr/share/yandex/maps/renderer5/fonts/LiberationSans-Regular.ttf";
    fu::FontFace face(path);
    std::cout << face.glyphCount() << std::endl;

    yandex::maps::proto::vector_data::glyphs::FontDescription fd;

    auto capitalX = face.render(
        face.charGlyphIndex(U'x'),
        32,
        fu::RenderingType::GrayscaleWhiteOnBlack);

    fd.set_fontid("haah");
    fd.set_xheight(capitalX.height());
    fd.set_margin(3);

    // std::cout << fd.fontid() << std::endl;

    std::stringstream ss;
    fd.SerializeToOstream(&ss);
    SDFFont<mms::Standalone> sdfFont(ss.str());

    for (auto index: face.glyphIndices()) {
        auto glyph = face.render(index, 32, fu::RenderingType::GrayscaleWhiteOnBlack);
        yandex::maps::proto::vector_data::glyphs::Glyph proto;
        proto.set_id(index);

        proto.set_bitmap(glyph.data().data(), glyph.data().size());

        proto.set_width(glyph.width());
        proto.set_height(glyph.height());
        proto.set_bearingx(glyph.horizontalBearingX());
        proto.set_bearingy(glyph.horizontalBearingY());
        proto.set_advance(glyph.horizontalAdvance());

        std::stringstream gss;
        proto.SerializeToOstream(&gss);
        sdfFont.glyphsProto[index] = gss.str();
    }

    std::ofstream out("font.mms");
    mms::Writer w(out);
    size_t ofs = mms::safeWrite(w, sdfFont);
    std::cout << "file size = " << ofs << std::endl;
    out.close();
    return 0;

}
