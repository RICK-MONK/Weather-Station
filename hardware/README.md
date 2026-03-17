# Hardware

This folder is organized by status so it is easier to see which sketches are current and which ones are older experiments.

## Current Sketches

- `active/weather_station/weather_station.ino` is the standalone ESP32 weather station sketch.
- `active/espnow_weather/sensor_espnow_weather_sender/sensor_espnow_weather_sender.ino` is the sensor-side ESP-NOW weather sender.
- `active/espnow_weather/display_espnow_weather_gateway/display_espnow_weather_gateway.ino` is the display and backend gateway for the ESP-NOW weather setup.

## Folder Layout

- `active/` contains the sketches you are most likely to flash right now.
- `experiments/` contains older link tests and display experiments kept for reference.
- `reference/` contains older lab material that is not part of the current weather workflow.

## Which Sketch Am I Using?

- Use `active/weather_station/weather_station.ino` if you want one main weather-station firmware folder.
- Use the two sketches in `active/espnow_weather/` if you want the split ESP-NOW setup with a sensor node and a display gateway.
- Check the README inside each folder before flashing if you are switching between setups.
