# Version 0.0.1
FROM ubuntu:14.04
MAINTAINER Pavel Kostikov <pkostikov@yandex-team.ru>
COPY yandex.list /etc/apt/sources.list.d/yandex.list
COPY verstka-unstable.list /etc/apt/sources.list.d/verstka-unstable.list

RUN apt-get update && \
    mkdir -p /etc/syslog-ng/conf-enabled/ && \
    apt-get install --force-yes -y -o Dpkg::Options::='--force-confnew' syslog-ng syslog-ng-core syslog-ng-mod-sql syslog-ng-mod-mongodb syslog-ng-mod-json supervisor yandex-maps-common-nginx yandex-maps-jsapi-box=1.0.0-8

COPY yandex-maps-jsapi-box.conf /etc/yandex-maps-jsapi-box.conf
COPY supervisord.conf /etc/supervisor/conf.d/supervisord.conf
EXPOSE 80
CMD /usr/bin/supervisord
