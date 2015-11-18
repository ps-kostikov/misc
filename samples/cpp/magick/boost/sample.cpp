#include <boost/noncopyable.hpp>
#include <boost/python.hpp>

#include <Magick++.h>

class Image
{
public:
    Image() {
        Magick::Image image("256x256", "black");
    }
};

BOOST_PYTHON_MODULE(sample) {

    Magick::InitializeMagick("");

    using namespace boost::python;


    class_<Image, boost::noncopyable>("Image", init<>());

}
