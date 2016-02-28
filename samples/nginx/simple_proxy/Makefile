CUR_DIR = $(shell pwd)

stop:
	/sbin/start-stop-daemon --stop --quiet --pidfile nginx.pid || true

start: stop
	nohup /usr/sbin/nginx -p $(CUR_DIR)/ -c nginx.conf &