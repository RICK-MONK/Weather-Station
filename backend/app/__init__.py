from flask import Flask
from flask_cors import CORS

from app.views import api_blueprint


def create_app():
    app = Flask(__name__)
    CORS(app)
    app.register_blueprint(api_blueprint)

    return app
