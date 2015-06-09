#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <vector>
#include <chrono>

#include <yandex/maps/fontutils/glyph.h>
#include <yandex/maps/fontutils/font_face.h>
#include <yandex/maps/fontutils/exception.h>

namespace fu = maps::fontutils;

struct Params
{
    size_t pixelSize;
    fu::RenderType renderType;
    size_t paddingSize;
    double scale;
};

fu::Glyph renderGlyph(
    const fu::FontFace& face,
    const Params& params,
    size_t index)
{
    return face.render(
        index,
        params.pixelSize,
        params.renderType
    )
        .addPaddings(params.paddingSize)
        .convertToSignedDistanceField()
        .scale(params.scale);
}

int main()
{
    std::cout << "hello" << std::endl;

    std::string path = "/usr/share/yandex/maps/renderer5/fonts/LiberationSans-Regular.ttf";

    fu::FontFace face(path);
    std::cout << "number of glyphs: " << face.glyphsCount() << std::endl;

    auto begin = std::chrono::high_resolution_clock::now();
    // Params params{50, fu::RenderType::MonochromeWhiteOnBlack, 15, 0.5};
    Params params{50, fu::RenderType::MonochromeWhiteOnBlack, 6, 0.5};
    for (size_t index = 0; index < face.glyphsCount(); ++index) {
        try {
            auto glyph = renderGlyph(face, params, index);
            glyph.width();
        } catch (maps::Exception& ex) {
            std::cout << "maps::Exception: " << ex << std::endl;
            std::cout << "error rendering glyph with index " << index << std::endl;
        } catch (std::exception& ex) {
            std::cout << "std::exception: " << ex.what() << std::endl;
            std::cout << "error rendering glyph with index " << index << std::endl;
        } catch (...) {
            std::cout << "unknown exception" << std::endl;
            std::cout << "error rendering glyph with index " << index << std::endl;
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto total = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "Total time is " << total << " ms " << std::endl;

    return 0;

}
