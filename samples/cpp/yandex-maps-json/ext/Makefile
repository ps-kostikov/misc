BIN := sample1 sample2 

sample1: SRCS = \
    sample1.cpp

sample1: LIBS = \
    yandex-maps-json \
    boost153_date_time \


sample2: SRCS = \
    sample2.cpp

sample2: LIBS = \
    yandex-maps-json \

CXX = g++-4.8
CXXFLAGS += -std=c++0x

include /usr/share/yandex/maps/build/build.mk
