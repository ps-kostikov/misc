BIN := sample 

sample: SRCS = \
    sample.cpp

sample: LIBS = \
	yandex-maps-fontutils \
	protobuf \

PROTO_DEPENDS = \
	vector-data=0.0 \

CXX = g++-4.8

CXXFLAGS += -std=c++0x

include /usr/share/yandex/maps/build/build.mk
