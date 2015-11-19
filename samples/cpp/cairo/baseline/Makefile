BIN := sample 

sample: SRCS = \
    sample.cpp

sample: INCLUDES += \
	/usr/include/cairo \

sample: LIBS = \
	cairo \

USE += g++-4.8
CXX = $(g++-4.8)
CXXFLAGS += -std=c++11

include /usr/share/yandex/maps/build/build.mk