from flask import Blueprint, jsonify, request

from app.functions import DB
from app.mqtt import MQTTClient


api_blueprint = Blueprint("api", __name__)
db = DB()
mqtt = MQTTClient()

REQUIRED_WEATHER_FIELDS = (
    "id",
    "type",
    "temperature",
    "humidity",
    "heatIndex",
    "pressure",
    "altitude",
    "soilMoisture",
)


def validate_weather_payload(payload):
    missing = [field for field in REQUIRED_WEATHER_FIELDS if field not in payload]
    if missing:
        return f"Missing required fields: {', '.join(missing)}"

    if payload.get("type") != "weather":
        return "Field 'type' must be 'weather'"

    numeric_fields = (
        "temperature",
        "humidity",
        "heatIndex",
        "pressure",
        "altitude",
        "soilMoisture",
    )

    for field in numeric_fields:
        try:
            float(payload[field])
        except (TypeError, ValueError):
            return f"Field '{field}' must be numeric"

    return None


@api_blueprint.route("/api/health", methods=["GET"])
def health():
    return jsonify(
        {
            "status": "complete",
            "data": {
                "service": "weather-station-backend",
                "message": "API is running",
            },
        }
    )


@api_blueprint.route("/api/weather/update", methods=["POST"])
def weather_update():
    payload = request.get_json(silent=True) or {}
    error = validate_weather_payload(payload)
    if error:
        return jsonify({"status": "error", "error": error}), 400

    saved = db.save_weather_update(payload)
    if saved["status"] != "complete":
        return jsonify({"status": "error", "error": saved["message"]}), 500

    mqtt.publish("weatherstation/update", payload)

    return jsonify(
        {
            "status": "complete",
            "data": {
                "message": "Weather update received",
                "received": payload,
                "saved": saved,
            },
        }
    )


@api_blueprint.route("/api/weather/latest", methods=["GET"])
def weather_latest():
    latest = db.get_latest_weather()
    if latest.get("status") == "error":
        return jsonify({"status": "error", "error": latest["message"]}), 500

    if latest.get("status") == "empty":
        return jsonify({"status": "empty", "data": latest}), 404

    return jsonify({"status": "found", "data": latest})


@api_blueprint.route("/api/weather/recent", methods=["GET"])
def weather_recent():
    limit = request.args.get("limit", default=50)
    recent = db.get_recent_weather(limit=limit)
    if isinstance(recent, dict) and recent.get("status") == "error":
        return jsonify({"status": "error", "error": recent["message"]}), 500

    if isinstance(recent, dict) and recent.get("status") == "empty":
        return jsonify({"status": "empty", "data": recent}), 404

    return jsonify({"status": "found", "data": recent})
