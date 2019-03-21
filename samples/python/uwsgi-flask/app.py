from flask import Flask, Blueprint
import time
import logging

logging.basicConfig(format='%(asctime)s %(levelname)s:%(message)s', level=logging.DEBUG)


test_bp = Blueprint('test', 'bp_name')

@test_bp.record_once
def init_blueprint(state):
    for i in range(100000000): j=i
    logging.info('bp record once')

@test_bp.route('/test')
def test():
    return 'Hello, World!'


def create_app():
    for i in range(100000000): j=i
    logging.info("long operation before app created")
    return Flask('app_name')

app = create_app()
app.register_blueprint(test_bp)

if __name__ == '__main__':
    app.run()
