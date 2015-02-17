#!/usr/bin/python

import flask
import opster

app = flask.Flask(__name__)


def send_json(path_to_file):
    with open(path_to_file) as f:
        content = f.read()
    resp = flask.make_response(content)
    resp.mimetype = "application/json"
    resp.headers['Access-Control-Allow-Origin'] = '*'
    resp.headers['Access-Control-Allow-Methods'] = 'GET, POST, DELETE, PUT, PATCH, OPTIONS'
    resp.headers['Access-Control-Allow-Headers'] = 'Content-Type, api_key, Authorization'
    return resp


@app.route("/swagger1.2.header.json")
def get_header_api_1_2():
    return send_json('swagger1.2.header.json')


@app.route("/swagger1.2.header.json/")
def get_header_api_1_2_():
    return send_json('swagger1.2.header.json')


@app.route("/swagger1.2.json")
def get_api_1_2():
    return send_json('swagger1.2.json')


@app.route("/swagger2.0.json")
def get_api_2_0():
    return send_json('swagger2.0.json')


@app.route("/handle")
def get_handle():
    return '{"hello": "world"}'


@opster.command()
def main():
    app.run(host="0.0.0.0", port=9016, use_debugger=True)


if __name__ == '__main__':
    main()
