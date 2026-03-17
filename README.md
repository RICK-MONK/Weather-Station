# Weather Station Project

This repository is organized using the same backend, frontend, and hardware separation used in the ELET2415 lab repositories.

## Folder Structure

- `backend/` contains the Flask API and placeholder MQTT/database integration files.
- `frontend/` contains the Vue/Vuetify dashboard scaffold.
- `hardware/` contains ESP32-related firmware folders and hardware notes.

## Project Purpose

This project is an IoT weather station. The backend receives and serves weather data, the frontend displays a dashboard, and the hardware folder contains ESP32 firmware and supporting hardware experiments.

## Next Steps

1. Run the backend and test `GET /api/health`.
2. Run the frontend and confirm the dashboard page loads.
3. Extend `hardware/active/weather_station/` with sensor and MQTT logic.
