# for more details glance through wiki
# https://wiki.yandex-team.ru/jandekskarty/projects/catalog/sprav/new/storage/dev/libs/matching_experiments

BIN := \
    make_ammo \

PYTHON := $(shell pyversions -d)

ALL_INCLUDES = \
    /usr/include/pqxx \
    /usr/include/libxml2 \
    /usr/include/$(PYTHON) \
    ../include \

ALL_LIBS = \
    $(PYTHON) \
    boost153_program_options \
    boost153_python \
    yandex-maps-sprav-model \
    yandex-maps-sprav-common \
    yandex-maps-sprav-algorithms-datamodel \
    yandex-maps-sprav-algorithms-similarity \
    yandex-maps-mining-sprav2miningson \
    yandex-maps-sprav-common \
    yandex-maps-geolib3 \
    yandex-maps-localeutils \
    yandex-maps-pgpool3 \
    yandex-maps-log \


make_ammo: SRCS = \
    make_ammo.cpp \
    ammo.cpp \

make_ammo: INCLUDES += \
    $(ALL_INCLUDES) \

make_ammo: LIBS = \
    $(ALL_LIBS) \


CXX = g++-4.8
CXXFLAGS += -std=c++11

include /usr/share/yandex/maps/build/build.mk
