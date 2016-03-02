USE += yacare

bin-name = yacare
package-name := yandex-maps-sample-pkostikov

SRCPKG = $(package-name)
PKG = $(package-name)

$(PKG): YCR_APPS += $(bin-name)

$(bin-name): LIBS = \
    yandex-maps-pgpool3 \
    yandex-maps-http2 \

$(bin-name): SRCS := \
    yacare.cpp \

$(bin-name): INCLUDES += \
    /usr/include/pqxx \

CXX = g++-4.8
CXXFLAGS += -std=c++0x

include /usr/share/yandex/maps/build/build.mk
