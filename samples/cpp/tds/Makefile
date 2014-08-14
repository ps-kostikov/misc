BIN := sample 

sample: SRCS = \
	sample.cpp \
	config.cpp


sample: INCLUDES += \
    /usr/include/pqxx \
    /usr/include/libxml2 \


sample: LIBS := \
    yandex-maps-wiki-revision \
    yandex-maps-xml3 \
    yandex-maps-pgpool2 \


$(bin-name): LIBS := \
    yandex-maps-common \
    yandex-maps-log \
    yandex-maps-wiki-common \
    yandex-maps-revision-proto \

CXX = g++-4.8
CXXFLAGS += -std=c++0x

include /usr/share/yandex/maps/build/build.mk
