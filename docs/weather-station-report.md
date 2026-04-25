# Smarty Weather Station Report Draft

## 1. Introduction

The increasing demand for real-time environmental monitoring has led to wider use of Internet of Things systems in agriculture, laboratories, greenhouses, and smart-site monitoring. Many low-cost weather or sensor projects are limited to raw serial output and do not provide reliable data transfer, storage, or remote visibility. That makes them useful for testing, but weak as complete monitoring systems.

This project presents the design and implementation of **Smarty Weather Station**, an IoT-based weather and soil monitoring system built around ESP32 hardware, wireless communication, backend data storage, and a browser dashboard. The system measures environmental conditions in real time, transmits them wirelessly, stores them in MongoDB, and presents them through both a touch TFT display and a Vue-based frontend.

## 2. System Overview

The system is divided into three main layers:

### 2.1 Hardware Layer

The hardware layer is responsible for data acquisition and local display. A sensor ESP32 reads the environmental sensors, while a second ESP32 acts as a display and gateway node.

### 2.2 Backend Layer

The backend layer is implemented in Flask. It receives updates through HTTP requests, validates payloads, timestamps records, stores them in MongoDB, and exposes routes for retrieval and analysis.

### 2.3 Frontend Layer

The frontend layer is implemented in Vue and Vuetify. It displays live values, recent trends, analysis views, and control information in a browser dashboard.

## 3. Functional Description

The system measures and presents the following parameters:

- Temperature in degrees Celsius
- Relative humidity in percent
- Heat index in degrees Celsius
- Atmospheric pressure in hectopascals
- Altitude estimate in meters
- Soil moisture as a calibrated percentage

The main features of the system are:

- real-time sensor acquisition
- wireless board-to-board communication
- local visibility through a touch TFT display
- remote monitoring through a browser dashboard
- backend storage for recent and historical data
- trend analysis and system-state visibility

## 4. System Architecture

The end-to-end data flow of the project is:

`Sensors -> Sensor ESP32 -> ESP-NOW -> Gateway ESP32 -> Wi-Fi -> Flask Backend -> MongoDB -> Web Dashboard`

The sensor ESP32 collects readings from the DHT22, BMP280, and soil sensor. These values are processed and packed into a payload, then sent to the gateway ESP32 using ESP-NOW. The gateway board updates the local TFT display, checks packet freshness, and forwards the data to the Flask backend by HTTP POST. The backend validates and stores the record in MongoDB, then serves it to the Vue dashboard for live and historical visualization.

## 5. Hardware Design

### 5.1 Main Components

- ESP32 sensor development board
- ESP32-2432S028 touch display and gateway board
- DHT22 temperature and humidity sensor
- BMP280 pressure sensor
- capacitive soil moisture sensor

### 5.2 Bill Of Materials

| Item | Quantity | Estimated cost (USD) | Purpose |
| --- | ---: | ---: | --- |
| ESP32 development board (sensor node) | 1 | 7.69 | sensing and packet transmission |
| ESP32-2432S028 touch display board (gateway node) | 1 | 13.99 | local display, touch UI, and gateway forwarding |
| DHT22 sensor | 1 | 8.90 | temperature and humidity measurement |
| BMP280 module | 1 | 9.95 | pressure measurement and derived altitude estimate |
| Capacitive soil moisture sensor | 1 | 5.90 | soil condition sensing |
| USB power leads and small support parts | 1 set | 5.90 | board power and basic hookup support |
| Wiring, headers, and connectors | 1 set | 3.95 | interconnection between boards and sensors |
| Prototype breadboard hardware | 1 set | 4.95 | physical assembly and integration |

Estimated hardware total: **$61.23 USD** excluding shipping and tax, based on representative online prices checked on **April 24, 2026**.

### 5.3 Sensor Interfaces

- DHT22 data line on GPIO 4
- BMP280 on I2C using GPIO 21 for SDA and GPIO 22 for SCL
- soil sensor on analog GPIO 34

### 5.4 Design Justification

The ESP32 was selected because it provides ADC support, Wi-Fi connectivity, and ESP-NOW support on a single platform. The BMP280 was chosen because it provides pressure data and supports derived altitude estimation, which reduces component count. The gateway hardware uses an ESP32-2432S028 touch display board, so the second ESP32 and the local TFT interface are combined into one demonstration unit.

The project uses two ESP32-based boards instead of one. This separates sensing from presentation and network forwarding duties. As a result, the sensor board can remain close to the measurement point while the display board acts as a wireless receiver and user-facing interface.

## 6. Software Design

### 6.1 Embedded Firmware

The embedded firmware performs periodic sensor acquisition, validation, smoothing, and payload generation. It calculates heat index from temperature and humidity and maps raw soil readings into a calibrated percentage scale. The firmware also keeps last-valid samples so temporary failures do not break the entire update cycle.

### 6.2 Backend System

The backend is built in Flask. The main data-ingest route is:

`/api/weather/update`

When a payload is received, the backend validates required fields, converts values to the correct types, timestamps the document, and stores it in MongoDB. Additional routes provide latest readings, recent-history queries, analysis data, control status, and system metadata.

Example document:

```json
{
  "temperature": 29.6,
  "humidity": 67.5,
  "heatIndex": 37.4,
  "pressure": 1013.2,
  "altitude": 150.2,
  "soilMoisture": 85,
  "timestamp": 1774896919
}
```

### 6.3 Frontend System

The frontend is built using Vue.js and Vuetify. It provides:

- live sensor cards
- recent trend graphs
- analysis pages for date-range review
- system and control views for operational visibility

## 7. Software Flow

The overall operation of the software can be summarized as follows:

1. Initialize sensors, communication, and display services.
2. Read sensor data from the DHT22, BMP280, and soil sensor.
3. Validate the readings against expected ranges.
4. Perform calculations such as heat index and soil moisture percentage.
5. Transmit the payload to the gateway ESP32 using ESP-NOW.
6. Forward the validated payload to the backend with HTTP POST.
7. Store the data in MongoDB.
8. Update the frontend and local display with the latest values.

## 8. Challenges And Solutions

### 8.1 Negative Or Unstable Altitude Readings

Altitude is derived from pressure rather than measured directly. Incorrect sea-level pressure assumptions can lead to unrealistic altitude values. This was addressed by using a fixed sea-level pressure reference and treating altitude as an estimate rather than an absolute reference.

### 8.2 Sensor Noise

Low-cost sensors, especially the soil moisture sensor, can produce unstable values. To reduce noise, the firmware applies smoothing and uses calibration to convert raw values into a relative percentage scale.

### 8.3 Sensor Read Failures

The DHT22 and BMP280 can occasionally fail to return valid data. Retry logic and fallback to last-valid readings were implemented to prevent a temporary failure from corrupting the displayed output.

### 8.4 Wireless Communication Reliability

ESP-NOW communication can fail if channel settings are misaligned. This was handled by ensuring that both boards operate on the same channel and by exposing packet-age freshness indicators in the UI.

## 9. Future Improvements

Possible future extensions include:

- mobile application support
- cloud deployment for wider remote access
- alerting through email or SMS
- automated irrigation or actuator control
- advanced analytics and predictive monitoring

## 10. Conclusion

Smarty Weather Station demonstrates a complete IoT monitoring system rather than a simple sensor demo. It combines embedded sensing, wireless communication, backend processing, MongoDB storage, and frontend visualization in one coherent design. The project also addresses practical engineering concerns such as validation, fallback handling, calibration, and communication freshness. This makes it a strong platform for future expansion into smart agriculture and environmental automation.
