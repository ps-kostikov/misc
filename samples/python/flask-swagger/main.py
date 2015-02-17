#!/usr/bin/python

import flask
import opster

app = flask.Flask(__name__)


def send_json_data(data):
    print flask.request.headers
    resp = flask.make_response(data)
    resp.mimetype = "application/json"
    resp.headers['Access-Control-Allow-Origin'] = 'http://hadar.backa.dev.yandex.net:18005'
    resp.headers['Access-Control-Allow-Methods'] = 'GET, POST, DELETE, PUT, PATCH, OPTIONS'
    resp.headers['Access-Control-Allow-Headers'] = 'Content-Type, api_key, Authorization'
    return resp


def send_json_from_file(path_to_file):
    with open(path_to_file) as f:
        content = f.read()
    return send_json_data(content)


@app.route("/swagger1.2.header.json")
def get_header_api_1_2():
    return send_json_from_file('swagger1.2.header.json')


@app.route("/swagger1.2.header.json/")
def get_header_api_1_2_():
    return send_json_from_file('swagger1.2.header.json')


@app.route("/swagger1.2.json")
def get_api_1_2():
    return send_json_from_file('swagger1.2.json')


@app.route("/swagger2.0.json")
def get_api_2_0():
    return send_json_from_file('swagger2.0.json')


@app.route("/handle")
def get_handle():
    return send_json_data('{"hello": "world"}')


@opster.command()
def main():
    app.run(host="0.0.0.0", port=9016, use_debugger=True)


if __name__ == '__main__':
    main()
