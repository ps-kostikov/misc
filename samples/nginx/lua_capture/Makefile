stop-nginx:
	/sbin/start-stop-daemon --stop --quiet --pidfile nginx.pid || true

start-nginx: stop-nginx
	nohup /usr/sbin/nginx -p $$(pwd)/ -c nginx.conf &

clean:
	rm -f *.log *.out