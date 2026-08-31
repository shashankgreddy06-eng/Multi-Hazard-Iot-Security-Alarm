# Sentinel: Multi-Hazard IoT Security & Intrusion Alert Node

An ESP32-powered multi-hazard security node that combines perimeter defense, optical tripwire, fire detection, ultrasonic proximity monitoring, and anti-tamper sensing into a unified Finite State Machine (FSM). The system features an embedded real-time web dashboard for telemetry and delivers instant, TLS-encrypted push notifications directly to Telegram.

---

## 📸 Project Showcase

| Hardware Prototype | Web Telemetry Dashboard | Mobile Telegram Alerts |
| :---: | :---: | :---: |
| ![Hardware Setup](<img width="468" height="351" alt="WhatsApp Image 2026-08-31 at 10 27 53 PM" src="https://github.com/user-attachments/assets/01d3f0a7-6ec7-4dec-9bc3-9f78ee16b495" />
) | ![Web Dashboard](<img width="500" height="366" alt="Screenshot 2026-08-31 222508" src="https://github.com/user-attachments/assets/2118f6fb-b361-4f4a-b654-2f2434d6fdf9" />
) | ![Telegram Alert](<img width="296" height="641" alt="WhatsApp Image 2026-08-31 at 10 27 53 PM (1)" src="https://github.com/user-attachments/assets/cd50d100-7be0-4b6a-9ed8-78266b095153" />
) |
| *ESP32 Multi-Sensor Breadboard Node* | *Real-Time Asynchronous Web UI* | *Sub-2s Encrypted Breach Alerts* |

---

##  Key Features

* **Multi-Sensor Security Fusion:** Evaluates threats across optical, thermal, ultrasonic, and mechanical tamper vectors.
* **4-Stage Finite State Machine (FSM):** Deterministic state flow (`STANDBY`, `ARMED`, `WARNING`, `ALARM`) preventing false alarms while ensuring 24/7 background flame monitoring.
* **Embedded Real-Time Dashboard:** Hosted directly on the ESP32's SPI Flash memory; updates live telemetry every 800 ms via asynchronous JavaScript `fetch()` calls.
* **Instant Cloud Push Notifications:** Uses `WiFiClientSecure` to dispatch encrypted HTTPS alert messages to the Telegram Bot API within 1–2 seconds of a breach.
* **Non-Blocking Architecture:** Built entirely using `millis()` timing loops to maintain uninterrupted Wi-Fi packet processing and web server responsiveness.

---

##  System Architecture & Pin Mapping

| Sensor / Actuator | Module Type | ESP32 GPIO Pin | Function |
| :--- | :--- | :--- | :--- |
| **Microcontroller** | ESP32 Dev Module | — | Central Edge Controller & Web Server |
| **Laser Emitter** | 5V 650nm Laser Diode | `GPIO 19` | Optical tripwire beam emitter (active only when Armed) |
| **Optical Receiver** | Light Dependent Resistor (LDR) | `GPIO 34` (ADC1) | Tripwire threshold monitor |
| **Proximity Range** | HC-SR04 Ultrasonic | `Trig: 5` / `Echo: 18` | 30 cm perimeter buffer & warning zone |
| **Flame / Thermal** | IR Flame Sensor | `GPIO 35` (ADC1) | 760–1100 nm IR radiation detection |
| **Shock / Impact** | Vibration Sensor | `GPIO 21` (Pullup) | Mechanical tamper & impact detection |
| **Orientation** | Tilt / Ball Switch | `GPIO 22` (Pullup) | Anti-removal & enclosure displacement |
| **Climate** | DHT11 Sensor | `GPIO 4` | Ambient temperature & relative humidity |
| **Acoustic Deterrent** | Active Buzzer | `GPIO 25` | Pulsed warning tones & continuous alarm siren |

---

##  Finite State Machine (FSM) Workflow

