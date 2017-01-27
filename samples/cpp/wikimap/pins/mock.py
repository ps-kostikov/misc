#!/usr/bin/python
# -*- coding: utf-8 -*-
import os
import socket
import opster
import requests
import json

import flask

from yandex.maps.factory.tile import Tile
from yandex.maps.factory import geomhelpers

from ytools.pidfile import PIDFile


app = flask.Flask(__name__)



@app.route("/tiles")
def get_tile():
    x = int(flask.request.args['x'])
    y = int(flask.request.args['y'])
    z = int(flask.request.args['z'])
    callback = flask.request.args.get('callback')
    tile = Tile(x, y, z)
    point = geomhelpers.mercator_to_lonlat(tile.geom.centroid)

    res = {
      "type": "FeatureCollection",
      "features": [
        {
          "type": "Feature",
          "id": x + y * 2**20 + z * 2**40,
          "geometry": {
            "type": "Point",
            "coordinates": [point.y, point.x]
          },
          "properties": {
            "balloonContent": "content",
            "clusterCaption": "label 1",
            "hintContent": "Hint text"
          }
        }
      ]
    }

    # res = [{
    #         "type": "Feature",
    #         "geometry": {
    #             "type": "Point",
    #             "coordinates": [
    #                 point.x,
    #                 point.y
    #             ]
    #         }
    #     }]

    @flask.after_this_request
    def add_header(response):
        response.headers['Content-Type'] = 'application/json'
        return response
    if callback:
        return callback + '(' + json.dumps(res) + ')'
    return json.dumps(res)


def unused_port():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.bind(('localhost', 0))
    addr, port = s.getsockname()
    s.close()
    return port


@opster.command()
def main(port=('p', 0, 'port to listen on')):
    if not port:
        port = unused_port()

    print "url template for factory http://{host}:{port}/tiles?x=0&y=0&z=0".format(
            host=os.uname()[1], port=port)
    p = PIDFile("mock.pid")
    app.run(host="0.0.0.0", port=port, use_debugger=True)


if __name__ == '__main__':
    main()
