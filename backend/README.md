# Backend

Minimal Flask backend scaffold for the weather station project.

## Setup

```bash
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
python run.py
```

The Flask server listens on `0.0.0.0:5000` by default so devices on the same LAN can reach it.
Override with `FLASK_HOST`, `FLASK_PORT`, or `FLASK_DEBUG` in your environment if needed.

## Available Routes

- `GET /api/health`
- `POST /api/weather/update`
- `GET /api/weather/latest`
- `GET /api/weather/recent?limit=50`
- `GET /api/control/status`
- `POST /api/control/action`

## Weather Payload

`POST /api/weather/update` expects JSON like:

```json
{
  "id": "620169874",
  "type": "weather",
  "temperature": 24.6,
  "humidity": 53.1,
  "heatIndex": 24.5,
  "pressure": 932.9,
  "altitude": 691.8,
  "soilMoisture": 62,
  "soilRaw": 372,
  "soilMoisturePercent": 62,
  "dhtOk": 1,
  "bmpOk": 1,
  "soilOk": 1,
  "sampleMillis": 182340
}
```

The backend adds `timestamp` before storing the document in MongoDB.

## Control API

`GET /api/control/status` returns maintenance mode, queue depth, recent control activity, and the latest weather snapshot metadata used by the frontend control panel.

`POST /api/control/action` expects JSON like:

```json
{
  "action": "queue-gateway-restart"
}
```

Supported actions:

- `enable-maintenance`
- `disable-maintenance`
- `acknowledge-alerts`
- `queue-gateway-refresh`
- `queue-gateway-restart`
- `queue-sensor-restart`
- `clear-pending-commands`

## Notes

- `app/functions.py` connects to MongoDB and persists weather updates.
- `app/mqtt.py` is still a placeholder MQTT client.
- Environment variables are loaded from `.env` through `app/config.py`.
