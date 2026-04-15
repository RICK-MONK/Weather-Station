---
marp: true
theme: default
paginate: true
size: 16:9
footer: "ELET2415 | ESP32 Weather Station Project"
style: |
  section {
    font-family: Arial, Helvetica, sans-serif;
    font-size: 28px;
    padding: 48px 56px;
    background: #f6f9fc;
    color: #153047;
  }
  h1, h2 {
    color: #0f4c5c;
  }
  h1 {
    font-size: 2.0em;
  }
  h2 {
    font-size: 1.35em;
    margin-bottom: 0.4em;
  }
  strong {
    color: #0b7285;
  }
  code {
    background: #e7f1f7;
    color: #114b5f;
    padding: 0.12em 0.28em;
    border-radius: 0.2em;
  }
  table {
    font-size: 0.78em;
  }
  ul {
    line-height: 1.35;
  }
  .lead {
    background: linear-gradient(135deg, #0f4c5c 0%, #1f6f8b 100%);
    color: #ffffff;
  }
  .lead h1, .lead h2, .lead strong {
    color: #ffffff;
  }
  .small {
    font-size: 22px;
  }
  .center {
    text-align: center;
  }
---

<!-- _class: lead -->

# ESP32 IoT Weather Station
## PCB Sensor Node, Display Gateway, Flask Backend, and Vue Dashboard

**Course:** ELET2415  
**Focus:** Real-time sensing, wireless transfer, backend storage, and dashboard visualization

<!--
Open with the full system idea: this is not just a sensor readout, it is an end-to-end IoT monitoring platform.
-->

---

# Problem We Wanted To Solve

- Many low-cost sensor projects stop at raw serial output
- Users need **readable live status**, **history**, and **remote visibility**
- A useful weather station should still be informative when a sensor fails temporarily
- The real goal was to build a **complete sensing-to-dashboard pipeline**

<!--
Emphasize that the project is about usability and integration, not just collecting one reading.
-->

---

# Main Objectives

- Measure **temperature**, **humidity**, **heat index**, **pressure**, **altitude estimate**, and **soil moisture**
- Send readings wirelessly with **ESP-NOW**
- Forward readings to a backend over **Wi-Fi and HTTP**
- Store data in **MongoDB**
- Show live conditions on a **TFT display** and a **web dashboard**
- Improve reliability with **validation**, **smoothing**, and **fallback logic**

---

# System Architecture

| Stage | Role |
| --- | --- |
| **Sensor PCB** | Reads DHT22, BMP280, and soil sensor |
| **ESP-NOW Link** | Sends compact sensor payloads |
| **Display / Gateway PCB** | Receives data, updates TFT, forwards to backend |
| **Flask Backend** | Validates, timestamps, stores, and serves data |
| **Vue Dashboard** | Displays live status, trends, analysis, and control views |

**Flow:** Sensors -> Sensor ESP32 -> ESP-NOW -> Gateway ESP32 -> Wi-Fi/HTTP -> Flask -> MongoDB -> Vue UI

---

# Hardware Design

## Sensor PCB

- **ESP32** microcontroller
- **DHT22** on `GPIO 4`
- **BMP280** on `GPIO 21/22` over I2C
- **Soil sensor** on `GPIO 34`

## Display / Gateway PCB

- **ESP32** microcontroller
- **TFT display** using `TFT_eSPI`
- Receives **ESP-NOW**
- Sends backend updates over **Wi-Fi**

---

# Why Two ESP32 Boards?

- Separates **sensing** from **display and gateway** tasks
- Makes the system more **modular**
- Lets the sensor board be placed where readings are best taken
- Keeps the gateway focused on:
  - display updates
  - stale-data checks
  - backend posting

**Result:** cleaner architecture and easier future expansion

---

# Sensor Firmware Logic

- Reads sensors every **2 seconds**
- Retries failed **DHT22** and **BMP280** reads
- Validates sensor ranges before accepting a sample
- Applies a **moving average** window for smoother output
- Sends **partial updates** if one sensor is unavailable
- Reuses the **last valid baseline** instead of dropping all data

**Health flags included:** `dhtOk`, `bmpOk`, `soilOk`

---

# Data Processing And Calibration

- **Heat index** is calculated from temperature and humidity
- **Altitude** is estimated from BMP280 pressure using sea-level reference pressure
- **Soil moisture** is mapped from raw ADC values to a **relative 0-100% scale**

## Important technical notes

- Soil calibration uses:
  - dry = `720`
  - wet = `160`
- Sea-level reference pressure = `1013.25 hPa`

**Interpretation matters:** soil moisture is relative, and altitude is an estimate

---

# Gateway Node Responsibilities

- Receives sensor packets over **ESP-NOW**
- Displays live values and status on the **TFT**
- Tracks **packet age** and stale readings
- Forwards JSON payloads to the backend using **HTTP POST**

## Current runtime behavior

- UI refresh interval: `500 ms`
- Data considered stale after: `8000 ms`
- Backend forward delay after packet: about `4000 ms`

---

# Backend Design

- Built with **Flask**
- Stores weather data in **MongoDB**
- Validates payloads before saving
- Adds a backend-side **timestamp**
- Attempts **MQTT publish** after successful save

## Main routes

- `GET /api/health`
- `POST /api/weather/update`
- `GET /api/weather/latest`
- `GET /api/weather/recent`
- `GET /api/weather/analysis`
- `GET /api/control/status`
- `POST /api/control/action`

---

# Frontend Dashboard

- Built with **Vue**, **Vuetify**, **Pinia**, and **Vue Router**
- Polls backend every **5 seconds**
- Supports:
  - **Home dashboard**
  - **Live sensor trends**
  - **Control page**
  - **Analysis page**
- Includes **metric / imperial** unit conversion in the render layer

## User-facing benefits

- live conditions
- recent history
- freshness indicators
- sensor fallback visibility

---

# Key Engineering Challenges

- **Sensor noise and invalid reads**
- **Temporary sensor failure**
- **Soil sensor ambiguity**
- **ESP-NOW and Wi-Fi coexistence**
- **Need to show stale or fallback-based data clearly**

## Solutions

- retries
- validation limits
- smoothing
- last-valid fallback
- health flags
- freshness indicators in UI

---

# Results And What We Achieved

- Built a full **sensing-to-dashboard weather pipeline**
- Achieved both **local** and **remote** visibility
- Added resilience through **validation**, **fallback**, and **health flags**
- Created a modular system that can be extended later

**This is stronger than a basic microcontroller demo because it combines:**

- hardware
- firmware
- wireless networking
- backend storage
- frontend visualization

---

# Current Limitations

- Some network settings are still **hard-coded in firmware**
- Backend control queue is **in memory**, not persistent
- Device-side execution of queued control actions is **not implemented**
- Altitude is **estimated**, not directly measured
- Soil moisture is **relative**, not an absolute physical measurement

**These are good areas to mention honestly in the presentation**

---

# Future Improvements

- Add enclosure and long-term outdoor protection
- Improve calibration with more field data
- Persist control state in MongoDB
- Add device-side command handling
- Add alerts and notifications
- Extend the system toward **automatic irrigation** or smart control

---

# Demo Flow

1. Show the **sensor PCB**
2. Show the **display / gateway PCB**
3. Explain the **architecture flow**
4. Show the **TFT updating**
5. Show backend activity or stored data
6. Show the **dashboard** and **analysis view**

**Best takeaway:** this project integrates embedded sensing, wireless communication, backend processing, and frontend monitoring in one working system

---

<!-- _class: lead -->

# Conclusion

This project demonstrates an **end-to-end IoT weather station** built with:

- ESP32 hardware
- validation-focused firmware
- wireless communication
- Flask backend
- MongoDB storage
- Vue dashboard visualization

**Final message:** it is not just reading sensors, it is building a usable environmental monitoring platform.
