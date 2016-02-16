import logging
import StringIO
import requests
import time

from ytools import xml
from yandex.maps.utils import common as common_utils

from yandex.maps.factory import utils


CHUNK_SIZE_BYTES = 1024 * 1024
TIMEOUT_SEC = 5
# logger = logging.getLogger("mds")


class Error(Exception):
    def __init__(self, message, url, http_code):
        self.message = message
        self.url = url
        self.http_code = http_code

    def __str__(self):
        return '[Error message={0} url={1!r} http_code={2}]'.format(
            self.message,
            self.url,
            self.http_code)


class ServerError(Error):
    pass


class NotFound(Error):
    def __init__(self, key, url):
        self.key = key
        super(NotFound, self).__init__('Not found', url, requests.codes.NOT_FOUND)

    def __str__(self):
        return '[NotFound key={0} url={1!r}]'.format(self.key, self.url)


class AlreadyExists(Error):
    def __init__(self, name, available_key, url):
        self.name = name
        self.available_key = available_key
        super(AlreadyExists, self).__init__('Forbidden', url, requests.codes.FORBIDDEN)

    def __str__(self):
        return '[AlreadyExists name={0} available_key={1} url={2!r}]'.format(
            self.name,
            self.available_key,
            self.url)


class Auth(requests.auth.AuthBase):

    def __init__(self, auth_header):
        self.auth_header = auth_header

    def __call__(self, request):
        request.headers['Authorization'] = self.auth_header
        return request

    def __repr__(self):
        return "-H 'Authorization: {0}'".format(self.auth_header)


class MdsEngine(object):
    def __init__(self, schema, host, read_port, write_port, auth_header, namespace, subdir):
        self.schema = schema
        self.host = host
        self.read_port = read_port
        self.write_port = write_port
        self.auth = Auth(auth_header)
        self.namespace = namespace
        self.subdir = subdir
        self.session = requests.Session()
        # self.session = requests

    def make_write_url(self, name):
        if not self.subdir:
            template = '{schema}://{host}:{port}/upload-{namespace}/{name}'
        else:
            template = '{schema}://{host}:{port}/upload-{namespace}/{subdir}/{name}'

        return template.format(
                schema=self.schema,
                host=self.host,
                port=self.write_port,
                namespace=self.namespace,
                subdir=self.subdir,
                name=name)

    def make_delete_url(self, key):
        return '{schema}://{host}:{port}/delete-{namespace}/{key}'.format(
            schema=self.schema,
            host=self.host,
            port=self.write_port,
            namespace=self.namespace,
            key=key)

    def make_read_url(self, key):
        return '{schema}://{host}:{port}/get-{namespace}/{key}'.format(
            schema=self.schema,
            host=self.host,
            port=self.read_port,
            namespace=self.namespace,
            key=key)


    def write(self, name, file_obj):
        url = self.make_write_url(name)
        # logging.debug(url)
        resp = self.session.post(url, auth=self.auth, data=file_obj, timeout=TIMEOUT_SEC)
        # resp = requests.post(url, auth=self.auth, data=file_obj, timeout=TIMEOUT_SEC)

        if resp.status_code == requests.codes.BAD_REQUEST:
            raise Error("Protocol miscommunication url: {0!r}".format(url), url, resp.status_code)
        elif resp.status_code == requests.codes.UNAUTHORIZED:
            raise Error("Wrong authorization header {0!r}".format(self.auth), url, resp.status_code)
        elif resp.status_code == requests.codes.FORBIDDEN:
            key = key_from_forbidden_resp(utils.to_string(resp.text))
            raise AlreadyExists(name=name, available_key=key, url=url)
        elif resp.status_code == requests.codes.INSUFFICIENT_STORAGE:
            raise Error("No space left", url, resp.status_code)
        elif resp.status_code >= requests.codes.SERVER_ERROR:
            raise ServerError("Server error", url, resp.status_code)
        elif resp.status_code != requests.codes.OK:
            raise Error("Unexpected error", url, resp.status_code)
        return key_from_ok_resp(utils.to_string(resp.text))

    def delete(self, key):
        url = self.make_delete_url(key)
        # resp = requests.get(url, auth=self.auth, timeout=TIMEOUT_SEC)
        resp = self.session.get(url, auth=self.auth, timeout=TIMEOUT_SEC)
        if resp.status_code == requests.codes.NOT_FOUND:
            raise NotFound(key, url)
        elif resp.status_code >= requests.codes.SERVER_ERROR:
            raise ServerError("Server error", url, resp.status_code)
        elif resp.status_code != requests.codes.OK:
            raise Error("Unexpected error", url, resp.status_code)

    def read(self, key, file_obj):
        url = self.make_read_url(key)
        resp = self.session.get(url, auth=self.auth, timeout=TIMEOUT_SEC)
        # resp = requests.get(url, auth=self.auth, timeout=TIMEOUT_SEC)
        if resp.status_code == requests.codes.NOT_FOUND:
            raise NotFound(key, url)
        elif resp.status_code >= requests.codes.SERVER_ERROR:
            raise ServerError("Server error", url, resp.status_code)
        elif resp.status_code != requests.codes.OK:
            raise Error("Unexpected error", url, resp.status_code)
        for chunk in resp.iter_content(CHUNK_SIZE_BYTES):
            file_obj.write(chunk)


class MdsStorage(object):
    def __init__(self, engine, retry_policy):
        self.engine = engine
        self.retry_policy = retry_policy

    @classmethod
    def from_config(cls, config, namespace_type):
        auth = config.namespaces._asdict()[namespace_type]
        return cls(
            MdsEngine(
                schema=config.schema,
                host=config.host,
                read_port=config.read_port,
                write_port=config.write_port,
                auth_header=auth.auth_header,
                namespace=auth.namespace,
                subdir=auth.subdir),
            config.retry_policy)

    # def do_retry(self, func):
    #     return common_utils.retry(
    #         exceptions=(requests.RequestException, ServerError),
    #         tries=self.retry_policy.retry_number,
    #         delay=self.retry_policy.wait_seconds,
    #         backoff_multiplier=self.retry_policy.wait_factor,
    #         logger=logging
    #     )(func)()

    def do_retry(self, func):
        retry_number = self.retry_policy.retry_number
        delay = self.retry_policy.wait_seconds
        for i in range(retry_number):
            try:
                return func()
            except (requests.RequestException, ServerError), ex: 
                if i == retry_number - 1:
                    raise
                else:
                    logging.exception("Attempt number {0} failed. Retrying in {1} seconds".format(i, delay))
            time.sleep(delay)
            delay *= self.retry_policy.wait_factor

    def write_data(self, name, data):
        file_obj = StringIO.StringIO(data)
        return self.write(name, file_obj)

    def write_from_file(self, name, filename):
        with open(filename) as f:
            return self.write(name, f)

    def write(self, name, file_obj):
        return self.do_retry(lambda: self.engine.write(name, file_obj))


    def read_data(self, key):
        file_obj = StringIO.StringIO()
        self.read(key, file_obj)
        return file_obj.getvalue()

    def read_to_file(self, key, filename):
        with open(filename, 'w') as f:
            self.read(key, f)

    def read(self, key, file_obj):
        self.do_retry(lambda: self.engine.read(key, file_obj))


    def delete(self, key):
        self.do_retry(lambda: self.engine.delete(key))



def key_from_ok_resp(xml_str):
    xml_obj = xml.ET.fromstring(xml_str)
    return str(xml_obj.xpath('/post/@key')[0])


def key_from_forbidden_resp(xml_str):
    xml_obj = xml.ET.fromstring(xml_str)
    return xml_obj.xpath('/post/key')[0].text
