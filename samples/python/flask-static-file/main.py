import flask
import opster

app = flask.Flask(__name__)


def send_json(path_to_file):
    with open(path_to_file) as f:
        content = f.read()
    resp = flask.make_response(content)
    resp.mimetype = "application/json"
    return resp


@app.route("/f")
def get_api():
    return send_json('f.json')


@opster.command()
def main():
    app.run(host="0.0.0.0", port=9016, use_debugger=True)


if __name__ == '__main__':
    main()
