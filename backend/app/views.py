from flask import Blueprint, current_app, jsonify, request

from app.control import ControlPlane
from app.functions import DB
from app.mqtt import MQTTClient


api_blueprint = Blueprint("api", __name__)
db = DB()
mqtt = MQTTClient()
control = ControlPlane()

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


CONTROL_ACTIONS = {
    "enable-maintenance",
    "disable-maintenance",
    "acknowledge-alerts",
    "queue-gateway-refresh",
    "queue-gateway-restart",
    "queue-sensor-restart",
    "clear-pending-commands",
}

SYSTEM_SCHEMA_FIELDS = (
    {"name": "id", "type": "string", "required": True},
    {"name": "type", "type": "string", "required": True},
    {"name": "temperature", "type": "float", "required": True},
    {"name": "humidity", "type": "float", "required": True},
    {"name": "heatIndex", "type": "float", "required": True},
    {"name": "pressure", "type": "float", "required": True},
    {"name": "altitude", "type": "float", "required": True},
    {"name": "soilMoisture", "type": "int", "required": True},
    {"name": "seaLevelPressureHpa", "type": "float", "required": False},
    {"name": "altitudeEstimated", "type": "bool", "required": False},
    {"name": "soilRaw", "type": "int", "required": False},
    {"name": "soilMoisturePercent", "type": "int", "required": False},
    {"name": "dhtOk", "type": "bool", "required": False},
    {"name": "bmpOk", "type": "bool", "required": False},
    {"name": "soilOk", "type": "bool", "required": False},
    {"name": "sampleMillis", "type": "int", "required": False},
    {"name": "timestamp", "type": "int", "required": True, "source": "backend"},
)


def get_latest_weather_for_control():
    latest = db.get_latest_weather()

    if isinstance(latest, dict) and latest.get("status") in {"error", "empty"}:
        return None, latest

    return latest, None


@api_blueprint.route("/api/system/info", methods=["GET"])
def system_info():
    current_app.logger.info("GET /api/system/info")
    return jsonify(
        {
            "status": "complete",
            "data": {
                "database": {
                    "engine": "MongoDB",
                    "database": db.mongo_db,
                    "collection": db.mongo_collection,
                    "host": db.mongo_host,
                    "port": db.mongo_port,
                    "uriConfigured": bool(db.mongo_uri),
                },
                "schema": {
                    "name": "weather",
                    "description": "Time-series weather documents stored per reading.",
                    "fields": SYSTEM_SCHEMA_FIELDS,
                },
                "api": {
                    "basePath": "/api",
                    "routes": [
                        "GET /api/health",
                        "POST /api/weather/update",
                        "GET /api/weather/latest",
                        "GET /api/weather/recent?limit=50",
                        "GET /api/weather/analysis?start_ts=<ts>&end_ts=<ts>",
                        "GET /api/control/status",
                        "POST /api/control/action",
                        "GET /api/system/info",
                    ],
                },
            },
        }
    )


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
    start_ts = request.args.get("start_ts")
    end_ts = request.args.get("end_ts")
    current_app.logger.info(
        "GET /api/weather/recent limit=%s start_ts=%s end_ts=%s",
        limit,
        start_ts,
        end_ts,
    )
    recent = db.get_recent_weather(limit=limit, start_ts=start_ts, end_ts=end_ts)
    if isinstance(recent, dict) and recent.get("status") == "error":
        current_app.logger.error("Recent weather query failed: %s", recent["message"])
        return jsonify({"status": "error", "error": recent["message"]}), 500

    if isinstance(recent, dict) and recent.get("status") == "empty":
        current_app.logger.info("Recent weather query returned no data")
        return jsonify({"status": "empty", "data": recent}), 404

    return jsonify({"status": "found", "data": recent})


@api_blueprint.route("/api/weather/analysis", methods=["GET"])
def weather_analysis():
    start_ts = request.args.get("start_ts")
    end_ts = request.args.get("end_ts")
    sample_limit = request.args.get("sample_limit", default=240)
    table_limit = request.args.get("table_limit", default=10)
    current_app.logger.info(
        "GET /api/weather/analysis start_ts=%s end_ts=%s sample_limit=%s table_limit=%s",
        start_ts,
        end_ts,
        sample_limit,
        table_limit,
    )
    analysis = db.get_weather_analysis(
        start_ts=start_ts,
        end_ts=end_ts,
        sample_limit=sample_limit,
        table_limit=table_limit,
    )

    if isinstance(analysis, dict) and analysis.get("status") == "error":
        current_app.logger.error("Weather analysis query failed: %s", analysis["message"])
        return jsonify({"status": "error", "error": analysis["message"]}), 500

    if isinstance(analysis, dict) and analysis.get("status") == "empty":
        current_app.logger.info("Weather analysis query returned no data")
        return jsonify({"status": "empty", "data": analysis}), 404

    return jsonify({"status": "found", "data": analysis})


@api_blueprint.route("/api/control/status", methods=["GET"])
def control_status():
    current_app.logger.info("GET /api/control/status")
    latest_weather, latest_error = get_latest_weather_for_control()
    data = control.get_status(latest_weather)

    if latest_error:
        data["latestWeatherStatus"] = latest_error
    else:
        data["latestWeatherStatus"] = {
            "status": "found" if latest_weather else "empty",
            "message": "Latest weather snapshot loaded" if latest_weather else "No weather data available",
        }

    return jsonify({"status": "complete", "data": data})


@api_blueprint.route("/api/control/action", methods=["POST"])
def control_action():
    payload = request.get_json(silent=True) or {}
    action = str(payload.get("action", "")).strip()
    note = str(payload.get("note", "")).strip()
    current_app.logger.info("POST /api/control/action action=%s payload=%s", action, payload)

    if action not in CONTROL_ACTIONS:
        return jsonify({"status": "error", "error": f"Unsupported control action: {action}"}), 400

    if action == "enable-maintenance":
        event = control.set_maintenance_mode(True)
        message = "Maintenance mode enabled"
    elif action == "disable-maintenance":
        event = control.set_maintenance_mode(False)
        message = "Maintenance mode disabled"
    elif action == "acknowledge-alerts":
        event = control.acknowledge_alerts()
        message = "Alerts acknowledged"
    elif action == "queue-gateway-refresh":
        event = control.queue_command("gateway", "refresh-now", note=note)
        message = "Gateway refresh queued"
    elif action == "queue-gateway-restart":
        event = control.queue_command("gateway", "restart", note=note)
        message = "Gateway restart queued"
    elif action == "queue-sensor-restart":
        event = control.queue_command("sensor", "restart", note=note)
        message = "Sensor restart queued"
    else:
        event = control.clear_pending_commands()
        message = "Pending command queue cleared"

    latest_weather, latest_error = get_latest_weather_for_control()
    data = control.get_status(latest_weather)
    data["latestWeatherStatus"] = latest_error or {
        "status": "found" if latest_weather else "empty",
        "message": "Latest weather snapshot loaded" if latest_weather else "No weather data available",
    }

    return jsonify(
        {
            "status": "complete",
            "data": {
                "message": message,
                "event": event,
                "control": data,
            },
        }
    )
