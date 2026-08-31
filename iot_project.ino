#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <DHT.h>
// 1. WI-FI & TELEGRAM CREDENTIALS
const char* ssid     = ".......";      // Enter your Wi-Fi name
const char* password = "........";  // Enter your Wi-Fi password

const String botToken = "............"; //Enter the Bot Tokens"
const String chatId   = ".............";//Enter the ID

// 2. HARDWARE PIN DEFINITIONS
#define PIN_TRIG        5
#define PIN_ECHO        18
#define PIN_LASER       19
#define PIN_LDR         34    // ADC1
#define PIN_FLAME       35    // ADC1
#define PIN_DHT         4
#define PIN_SHOCK       21
#define PIN_TILT        22
#define PIN_BUZZER      25

#define DHTTYPE DHT11
DHT dht(PIN_DHT, DHTTYPE);

WebServer server(80);

// 3. SYSTEM STATE MACHINE
enum SystemState {
  STATE_STANDBY,
  STATE_ARMED,
  STATE_WARNING,
  STATE_ALARM
};

SystemState currentState = STATE_STANDBY;
unsigned long warningStartTime = 0;
const unsigned long WARNING_TIMEOUT_MS = 10000;

float currentTemp = 0.0;
float currentHum = 0.0;
long currentDist = -1;
int currentLdr = 0;
int currentFlame = 4095; // Initialized high so it doesn't false-trigger on boot
bool laserArmed = false;

// 4. EMBEDDED WEB DASHBOARD (FLASH MEMORY)

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
<head>
  <title>Sentinel IoT Security Node</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    * { box-sizing: border-box; font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif; }
    body { background-color: #0f172a; color: #f8fafc; margin: 0; padding: 20px; text-align: center; }
    h2 { margin-bottom: 5px; color: #38bdf8; }
    p.subtitle { color: #94a3b8; font-size: 0.9rem; margin-top: 0; }
    .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(180px, 1fr)); gap: 15px; max-width: 800px; margin: 20px auto; }
    .card { background-color: #1e293b; padding: 20px; border-radius: 12px; border: 1px solid #334155; }
    .card h3 { font-size: 0.85rem; text-transform: uppercase; color: #94a3b8; margin: 0 0 10px 0; }
    .card .val { font-size: 1.8rem; font-weight: bold; color: #38bdf8; }
    .status-card { max-width: 800px; margin: 0 auto 20px auto; padding: 20px; border-radius: 12px; background-color: #1e293b; border: 2px solid #38bdf8; }
    .status-card h1 { margin: 0; font-size: 2rem; color: #38bdf8; }
    .btn-group { max-width: 800px; margin: 20px auto; display: flex; gap: 10px; }
    button { flex: 1; padding: 15px; font-size: 1rem; font-weight: bold; border: none; border-radius: 8px; cursor: pointer; transition: 0.2s; }
    .btn-arm { background-color: #10b981; color: white; }
    .btn-disarm { background-color: #ef4444; color: white; }
    button:active { transform: scale(0.98); }
  </style>
</head>
<body>
  <h2>SENTINEL SECURITY NODE</h2>
  <p class="subtitle">Direct ESP32 Real-Time Edge Dashboard</p>

  <div class="status-card" id="statusBox">
    <div style="font-size: 0.85rem; color: #94a3b8; text-transform: uppercase;">Current System State</div>
    <h1 id="stateVal">STANDBY</h1>
  </div>

  <div class="btn-group">
    <button class="btn-arm" onclick="sendAction('arm')">ARM SYSTEM</button>
    <button class="btn-disarm" onclick="sendAction('disarm')">DISARM / RESET</button>
  </div>

  <div class="grid">
    <div class="card"><h3>Proximity Range</h3><div class="val" id="distVal">-- cm</div></div>
    <div class="card"><h3>Room Temp</h3><div class="val" id="tempVal">-- &#176;C</div></div>
    <div class="card"><h3>Humidity</h3><div class="val" id="humVal">-- %</div></div>
    <div class="card"><h3>Laser Tripwire (LDR)</h3><div class="val" id="ldrVal">--</div></div>
    <div class="card"><h3>Flame Reading</h3><div class="val" id="flameVal">--</div></div>
  </div>

<script>
  function updateData() {
    fetch('/telemetry')
      .then(response => response.json())
      .then(data => {
        document.getElementById('stateVal').innerText = data.state;
        document.getElementById('distVal').innerText = data.dist + " cm";
        document.getElementById('tempVal').innerText = data.temp + " °C";
        document.getElementById('humVal').innerText = data.hum + " %";
        document.getElementById('ldrVal').innerText = data.ldr;
        document.getElementById('flameVal').innerText = data.flame;

        var box = document.getElementById('statusBox');
        var stateText = document.getElementById('stateVal');
        if (data.state === "ARMED") {
          box.style.borderColor = "#10b981"; stateText.style.color = "#10b981";
        } else if (data.state === "WARNING") {
          box.style.borderColor = "#f59e0b"; stateText.style.color = "#f59e0b";
        } else if (data.state === "ALARM") {
          box.style.borderColor = "#ef4444"; stateText.style.color = "#ef4444";
        } else {
          box.style.borderColor = "#38bdf8"; stateText.style.color = "#38bdf8";
        }
      })
      .catch(err => console.error(err));
  }

  function sendAction(action) {
    fetch('/action?type=' + action);
  }

  setInterval(updateData, 800);
</script>
</body>
</html>
)rawliteral";

// 5. HELPER & ALERT FUNCTIONS

// Custom URL encoder function to handle spaces, symbols, and linebreaks
String urlEncode(String str) {
  String encoded = "";
  char c;
  char code0;
  char code1;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (isalnum(c)) {
      encoded += c;
    } else if (c == ' ') {
      encoded += "%20";
    } else if (c == '\n') {
      encoded += "%0A";
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      encoded += '%';
      encoded += code0;
      encoded += code1;
    }
  }
  return encoded;
}

void sendTelegramAlert(String triggerReason) {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure(); // Skips certificate validation

    HTTPClient https;
    String message = " SENTINEL INTRUSION ALERT \n\n"
                     "Trigger: " + triggerReason + "\n"
                     "Status: ALARM ACTIVATED\n"
                     "Proximity: " + String(currentDist) + " cm";

    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatId + "&text=" + urlEncode(message);

    if (https.begin(client, url)) {
      int httpCode = https.GET();
      if (httpCode == 200) {
        Serial.printf("[TELEGRAM] Alert delivered successfully! (HTTP %d)\n", httpCode);
      } else {
        Serial.printf("[TELEGRAM] Request failed with HTTP Code: %d\n", httpCode);
      }
      https.end();
    }
  }
}

long readUltrasonicCM() {
  digitalWrite(PIN_TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(PIN_TRIG, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIG, LOW);
  long duration = pulseIn(PIN_ECHO, HIGH, 30000);
  if (duration == 0) return -1;
  return duration * 0.034 / 2;
}

const char* getStateString() {
  switch (currentState) {
    case STATE_STANDBY: return "STANDBY";
    case STATE_ARMED:   return "ARMED";
    case STATE_WARNING: return "WARNING";
    case STATE_ALARM:   return "ALARM";
    default:            return "UNKNOWN";
  }
}

void setSystemState(SystemState newState, String reason = "") {
  // Prevent repeating ALARM actions if already in ALARM state
  if (currentState == STATE_ALARM && newState == STATE_ALARM) return;

  currentState = newState;
  switch (currentState) {
    case STATE_STANDBY:
      digitalWrite(PIN_LASER, LOW);
      digitalWrite(PIN_BUZZER, LOW);
      laserArmed = false;
      Serial.println(F("[STATE] -> STANDBY"));
      break;

    case STATE_ARMED:
      digitalWrite(PIN_LASER, HIGH);
      digitalWrite(PIN_BUZZER, LOW);
      laserArmed = true;
      Serial.println(F("[STATE] -> ARMED"));
      break;

    case STATE_WARNING:
      warningStartTime = millis();
      Serial.println(F("[STATE] -> WARNING"));
      break;

    case STATE_ALARM:
      digitalWrite(PIN_BUZZER, HIGH);
      Serial.printf("[STATE] -> ALARM TRIGGERED (%s)\n", reason.c_str());
      sendTelegramAlert(reason);
      break;
  }
}

// 6. HTTP SERVER ROUTING HANDLERS

void handleRoot() {
  server.send_P(200, "text/html", index_html);
}

void handleTelemetry() {
  char jsonBuffer[200];
  snprintf(jsonBuffer, sizeof(jsonBuffer),
           "{\"state\":\"%s\",\"dist\":%ld,\"temp\":%.1f,\"hum\":%.1f,\"ldr\":%d,\"flame\":%d}",
           getStateString(), currentDist, currentTemp, currentHum, currentLdr, currentFlame);
  server.send(200, "application/json", jsonBuffer);
}

void handleAction() {
  if (server.hasArg("type")) {
    String action = server.arg("type");
    if (action == "arm") setSystemState(STATE_ARMED);
    if (action == "disarm") setSystemState(STATE_STANDBY);
  }
  server.send(200, "text/plain", "OK");
}

// 7. SETUP

void setup() {
  Serial.begin(115200);

  pinMode(PIN_TRIG, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(PIN_LASER, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  pinMode(PIN_SHOCK, INPUT_PULLUP);
  pinMode(PIN_TILT, INPUT_PULLUP);

  dht.begin();

  Serial.print(F("Connecting to WiFi..."));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
  }

  Serial.println(F("\nConnected!"));
  Serial.print(F("Open this IP in your browser: http://"));
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/telemetry", handleTelemetry);
  server.on("/action", handleAction);
  server.begin();

  setSystemState(STATE_STANDBY);
}

// 8. MAIN EXECUTION LOOP
unsigned long lastSensorRead = 0;

void loop() {
  server.handleClient();

  // Non-blocking sensor polling every 500ms
  if (millis() - lastSensorRead >= 500) {
    lastSensorRead = millis();

    currentDist = readUltrasonicCM();
    currentLdr = analogRead(PIN_LDR);
    currentFlame = analogRead(PIN_FLAME);

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t)) currentTemp = t;
    if (!isnan(h)) currentHum = h;
  }

  switch (currentState) {
    case STATE_STANDBY:
      // Only trip in Standby if a real flame threshold is crossed (and not 0)
      if (currentFlame > 0 && currentFlame < 800) {
        setSystemState(STATE_ALARM, "Flame Detected in Standby");
      }
      break;

    case STATE_ARMED: {
      bool shockTriggered = (digitalRead(PIN_SHOCK) == LOW);
      bool tiltTriggered = (digitalRead(PIN_TILT) == LOW);
      bool flameTriggered = (currentFlame > 0 && currentFlame < 800);
      bool tripwireBroken = (laserArmed && currentLdr < 1200);

      if (shockTriggered) {
        setSystemState(STATE_ALARM, "Vibration / Shock Tamper");
        break;
      }
      if (tiltTriggered) {
        setSystemState(STATE_ALARM, "Orientation / Tilt Tamper");
        break;
      }
      if (flameTriggered) {
        setSystemState(STATE_ALARM, "Active Flame Detected");
        break;
      }
      if (tripwireBroken) {
        setSystemState(STATE_ALARM, "Laser Tripwire Beam Cut");
        break;
      }

      if (currentDist > 0 && currentDist < 30) {
        setSystemState(STATE_WARNING);
      }
      break;
    }

    case STATE_WARNING: {
      static unsigned long lastBeep = 0;
      if (millis() - lastBeep >= 500) {
        lastBeep = millis();
        digitalWrite(PIN_BUZZER, !digitalRead(PIN_BUZZER));
      }
      if (millis() - warningStartTime >= WARNING_TIMEOUT_MS) {
        setSystemState(STATE_ALARM, "Proximity Grace Period Expired");
      }
      break;
    }

    case STATE_ALARM: {
      digitalWrite(PIN_BUZZER, HIGH);
      break;
    }
  }
}