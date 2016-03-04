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
    cairo_surface_t* surface = cairo_image_surface_create_from_png("test.png");

    if (surface == NULL) {
        return 1;
    }

    unsigned char* data = cairo_image_surface_get_data(surface);
    int width = cairo_image_surface_get_width (surface);
    int height = cairo_image_surface_get_height (surface);
    std::cout << width << ":" << height << std::endl;
    for (int i = 0; i < width * height; ++i) {
        unsigned char* offset = data + 4 * i;
        if (
            (int(*(offset + 0)) == 255) and 
            (int(*(offset + 1)) == 255) and
            (int(*(offset + 2)) == 255)
        ) {
            continue;
        }
        if (int(*(offset + 3)) == 255) {
            *(offset + 0) = '\0';
            *(offset + 1) = '\0';           
        }

        // if (
        //     (int(*(offset + 0)) == 255) and 
        //     (int(*(offset + 1)) == 255) and
        //     (int(*(offset + 2)) == 255)
        // ) {
        //     std::cout << "hello" << std::endl;
        //     continue;
        // }
        // *(offset + 0) = '\0';
        // *(offset + 1) = '\0';
        // *(offset + 2) = *(offset + 3);
    }
    // cairo_t* cr = cairo_create(surface);
    // if (cr == NULL) {
    //     cairo_surface_destroy(surface);
    //     return 2;
    // }

    cairo_surface_write_to_png(surface, "out2.png");
    // cairo_surface_write_to_png_stream(surface, &writeToCoutStream, NULL);

    // cairo_destroy(cr);
    cairo_surface_destroy(surface);


    return 0;
}

