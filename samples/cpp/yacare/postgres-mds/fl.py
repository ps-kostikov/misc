#!/usr/bin/python
# -*- coding: utf-8 -*-
import flask
import contextlib
import requests

import sqlalchemy
from sqlalchemy import orm

app = flask.Flask(__name__)

engine = sqlalchemy.create_engine(
    'postgresql://mapadmin:mapadmin@target056i.load.yandex.net:5432/test',
    pool_size=32,
    connect_args=dict(sslmode="disable")
)
session_maker = orm.sessionmaker(bind=engine, autocommit=False)
http_session = requests.Session()

@contextlib.contextmanager
def session_ctx(maker):
    session = maker()
    try:
        yield session
    finally:
        session.close()

@app.route("/signals-renderer")
def get_tile():
    x = flask.request.values.get('x')
    y = flask.request.values.get('y')
    z = flask.request.values.get('z')
    if x is None or y is None  or z is None:
        flask.abort(400)

    query = "select mds_key from signals_tiles where style = 'point' and x = {0} and y = {1} and z = {2};".format(
        x, y, z)

    with session_ctx(session_maker) as session:
        result = session.execute(query)
        if not result.rowcount:
            flask.abort(404)

    mds_key = result.scalar()
    resp = http_session.get("http://target056i.load.yandex.net:17083/get-mapsfactory_signals/{0}".format(mds_key))
    return resp.text

