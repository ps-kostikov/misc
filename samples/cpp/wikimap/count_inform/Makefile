BIN := sample 

sample: SRCS = \
    sample.cpp \
	releases_notification.cpp \
	blackbox.cpp \
	sender.cpp \
	common.cpp \
	user_filters.cpp \

sample: LIBS = \
	blackbox2 \
	geos \
	yandex-maps-cmdline \
	yandex-maps-common \
	yandex-maps-grinder-worker \
	yandex-maps-http2 \
	yandex-maps-log8 \
	yandex-maps-json \
	yandex-maps-pgpool3 \
	yandex-maps-ssl \
	yandex-maps-wiki-acl \
	yandex-maps-wiki-common \
	yandex-maps-wiki-revision \
	yandex-maps-wiki-social \
	yandex-maps-xml3 \
	xml2 \

sample: INCLUDES += \
    /usr/include/pqxx \
	/usr/include/libxml2 \

CXX = g++-4.8
CXXFLAGS += -std=c++0x

include /usr/share/yandex/maps/build/build.mk
