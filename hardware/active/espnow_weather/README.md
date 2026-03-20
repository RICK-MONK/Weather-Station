# Active ESP-NOW Weather Setup

This folder contains the current two-board ESP-NOW weather station sketches.

## Sketches

- `sensor_espnow_weather_sender/sensor_espnow_weather_sender.ino` runs on the sensor ESP32 and sends DHT22, BMP280, and soil readings over ESP-NOW.
- `display_espnow_weather_gateway/display_espnow_weather_gateway.ino` runs on the display ESP32, receives ESP-NOW packets, shows the readings on the TFT, and forwards them to the backend over Wi-Fi.

## Use This Folder When

- You want a dedicated sensor node and a separate display/gateway node.
- You are testing hotspot or router-based Wi-Fi forwarding from the display board to the backend.

## Notes

- Both sketches must use the same ESP-NOW channel.
- The gateway sketch also needs the correct Wi-Fi credentials and backend URL.
- Soil moisture is a relative calibrated percentage derived from the raw ADC reading, not an absolute volumetric water-content measurement.
- BMP280 altitude is an estimate derived from pressure using the configured `SEA_LEVEL_PRESSURE_HPA` reference pressure.
- The current project calibration uses `SEA_LEVEL_PRESSURE_HPA = 1032.0 hPa` for the normal deployment location.
- Weather payloads preserve `altitude` as the numeric estimate and include `seaLevelPressureHpa` plus `altitudeEstimated` for calibration traceability.
- The sender now includes `dhtOk`, `bmpOk`, `soilOk`, and `sampleMillis` in the forwarded payload for sensor-health diagnostics.
- `WIFI_CHANGE_TEST_TEMPLATE.txt` contains a copy-paste block for quickly updating Wi-Fi and backend IP settings.
