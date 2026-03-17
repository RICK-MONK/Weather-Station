# Active Hardware

Use the sketches in this folder first.

## Current Options

- `weather_station/weather_station.ino` is the main single-folder weather station sketch.
- `espnow_weather/sensor_espnow_weather_sender/sensor_espnow_weather_sender.ino` is the sensor node for the ESP-NOW weather setup.
- `espnow_weather/display_espnow_weather_gateway/display_espnow_weather_gateway.ino` is the display node and HTTP gateway for the ESP-NOW weather setup.

## Rule Of Thumb

- If you are working on the direct weather station firmware, stay in `weather_station/`.
- If you are working on the two-board ESP-NOW version, stay in `espnow_weather/`.
