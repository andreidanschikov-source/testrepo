import pickle
import subprocess

from flask import Flask, request

app = Flask(__name__)


@app.route("/run")
def run():
    cmd = request.args.get("cmd")
    return subprocess.check_output(cmd, shell=True)


@app.route("/load", methods=["POST"])
def load():
    return str(pickle.loads(request.data))


@app.route("/eval")
def evaluate():
    return str(eval(request.args.get("expr")))


if __name__ == "__main__":
    app.run(host="0.0.0.0", debug=True)
