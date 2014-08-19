#!/usr/bin/python
import logging
import sys

from yandex.maps.factory import db
from yandex.maps.factory.models import vec
from yandex.maps.factory.celery.vec import utils

logging.basicConfig(level=logging.DEBUG)
session = db.create_session(db.META_DB)

layer_design_id = int(sys.argv[1])
layer_design = session.query(vec.LayerDesign).get(layer_design_id)
design_rev = layer_design.design_rev_id if layer_design.design_rev_id else layer_design.design.cur_revision_id
utils.update_design_revision_locales(design_rev)
