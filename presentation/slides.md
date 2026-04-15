---
marp: true
theme: weather-station
paginate: true
size: 16:9
footer: "ELET2415 | Smarty Weather Station"
style: |
  section.lead h1 {
    max-width: 52%;
  }
  section.lead h2 {
    max-width: 48%;
    font-size: 1.05em;
  }
  .split {
    display: grid;
    grid-template-columns: 1.02fr 0.98fr;
    gap: 26px;
    align-items: center;
  }
  .split-tight {
    display: grid;
    grid-template-columns: 1.1fr 0.9fr;
    gap: 20px;
    align-items: start;
  }
  .split img,
  .gallery img,
  .gallery-quad img,
  .hardware-grid img {
    width: 100%;
    border-radius: 18px;
    border: 1px solid rgba(36, 77, 115, 0.12);
    box-shadow: 0 14px 28px rgba(15, 91, 110, 0.16), 0 4px 10px rgba(15, 91, 110, 0.08);
    background: #ffffff;
    object-fit: contain;
  }
  .split img {
    max-height: 420px;
  }
  .gallery {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 18px;
    margin-top: 12px;
  }
  .gallery-quad {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 18px;
    margin-top: 10px;
  }
  .gallery img {
    height: 220px;
  }
  .gallery-quad img {
    height: 205px;
  }
  .gallery p,
  .gallery-quad p,
  .hardware-grid p {
    margin-top: 8px;
    font-size: 0.58em;
    text-align: center;
    color: #4a677d;
  }
  .hardware-grid {
    display: grid;
    grid-template-columns: repeat(5, 1fr);
    gap: 14px;
    margin-top: 12px;
  }
  .hardware-grid img {
    height: 138px;
  }
  .note {
    margin-top: 10px;
    font-size: 0.7em;
    color: #4a677d;
  }
  .flow-box {
    padding: 18px 20px;
    border-radius: 18px;
    background: rgba(255,255,255,0.72);
    border: 1px solid rgba(36, 77, 115, 0.12);
  }
  .reason-grid {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 18px;
    margin-top: 10px;
  }
  .reason-grid > div,
  .step-grid > div,
  .label-card {
    padding: 18px 20px;
    border-radius: 18px;
    background: rgba(255,255,255,0.78);
    border: 1px solid rgba(36, 77, 115, 0.12);
    box-shadow: 0 10px 20px rgba(15, 91, 110, 0.08);
  }
  .reason-grid h3,
  .step-grid h3,
  .label-card h3 {
    margin: 0 0 10px 0;
    font-size: 0.95em;
  }
  .reason-grid ul,
  .step-grid ul,
  .label-card ul {
    margin: 0;
    padding-left: 22px;
    font-size: 0.78em;
  }
  .step-grid {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 14px;
    margin-top: 18px;
  }
  .step-grid p {
    margin: 8px 0 0 0;
    font-size: 0.72em;
    color: #4a677d;
  }
  .label-gallery {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 18px;
    margin-top: 12px;
  }
  .label-gallery img {
    width: 100%;
    height: 175px;
    object-fit: contain;
    border-radius: 16px;
    border: 1px solid rgba(36, 77, 115, 0.12);
    box-shadow: 0 12px 22px rgba(15, 91, 110, 0.12);
    background: #ffffff;
  }
  .label-card p {
    margin: 8px 0 0 0;
    font-size: 0.68em;
    color: #4a677d;
  }
  .impact-box {
    padding: 24px 28px;
    border-radius: 22px;
    background: linear-gradient(135deg, rgba(32, 132, 166, 0.1), rgba(120, 193, 122, 0.12));
    border: 1px solid rgba(36, 77, 115, 0.12);
    margin-top: 18px;
  }
  .gallery-quad-wide {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 18px;
    margin-top: 10px;
  }
  .gallery-quad-wide img {
    width: 100%;
    height: 215px;
    object-fit: contain;
    border-radius: 18px;
    border: 1px solid rgba(36, 77, 115, 0.12);
    box-shadow: 0 12px 22px rgba(15, 91, 110, 0.12);
    background: #ffffff;
  }
  .gallery-quad-wide p {
    margin-top: 8px;
    font-size: 0.6em;
    text-align: center;
    color: #4a677d;
  }
  .compact td,
  .compact th {
    font-size: 0.78em;
  }
---

<!-- _class: lead -->
![bg right:40% opacity:.98](./assets/images/prototype-display-photo.jpg)

# Smarty Weather Station
## An IoT environmental monitoring system

**Presenter:** Patrick Marsden  
**Course:** ELET2415

---

# Problem And Objective

- Many low-cost weather builds stop at raw sensor output and do not support remote monitoring
- A practical system must collect data, move it wirelessly, store it, and present it clearly
- This project was designed as a complete **sensor-to-dashboard IoT pipeline**

**Main objective**

- Measure environmental conditions in real time and make them visible on a **wireless local display** and a **web dashboard**

---

# System Overview

![System architecture](./assets/images/iot-architecture-reference.jpg)

<p class="note">Data flow: Sensors -> Sensor ESP32 -> ESP-NOW -> Gateway ESP32 -> Wi-Fi/HTTP -> Flask backend -> MongoDB -> Vue dashboard</p>

---

# Data Flow Diagram

![Animated-style data flow](./assets/images/data-flow-animated.svg)

---

# Hardware Design And Wiring

<div class="split">
<div>

- **ESP32** selected for Wi-Fi, ESP-NOW, ADC, and flexible GPIO
- **DHT22** measures temperature and humidity
- **BMP280** measures pressure and supports altitude estimation
- **Soil sensor** adds plant-monitoring value
- **TFT display** provides a local interface without opening a browser

<p class="note">Key interfaces: DHT22 on GPIO 4, BMP280 on I2C GPIO 21/22, soil sensor on ADC GPIO 34.</p>

</div>
<div>

![Wiring diagram](./assets/images/esp32-soil-dht-wiring.jpg)

</div>
</div>

---

# PCB / Wiring Diagram

<div class="split">
<div>

- This slide is important because **ELET2415 expects the hardware interconnections to be explained clearly**
- The PCB/schematic view shows the logic of the circuit: power, ESP32, and sensor paths
- The wiring view shows the practical test connections used to prove the system

**Explain these links clearly**

- DHT22 on a digital GPIO line
- BMP280 on the `I2C` bus using `SDA` and `SCL`
- Soil moisture on an `ADC` input
- regulated power feeding the ESP32 and attached modules

</div>
<div>

![PCB and wiring focus](./assets/images/weather-station-schematic.jpg)

</div>
</div>

<p class="note">Use this slide to distinguish between a circuit-level schematic and the physical wiring arrangement used during testing and integration.</p>

---

# Prototype And Diagram References

<div class="gallery-quad-wide">
<div>

![Prototype display reference](./assets/images/prototype-display-photo.jpg)
<p>Prototype hardware photo showing the local display hardware context.</p>

</div>
<div>

![ESP32 DHT/BMP wiring reference](./assets/images/esp32-dht-bmp-wiring.jpg)
<p>Reference image showing ESP32 connections to environmental sensors.</p>

</div>
<div>

![ESP32 soil wiring reference](./assets/images/esp32-soil-dht-wiring.jpg)
<p>Reference image showing ESP32, DHT sensor, and capacitive soil sensor wiring.</p>

</div>
<div>

![Schematic reference](./assets/images/weather-station-schematic.jpg)
<p>Reference schematic showing the ESP32 module, programming header, and support circuitry.</p>

</div>
</div>

---

# Why ESP-NOW And Why Two ESP32s?

<div class="reason-grid">
<div>

### Why ESP-NOW?

- low latency board-to-board communication
- no router required between the sensing node and the display node
- efficient for short local IoT payloads
- well suited to a wireless display demonstration

</div>
<div>

### Why two ESP32s?

- separation of sensing and gateway responsibilities
- easier placement of sensors at the measurement point
- the second board becomes a **wireless display**
- clearer modular design for testing and expansion

</div>
</div>

---

# System Flow

<div class="step-grid">
<div>

### Sense

<ul>
<li>read DHT22, BMP280, and soil sensor</li>
<li>sample on a timed update cycle</li>
</ul>

<p>Capture the raw environmental data.</p>

</div>
<div>

### Validate

<ul>
<li>check sensor ranges</li>
<li>retry failed reads</li>
<li>calculate heat index and soil percentage</li>
</ul>

<p>Make the readings stable and usable.</p>

</div>
<div>

### Transmit

<ul>
<li>send payload by ESP-NOW</li>
<li>update the TFT gateway display</li>
<li>forward to Flask by HTTP</li>
</ul>

<p>Move the data from the field to the platform.</p>

</div>
<div>

### Store

<ul>
<li>validate again in Flask</li>
<li>timestamp and save in MongoDB</li>
<li>serve it to the dashboard</li>
</ul>

<p>Preserve the data for monitoring and analysis.</p>

</div>
</div>

---

# Wireless Display Experience

<div class="split">
<div>

- The display is not tied to the sensors by wiring; it receives live readings wirelessly
- The interface supports swiping between **Dashboard**, **Weather**, **Soil**, and **System**
- It exposes packet age, Wi-Fi state, sensor flags, and system status in real time
- That makes the local display a key innovation, not just an output screen

</div>
<div>

![Wireless display link](./assets/images/wireless-link.png)

</div>
</div>

---

# Backend And Data Storage

<div class="split-tight">
<div>

- Main update route: `POST /api/weather/update`
- Other routes support latest data, recent history, analysis, control, and system status
- Flask validates the payload, converts values, timestamps the record, and stores it in MongoDB

**Example document**

```json
{
  "temperature": 28.5,
  "humidity": 55.0,
  "heatIndex": 29.8,
  "pressure": 1012.5,
  "altitude": 145.2,
  "soilMoisturePercent": 48,
  "timestamp": "server-generated"
}
```

</div>
<div>

<div class="flow-box">

**Why the backend matters**

- preserves history
- supports remote access
- enables charts and analysis
- turns a sensor build into a complete IoT system

</div>

</div>
</div>

---

# Frontend Dashboard

<div class="label-gallery">
<div class="label-card">

### Live Metrics Cards

![Frontend home](./assets/images/frontend-home.png)
<p>Quick status visibility helps the user understand current conditions immediately.</p>

</div>
<div class="label-card">

### Time-Series Charts

![Frontend live](./assets/images/frontend-live.png)
<p>Trend charts answer how readings are changing, not just what they are right now.</p>

</div>
<div class="label-card">

### Analysis Views

![Frontend analysis](./assets/images/frontend-analysis.png)
<p>Historical analysis gives the system value for review, planning, and decisions.</p>

</div>
</div>

---

# Engineering Challenges And Solutions

| Challenge | Solution |
| --- | --- |
| Sensor noise and unstable values | validation ranges + smoothing |
| DHT22 or BMP280 read failures | retry logic + fallback to last valid values |
| Soil sensor interpretation | calibrated relative percentage instead of claiming absolute moisture |
| ESP-NOW communication reliability | fixed channel alignment and freshness tracking |
| Stale wireless data | packet-age status shown on the display and dashboard |

---

# Results And Demonstration Value

<div class="gallery-quad">
<div>

![Mock dashboard](./assets/images/mock-dashboard.png)
<p>Local display gives immediate on-site visibility.</p>

</div>
<div>

![Mock weather](./assets/images/mock-weather.png)
<p>The weather screen highlights climate and atmosphere values.</p>

</div>
<div>

![Mock soil](./assets/images/mock-soil.png)
<p>The soil screen turns readings into clear watering guidance.</p>

</div>
<div>

![Mock system](./assets/images/mock-system.png)
<p>The system screen exposes ESP-NOW, Wi-Fi, and health-state information.</p>

</div>
</div>

---

# Use Cases And Product Value

<div class="split">
<div>

- greenhouse and plant monitoring
- classroom IoT demonstrations
- small-site environmental tracking
- smart agriculture prototypes

**Why it matters**

- combines local awareness with remote visibility
- turns raw readings into actionable information
- creates a base for future automation

</div>
<div>

![Greenhouse use case](./assets/images/greenhouse.jpg)

</div>
</div>

---

# Future Roadmap

- Add mobile notifications for critical conditions
- Deploy the backend to a cloud host for wider access
- Improve enclosure and outdoor protection
- Add actuator control for irrigation or smart-environment automation
- Extend analytics with prediction or alert rules

![bg right:34% opacity:.12](./assets/images/roadmap.png)

---

# Conclusion

- Smarty Weather Station demonstrates a complete IoT pipeline from sensing to visualization
- The project integrates **hardware, wireless communication, backend storage, and frontend analysis**
- Engineering value comes from validation, fallback logic, and clear system status, not just sensor readings

---

# Impact Statement

<div class="impact-box">

- This project shows how a low-cost embedded system can become a practical monitoring platform
- It supports smarter environmental decisions through live data, history, and wireless accessibility
- The design is suitable for extension into agriculture, alerts, and automated control

</div>

<p class="note">The main impact is not just sensing weather data, but making environmental information visible, reliable, and actionable.</p>

---

<!-- _class: center -->

# Thank You
## Questions?
