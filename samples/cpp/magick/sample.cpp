#include <list>
#include <string>
#include <iostream>

#include <Magick++.h>

int main(int /*argc*/,char **argv) 
{ 
    Magick::InitializeMagick(*argv);

    try { 
        Magick::Image image("256x256", "black");
    } 
    catch( Magick::Exception &error_ ) 
    { 
          std::cerr << "Caught exception: " << error_.what() << std::endl; 
          return 1; 
    } 
    return 0; 
}

// int main(int argc,char **argv)
// {
//     // const size_t size = 256 * 256 * 4;
//     // char buf[size];
//     // Magick::Blob blob(buf, size);
//     // Magick::Geometry geometry(256, 256);
//     // Magick::Color color;
//     // Magick::Image(blob, geometry);
    
//     // std::cout << blob.length() << std::endl;
//     // Magick::Color color(std::string("#FFAABB00"));

//     Magick::InitializeMagick(*argv);
//     Magick::Color color(10, 20, 30, 0);
//     Magick::Image image(Magick::Geometry(256, 256), color);
//     // image.magick("RGBA");

//     // std::list<Magick::Drawable> drawList;
//     // drawList.push_back(Magick::DrawableStrokeLineJoin(Magick::LineJoin::RoundJoin));
//     // drawList.push_back(Magick::DrawableStrokeColor("#FFFFFFFF"));
//     // drawList.push_back(Magick::DrawableStrokeWidth(3));
//     // drawList.push_back(Magick::DrawableFillColor("#FFFFFFFF"));
//     // std::list<Magick::Coordinate> arrow;
//     // arrow.push_back({20, 20});
//     // arrow.push_back({30, 30});
//     // arrow.push_back({25, 30});
//     // arrow.push_back({30, 25});
//     // arrow.push_back({30, 30});
//     // drawList.push_back(Magick::DrawablePolyline(arrow));

//     // image.draw(drawList);

//     // Magick::Blob blob;
//     // image.write(&blob, "PNG");

//     return 0;
// }