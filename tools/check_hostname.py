#!/usr/bin/python

import re
import sys

regexp, host = sys.argv[1:3]
if (re.match(regexp, host)):
    print('Match')
else:
    print('Not match')
# print regexp, host