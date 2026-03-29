import logging

from flask import Flask
from flask_cors import CORS

from app.views import api_blueprint


def create_app():
    logging.basicConfig(
        level=logging.INFO,
        format="%(asctime)s %(levelname)s %(name)s: %(message)s",
    )
    app = Flask(__name__)
    app.logger.setLevel(logging.INFO)
    CORS(app)
    app.register_blueprint(api_blueprint)

    return app
