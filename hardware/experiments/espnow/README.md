# ESP-NOW Experiments

This folder keeps older ESP-NOW link tests and intermediate receiver variants.

## Current Production Pair

- The current weather sender and gateway sketches were moved to `../../active/espnow_weather/`.

## Experimental Sketches Kept Here

- `display_espnow_receiver/display_espnow_receiver.ino` receives a basic `SensorData` packet and prints temperature and humidity.
- `sensor_espnow_sender/sensor_espnow_sender.ino` sends a basic test `SensorData` packet every two seconds.
- `display_espnow_dht22_receiver/display_espnow_dht22_receiver.ino` receives live DHT22 temperature, humidity, and heat index values.
- `sensor_espnow_dht22_sender/sensor_espnow_dht22_sender.ino` reads the DHT22 on `GPIO4` and sends live values every two seconds.
- `display_espnow_weather_receiver/display_espnow_weather_receiver.ino` receives the expanded weather packet without the Wi-Fi gateway logic.

## Display ESP32 MAC

Use this receiver MAC address for the display board:

```text
D4:E9:F4:AF:7B:F0
```

Byte array form used by the sender sketches:

```cpp
uint8_t displayAddress[] = {0xD4, 0xE9, 0xF4, 0xAF, 0x7B, 0xF0};
```
