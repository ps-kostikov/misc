import urllib2
import logging

try:
    urllib2.urlopen("http://www.ya.ru/sdfsdfsf")
except urllib2.HTTPError, ex:
    logging.warning("While accessing {url} got HTTPError {code}: {msg}".format(
        url=ex.geturl(),
        code=ex.getcode(),
        msg=ex.msg
    ))
