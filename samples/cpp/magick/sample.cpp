#include <list>

#include <Magick++.h>

int main()
{
    Magick::Image image(Magick::Geometry(256, 256), "#FFFFFF00");
    // image.magick("RGBA");

    // std::list<Magick::Drawable> drawList;
    // drawList.push_back(Magick::DrawableStrokeLineJoin(Magick::LineJoin::RoundJoin));
    // drawList.push_back(Magick::DrawableStrokeColor("#FFFFFFFF"));
    // drawList.push_back(Magick::DrawableStrokeWidth(3));
    // drawList.push_back(Magick::DrawableFillColor("#FFFFFFFF"));
    // std::list<Magick::Coordinate> arrow;
    // arrow.push_back({20, 20});
    // arrow.push_back({30, 30});
    // arrow.push_back({25, 30});
    // arrow.push_back({30, 25});
    // arrow.push_back({30, 30});
    // drawList.push_back(Magick::DrawablePolyline(arrow));

    // image.draw(drawList);

    // Magick::Blob blob;
    // image.write(&blob, "PNG");

    return 0;
}