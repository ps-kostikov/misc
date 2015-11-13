stop-nginx:
	/sbin/start-stop-daemon --stop --quiet --pidfile nginx.pid || true

start-nginx: stop-nginx
	nohup /usr/sbin/nginx -p $$(pwd)/ -c nginx.conf &

start-pure-nginx: stop-nginx
	nohup /usr/sbin/nginx -p $$(pwd)/ -c nginx.pure.conf &

clean:
	rm -f *.log *.out
