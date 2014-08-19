#!/usr/bin/python

import os
import re
import subprocess
import urllib2

import opster
from ytools import xml

from yandex.maps.factory.config import config


def call(cmdline, env=None):
    p = subprocess.Popen(
        cmdline,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        env=env)
    out = p.communicate()
    if p.returncode:
        raise RuntimeError(
            "Failed call to [{cmdline}] (returned code: {return_code})\n"
            "Captured stdout:\n"
            "{stdout}\n\n"
            "Captured stderr:\n"
            "{stderr}\n\n".format(
                cmdline=" ".join(cmdline),
                return_code=p.returncode,
                stdout=out[0],
                stderr=out[1]
            )
        )
    return out[0]


def error(str_):
    print 'ERROR', str_


def hint(str_):
    print 'HINT', str_


def check(condition, error_str, hint_str=None):
    if not condition:
        error(error_str)
        if hint_str is not None:
            hint(hint_str)
    return condition


@opster.command(name='database')
def check_database():
    for alias, dbconfig in config.databases.iteritems():
        user, password, host, port, dbname = re.match(
            'postgresql://(\w+):(\w+)@([\w\.-]+):(\d+)/(\w+)', dbconfig.url).groups()
        call(["nc", "-z", host, port])


@opster.command(name='mongo')
def check_mongo():
    host, dbname = re.match('mongodb://([\w\.-]+)/(\w+)', config.celery.broker_url).groups()
    port = "27017"
    call(["nc", "-z", host, port])


if __name__ == '__main__':
    opster.dispatch()
