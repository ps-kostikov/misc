#!/usr/bin/python

import os
import logging
import functools

import opster


def log_exceptions(f):
    @functools.wraps(f)
    def wrapper(*args, **kwargs):
        try:
            return f(*args, **kwargs)
        except Exception as ex:
            logging.exception(ex)

    return wrapper


def setup_logging():
    filename = os.getenv('LOGFILE', '/var/log/yandex/maps/staticrenderer/staticrenderer-tool.log')
    logging.basicConfig(filename=filename, level=logging.DEBUG)


@opster.command(name='hook')
@log_exceptions
def hook():

    logging.debug("debug msg")
    logging.info("info msg")
    logging.warning("warning msg")
    logging.error("error msg")

    raise RuntimeError("error")


if __name__ == '__main__':
    setup_logging()
    opster.dispatch()