#include <iostream>
#include <sstream>
#include <fstream>
#include <thread>
#include <vector>
#include <chrono>

#include <yandex/maps/fontutils/glyph.h>

namespace fu = maps::fontutils;

fu::Glyph renderGlyph(
    const fu::FontFace& face,
    size_t index)
{
    return face.render(
        index,
        50,
        fu::RenderType::MonochromeWhiteOnBlack
    )
        .addPaddings(15)
        .convertToSignedDistanceField()
        .scale(0.5);
}

void f(const std::string& path, int threadIndex, int threadCount)
{
    fu::FontFace face(path);

    for (auto index: face.glyphIndices()) {
        if (index % threadCount != (size_t)threadIndex) {
            continue;
        }
        try {
            auto glyph = renderGlyph(face, index);
            glyph.width();
        } catch (...) {
            std::cout << "error rendering glyph with index " << index << std::endl;
        }
    }

}

int main()
{
    std::cout << "hello" << std::endl;

    // std::string path = "/usr/share/yandex/maps/renderer5/fonts/LiberationSans-Regular.ttf";
    std::string path = "/usr/share/yandex/maps/renderer5/fonts/arial.ttf";

    fu::FontFace face(path);
    std::cout << "number of glyphs: " << face.glyphCount() << std::endl;
    std::cout << "height of 'x': " 
        << renderGlyph(face, face.charGlyphIndex(U'x')).height() << std::endl;

    auto begin = std::chrono::high_resolution_clock::now();

    std::vector<std::thread> threads;
    int threadCount = 8;
    for (int i = 0; i < threadCount; ++i) {
        threads.push_back(std::thread([i, threadCount, &path](){
            f(path, i, threadCount);
        }));
    }
    for (auto& th: threads) {
        th.join();
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto total = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count();
    std::cout << "Total time is " << total << " ms " << std::endl;

    std::cout << "done" << std::endl;
    return 0;

}
