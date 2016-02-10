USE += yacare

BIN := gen
bin-name = yacare
package-name := yandex-maps-sample-pkostikov

SRCPKG = $(package-name)
PKG = $(package-name)

$(PKG): YCR_APPS += $(bin-name)

$(bin-name): SRCS := \
    yacare.cpp \

gen: SRCS := \
	gen.cpp \

CXX = g++-4.8
CXXFLAGS += -std=c++0x

include /usr/share/yandex/maps/build/build.mk
