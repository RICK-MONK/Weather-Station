from flask import Blueprint, current_app, jsonify, request

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

OPTIONAL_NUMERIC_WEATHER_FIELDS = (
    "seaLevelPressureHpa",
    "altitudeEstimated",
    "soilRaw",
    "soilMoisturePercent",
    "dhtOk",
    "bmpOk",
    "soilOk",
    "sampleMillis",
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

    numeric_fields_to_validate = list(numeric_fields)
    numeric_fields_to_validate.extend(
        field for field in OPTIONAL_NUMERIC_WEATHER_FIELDS if field in payload
    )

    for field in numeric_fields_to_validate:
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
    current_app.logger.info(
        "POST /api/weather/update from %s payload=%s",
        request.remote_addr,
        payload,
    )
    error = validate_weather_payload(payload)
    if error:
        current_app.logger.warning("Rejected weather update: %s", error)
        return jsonify({"status": "error", "error": error}), 400

    saved = db.save_weather_update(payload)
    if saved["status"] != "complete":
        current_app.logger.error("Weather update save failed: %s", saved["message"])
        return jsonify({"status": "error", "error": saved["message"]}), 500

    publish_ok = mqtt.publish(mqtt.topic, saved["data"])
    if not publish_ok:
        current_app.logger.error(
            "MQTT publish failed for device=%s topic=%s",
            payload.get("id"),
            mqtt.topic,
        )
    else:
        current_app.logger.info(
            "MQTT publish complete for device=%s topic=%s",
            payload.get("id"),
            mqtt.topic,
        )
    current_app.logger.info(
        "Weather update saved for device=%s inserted_id=%s",
        payload.get("id"),
        saved.get("inserted_id"),
    )

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
    current_app.logger.info("GET /api/weather/latest")
    latest = db.get_latest_weather()
    if latest.get("status") == "error":
        current_app.logger.error("Latest weather query failed: %s", latest["message"])
        return jsonify({"status": "error", "error": latest["message"]}), 500

    if latest.get("status") == "empty":
        current_app.logger.info("Latest weather query returned no data")
        return jsonify({"status": "empty", "data": latest}), 404

    return jsonify({"status": "found", "data": latest})


@api_blueprint.route("/api/weather/recent", methods=["GET"])
def weather_recent():
    limit = request.args.get("limit", default=50)
    current_app.logger.info("GET /api/weather/recent limit=%s", limit)
    recent = db.get_recent_weather(limit=limit)
    if isinstance(recent, dict) and recent.get("status") == "error":
        current_app.logger.error("Recent weather query failed: %s", recent["message"])
        return jsonify({"status": "error", "error": recent["message"]}), 500

    if isinstance(recent, dict) and recent.get("status") == "empty":
        current_app.logger.info("Recent weather query returned no data")
        return jsonify({"status": "empty", "data": recent}), 404

    return jsonify({"status": "found", "data": recent})
