USE += yacare

bin-name = yacare
package-name := yandex-maps-sample-pkostikov

SRCPKG = $(package-name)
PKG = $(package-name)

$(PKG): YCR_APPS += $(bin-name)

$(bin-name): LIBS = \
	yandex-maps-pgpool3 \
	yandex-maps-json \

$(bin-name): SRCS := \
	yacare.cpp \

$(bin-name): INCLUDES += \
	/usr/include/pqxx \

CXX = g++-4.8
CXXFLAGS += -std=c++0x


CUR_DIR = $(shell pwd)

stop:
	/sbin/start-stop-daemon --stop --quiet --pidfile nginx.pid || true

start: stop
	nohup /usr/sbin/nginx -p $(CUR_DIR)/ -c nginx.conf &


include /usr/share/yandex/maps/build/build.mk
