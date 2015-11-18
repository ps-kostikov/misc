LIB := sample 

sample: SRCS = \
	sample.cpp

PYTHON   = $(shell readlink /usr/bin/python)

sample: INCLUDES += \
	/usr/include/$(PYTHON) \
	/usr/include/ImageMagick


sample: LIBS = \
	boost_python \
	Magick++ \

CXX = g++-4.6
CXXFLAGS += -std=c++0x

include /usr/share/yandex/maps/build/build.mk