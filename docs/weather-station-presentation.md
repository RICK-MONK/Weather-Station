# Smarty Weather Station Presentation Guide

Use this as the speaking outline for the 10-minute presentation deck in `presentation/slides.md`.

## Slide 1: Title

- Introduce the project as an IoT environmental monitoring system
- State that it combines hardware, wireless communication, backend storage, and frontend visualization

## Slide 2: Problem And Objective

- Explain that many basic sensor projects stop at serial output
- State the goal: build a full sensing-to-dashboard pipeline with local and remote visibility

## Slide 3: System Overview

- Walk through the architecture from sensors to dashboard
- Emphasize that ESP-NOW is used between boards and HTTP is used to reach the backend

## Slide 4: Hardware Design And Wiring

- Introduce the ESP32, DHT22, BMP280, soil sensor, and TFT display
- Mention the key GPIO assignments and point to the wiring diagram

## Slide 5: Bill Of Materials

- List the key hardware items, quantities, and estimated costs clearly
- Explain that the build uses a sensor ESP32 plus an ESP32-2432S028 display/gateway board, so the touchscreen and second ESP32 are costed as one integrated unit

## Slide 6: Data Flow Diagram

- Use the animated-style flow to explain how readings move through the system
- Keep it simple: sensors, ESP32, gateway, backend, dashboard

## Slide 7: PCB / Wiring Diagram

- Explain the PCB/schematic view and the practical wiring view
- Call out digital GPIO, I2C, ADC, and power regulation explicitly

## Slide 8: Why ESP-NOW And Why Two ESP32s?

- Explain why ESP-NOW was chosen for local wireless communication
- Explain why the project uses two ESP32 boards instead of one

## Slide 9: System Flow

- Walk through the four-stage block flow: Sense, Validate, Transmit, Store
- Keep this short and clear

## Slide 10: Wireless Display Experience

- Show that the TFT display is wireless and not hard-wired to the sensing node
- Point out the swipeable tabs and live status information

## Slide 11: Backend And Data Storage

- Mention the main route `POST /api/weather/update`
- Explain what MongoDB is storing and why history is important

## Slide 12: Frontend Dashboard

- Label the three parts clearly: Live Metrics Cards, Time-Series Charts, Analysis Views
- Explain why each one matters, not just what it shows

## Slide 13: Engineering Challenges And Solutions

- Talk about sensor noise, invalid readings, soil calibration, and wireless reliability
- This is a strong slide for marks because it demonstrates engineering judgment

## Slide 14: Results And Demonstration Value

- Use the display screenshots to show the system is working in practice
- Point out that the project delivers on-device and browser-based monitoring

## Slide 15: Use Cases And Product Value

- Explain where the system is useful and why someone would want it

## Slide 16: Future Roadmap

- Keep this short: cloud deployment, alerts, better enclosure, actuator control

## Slide 17: Conclusion

- Reinforce that the project is an end-to-end IoT system
- Mention that it is practical, scalable, and suitable for future automation

## Slide 18: Impact Statement

- End with the broader value of the project
- Emphasize visibility, reliability, and practical environmental decision-making

## Slide 19: Thank You

- Pause for questions

## Suggested Timing

- Slides 1 to 7: 3.5 minutes
- Slides 8 to 11: 2.5 minutes
- Slides 12 to 15: 2 minutes
- Slides 16 to 19: 2 minutes

## Key Messages To Repeat

- This is not just a sensor demo; it is a complete IoT pipeline.
- The display is wireless and receives data over ESP-NOW.
- The backend and dashboard add real engineering value through storage, analysis, and visibility.
- Reliability features such as validation, retries, fallback values, and freshness checks strengthen the system.
