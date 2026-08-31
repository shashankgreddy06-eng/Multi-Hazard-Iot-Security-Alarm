# Sentinel: Multi-Hazard IoT Security & Intrusion Alert Node

An ESP32-powered multi-hazard security node that combines perimeter defense, optical tripwire, fire detection, ultrasonic proximity monitoring, and anti-tamper sensing into a unified Finite State Machine (FSM). The system features an embedded real-time web dashboard for telemetry and delivers instant, TLS-encrypted push notifications directly to Telegram.

---

<img width="767" height="366" alt="Screenshot 2026-08-31 222508" src="https://github.com/user-attachments/assets/4830e983-7bf1-4e60-8ed3-c0b0544864a0" />
<img width="296" height="641" alt="WhatsApp Image 2026-08-31 at 10 27 53 PM (1)" src="https://github.com/user-attachments/assets/0185315a-525f-4d3e-879a-ae727ece600e" />
<img width="468" height="351" alt="WhatsApp Image 2026-08-31 at 10 27 53 PM" src="https://github.com/user-attachments/assets/078c42c9-aa83-4ca0-b9f6-f0ca5d71ca21" />


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

1. **STANDBY:** Optical defenses are disarmed. Flame detection remains continuously active in the background.
2. **ARMED:** Laser is energized; all tamper and perimeter sensors are actively evaluated.
3. **WARNING:** Proximity breach inside 30 cm starts a 10-second pulsing warning buzzer.
4. **ALARM:** Siren sounds continuously and an encrypted HTTPS alert is pushed to Telegram with the breach reason.

---

## ⚡ Getting Started

### 1. Prerequisites & Libraries
Install the required libraries in the Arduino IDE (**Tools $\rightarrow$ Manage Libraries**):
* `DHT sensor library` by Adafruit
* `Adafruit Unified Sensor`

### 2. Configure Credentials
Update the configuration section in the `.ino` sketch:
```cpp
const char* ssid     = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

const String botToken = "YOUR_TELEGRAM_BOT_TOKEN";
const String chatId   = "YOUR_TELEGRAM_CHAT_ID";

