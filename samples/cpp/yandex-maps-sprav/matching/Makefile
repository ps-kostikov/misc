BIN := sample 

sample: SRCS = \
    sample.cpp


sample: INCLUDES += . \
    /usr/include/pqxx \
    /usr/include/libxml2 \
    /home/pkostikov/mapscore/sprav/libs/model/include \
    
    # yandex-maps-sprav-model \

sample: LIBS = \
	:/home/pkostikov/mapscore/sprav/libs/model/libyandex-maps-sprav-model.so \
    yandex-maps-sprav-algorithms-datamodel \
    yandex-maps-sprav-algorithms-similarity \
    yandex-maps-sprav-common \
    yandex-maps-geolib3 \
    yandex-maps-localeutils \
    yandex-maps-json \
    yandex-maps-pgpool2 \
    yandex-maps-log \


CXX = g++-4.8
CXXFLAGS += -std=c++11

include /usr/share/yandex/maps/build/build.mk
