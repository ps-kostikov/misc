#!/usr/bin/python

import os
import sys
import stat
import collections
import datetime
import time
import logging
import functools

import opster
import requests

@opster.command(name='command1')
def remove_tileindex_hook(
        param1=('', '', ''),
        param2=('', '', '')):
    pass


@opster.command(name='command2')
def remove_tileindex_hook(
        param1=('', '', ''),
        param2=('', '', '')):
    pass

if __name__ == '__main__':
    format_ = '%(asctime)s [%(process)s] %(levelname)s: %(message)s'
    logging.basicConfig(stream=sys.stdout, level=logging.DEBUG, format=format_)
    logging.info("Start command {0}".format(sys.argv))
    opster.dispatch()
    logging.info("Finish command {0}".format(sys.argv))

