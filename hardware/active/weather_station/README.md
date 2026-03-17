# Weather Station

This folder contains the main standalone weather-station firmware.

## Main Sketch

- `weather_station.ino` is the entry sketch for this folder.
- `mqtt.h` contains the MQTT and Wi-Fi publishing helpers used by the sketch.
- `ntp.h` contains NTP time helper logic.

## Use This Folder When

- You want one main ESP32 weather-station firmware folder.
- You are working on the MQTT-based weather flow instead of the split ESP-NOW gateway flow.
