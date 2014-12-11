BIN := sample s2


PYTHON := $(shell pyversions -d)

sample: SRCS = \
    sample.cpp

sample: INCLUDES += \
    /usr/include/$(PYTHON) \
    /usr/include/pqxx \
    /usr/include/libxml2 \

sample: LIBS = \
    boost153_program_options \
    boost153_python \
    $(PYTHON) \
    yandex-maps-json \
    yandex-maps-log \
    yandex-maps-geolib3 \
    yandex-maps-mining-sprav2miningson \
    yandex-maps-sprav-algorithms-datamodel \
    yandex-maps-sprav-matching \


s2: SRCS = \
    s2.cpp

s2: INCLUDES += \
    /usr/include/$(PYTHON) \
    /usr/include/pqxx \
    /usr/include/libxml2 \

s2: LIBS = \
    boost153_program_options \
    boost153_python \
    $(PYTHON) \
    yandex-maps-json \
    yandex-maps-log \
    yandex-maps-geolib3 \
    yandex-maps-mining-sprav2miningson \
    yandex-maps-sprav-algorithms-datamodel \
    yandex-maps-sprav-matching \


CXX = g++-4.8
CXXFLAGS += -std=c++11

include /usr/share/yandex/maps/build/build.mk
