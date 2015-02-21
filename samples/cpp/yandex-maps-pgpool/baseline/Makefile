BIN := sample 

sample: SRCS = \
    sample.cpp

sample: LIBS = \
    yandex-maps-pgpool3

sample: INCLUDES += \
    /usr/include/pqxx \

CXX = g++-4.8
CXXFLAGS += -std=c++0x

include /usr/share/yandex/maps/build/build.mk
