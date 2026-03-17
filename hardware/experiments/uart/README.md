# UART Experiments

Use this folder for two-board UART link tests before integrating sensors or the TFT dashboard.

## Sketches

- `sensor_uart_hello/sensor_uart_hello.ino` sends `HELLO_FROM_SENSOR` once per second.
- `display_uart_hello/display_uart_hello.ino` receives messages and prints them to the serial monitor.

## Wiring

```text
Sensor ESP32 GPIO17  ->  Display ESP32 GPIO16
Sensor ESP32 GND     ->  Display ESP32 GND
```
