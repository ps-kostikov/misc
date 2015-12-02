BIN := sample 

sample: SRCS = \
    sample.cpp

sample: LIBS = \
    yandex-maps-coverage5 \
    yandex-maps-tileutils5 \
    yandex-maps-geolib3 \

sample: INCLUDES += \
    /usr/include/libxml2 \


CXX = g++-4.8
CXXFLAGS += -std=c++0x

include /usr/share/yandex/maps/build/build.mk
