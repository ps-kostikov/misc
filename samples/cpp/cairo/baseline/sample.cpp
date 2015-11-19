#include <list>
#include <string>
#include <iostream>
#include <istream>
#include <iterator>

#include <cairo.h>

#ifndef CAIRO_HAS_PNG_FUNCTIONS
#error png functions support must be enabled
#endif

cairo_status_t writeToCoutStream(void* /*streamPtr*/, const unsigned char* data, unsigned int length)
{
    return std::cout.write(reinterpret_cast<const char*>(data), length) ? CAIRO_STATUS_SUCCESS : CAIRO_STATUS_WRITE_ERROR;
}

int main(int /*argc*/,char** /*argv*/) 
{
    std::istream_iterator<char> it(std::cin);
    std::istream_iterator<char> end;
    std::string results(it, end);

    size_t width = 256;
    size_t height = 256;

    cairo_surface_t* img = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);

    if (img == NULL) {
        return 1;
    }

    cairo_t* cr = cairo_create(img);
    if (cr == NULL) {
        cairo_surface_destroy(img);
        return 2;
    }

    cairo_set_antialias(cr, CAIRO_ANTIALIAS_DEFAULT);
    // cairo_set_antialias(cr, CAIRO_ANTIALIAS_NONE);

    // cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
    // cairo_set_line_join(cr, CAIRO_LINE_JOIN_ROUND);


    cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, 1.0);
    cairo_rectangle(cr, 0, 0, width, height);
    cairo_fill(cr);

    cairo_set_source_rgba(cr, 1.0, 0., 0., 1.);
    cairo_set_line_width(cr, 1);
    cairo_move_to(cr, 1, 1);
    cairo_line_to(cr, 7, 6);    

    cairo_stroke (cr);

    // cairo_surface_write_to_png(img, "out.png");
    // cairo_surface_write_to_png_stream(img, &writeToCoutStream, NULL);

    cairo_destroy(cr);
    cairo_surface_destroy(img);


    return 0;
}

