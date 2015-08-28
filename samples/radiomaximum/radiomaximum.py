import requests
import tempfile
import shutil
import subprocess
import os
import contextlib
import time
import sys

timedelta = int(sys.argv[1])
oauth = '88fed87d955f4c2ba82435269dbf51e1'


@contextlib.contextmanager
def tmp_dir():
    try:
        d = tempfile.mkdtemp()
        yield d
    finally:
        shutil.rmtree(d)


def upload(filename, diskname):
    host = 'cloud-api.yandex.net'
    headers = {'Authorization': 'OAuth {0}'.format(oauth)}
    href_resp = requests.get(
        'https://{host}:443/v1/disk/resources/upload?path=app:/{diskname}&overwrite=true'.format(**locals()),
        headers=headers)

    assert(href_resp.ok)
    assert(href_resp.json()['templated'] == False)
    assert(href_resp.json()['method'] == 'PUT')
    href = href_resp.json()['href']

    with open(filename, 'rb') as f:
        upload_resp = requests.put(href, data=f)

    assert(upload_resp.ok)


def download(filename):
    tmp_fname = 'f.mp3'
    with tmp_dir() as d:
        retcode = subprocess.call([
            'streamripper',
            'http://maximum.fmtuner.ru/',
            '-l', str(timedelta),
            '-a', tmp_fname,
            '-quiet'
            '-s',
            '-d', d
        ], stdout=None, stderr=None)
        assert(retcode == 0)
        os.rename(os.path.join(d, tmp_fname), filename)


def make_disk_filename():
    return time.strftime(
        '%Y-%m-%d %H-%M.{0} min.mp3'.format(timedelta / 60),
        time.gmtime(time.time() + 60 * 60 * 3))

tmp_filename = 'tmp.mp3'
download(tmp_filename)
upload(tmp_filename, make_disk_filename())
os.remove(tmp_filename)

