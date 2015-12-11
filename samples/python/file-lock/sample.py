#!/usr/bin/python

import fcntl
import time

# print('before open')
# f = open('1', 'w')
# print('after open')

# print('before lock')
# fcntl.flock(f, fcntl.LOCK_EX)
# print('after lock')

# time.sleep(20)

# print('before unlock')
# fcntl.flock(f, fcntl.LOCK_UN)
# print('after unlock')

# print('before close')
# f.close()
# print('after close')


class FileLock:
    def __init__(self, file_name):
        self._file_name = file_name

    def __enter__(self):
        self._fd = open(self._file_name, 'w')
        fcntl.flock(self._fd, fcntl.LOCK_EX)

    def __exit__(self, type, value, traceback):
        fcntl.flock(self._fd, fcntl.LOCK_UN)
        self._fd.close()


with FileLock('1'):
    print('before job')
    time.sleep(10)
    print('after job')
