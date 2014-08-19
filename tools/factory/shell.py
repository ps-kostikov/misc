#!/usr/bin/python
import logging
from celery.result import AsyncResult
from celery.execute import send_task
from yandex.maps.factory import db
from yandex.maps.factory.models import vec, sat

logging.basicConfig(level=logging.DEBUG)
session = db.create_session(db.META_DB)
