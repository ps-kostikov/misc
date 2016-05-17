# Version 0.0.1
FROM ubuntu:14.04
MAINTAINER Egor Kartashov <kartvep@yandex-team.ru>
COPY yandex.list /etc/apt/sources.list.d/yandex.list
COPY packages/*.deb /tmp/
RUN apt-get update && \
    mkdir -p /etc/syslog-ng/conf-enabled/ && \
    apt-get install --force-yes -y -o Dpkg::Options::='--force-confnew' libyandex-maps-log8 libyandex-maps-common libfastcgi-daemon2 libyandex-maps-http libyandex-maps-i18n libyandex-maps-renderer5 libyandex-maps-renderer6 libyandex-maps-tilerenderer5 libyandex-maps-hotspots-base5 libyandex-maps-json-config libyandex-maps-rapidjson-dev libyandex-maps-localeutils libboost-filesystem1.54.0:amd64 libboost-regex1.54.0:amd64 zlib1g:amd64 fastcgi-daemon2 fastcgi-daemon2-init yandex-maps-config-sysctl yandex-maps-config-ulimits-www-data libfastcgi2-syslog nginx nginx-full yandex-maps-common-nginx supervisor libfastcgi-daemon2-dev libyandex-maps-http-dev libyandex-maps-common-dev yandex-environment-testing syslog-ng-include syslog-ng-include-tpl3.5 syslog-ng syslog-ng-core yandex-maps-common-nginx-listen-80 && \
    dpkg -i /tmp/*.deb && \
    sed -i '/kmsg/d' /etc/syslog-ng/syslog-ng.conf && \
    sed -i '3a daemon off\;' /etc/nginx/nginx.conf && \
    sed -i 's/8080/8180/' /etc/nginx/listen && \
    mkdir -p /var/lib/yandex/maps/staticrenderer/compiled_maps
COPY maps/map_russia_90876a6d28ab8875d216aa7f5b9d82ffbafed69b575baf11df4b9648b80273bd* /var/lib/yandex/maps/staticrenderer/compiled_maps/
COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf
#COPY nginx.conf /etc/nginx/nginx.conf
EXPOSE 80
CMD /usr/bin/supervisord
