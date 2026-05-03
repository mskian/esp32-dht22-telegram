#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <esp_wifi.h>

// ✅ Built-in server
#include <WebServer.h>

// ===== CONFIG =====
#define DHTPIN 4
#define DHTTYPE DHT22

const char* ssid = "YOUR WIFI NAME";
const char* password = "YOUR WIFI PASSWORD";
const char* BOT_TOKEN = "YOUR TELEGRAM BOT TOKEN";

// ===== ALLOWED USERS =====
const char* ALLOWED_IDS[] = {
  "YOUR CHAT ID"
};
const int ALLOWED_COUNT = sizeof(ALLOWED_IDS)/sizeof(ALLOWED_IDS[0]);

// ===== OBJECTS =====
DHT dht(DHTPIN, DHTTYPE);
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);
WebServer server(80);

// ===== TIMERS =====
unsigned long lastBotCheck = 0;
unsigned long lastSensorRead = 0;
unsigned long lastReconnect = 0;

// 🔥 Slightly optimized
const unsigned long BOT_INTERVAL = 2500;
const unsigned long SENSOR_INTERVAL = 10000;
const unsigned long WIFI_RECONNECT_INTERVAL = 20000;

// ===== SENSOR DATA =====
#define SAMPLE_SIZE 5
float tBuf[SAMPLE_SIZE], hBuf[SAMPLE_SIZE];
int idx = 0;
bool filled = false;

float avgTemp = NAN;
float avgHum = NAN;

int sensorFailCount = 0;

// ===== SECURITY =====
String lastBlockedID = "";

// ===== AUTH =====
bool isAllowed(String id) {
  for (int i = 0; i < ALLOWED_COUNT; i++) {
    if (id == ALLOWED_IDS[i]) return true;
  }
  return false;
}

// ===== WIFI CONNECT =====
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("📡 Connecting");
  unsigned long start = millis();

  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > 20000) {
      Serial.println("\n❌ WiFi timeout");
      return;
    }
    delay(300);
    Serial.print(".");
  }

  Serial.println("\n✅ Connected");
  Serial.print("🌐 IP: ");
  Serial.println(WiFi.localIP());

  WiFi.setSleep(true);
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
  esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
}

// ===== SENSOR =====
void updateSensor() {
  if (millis() - lastSensorRead < SENSOR_INTERVAL) return;
  lastSensorRead = millis();

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(t) || isnan(h)) {
    sensorFailCount++;
    if (sensorFailCount > 5) {
      Serial.println("⚠️ Sensor unstable");
    }
    return;
  }

  sensorFailCount = 0;

  tBuf[idx] = t;
  hBuf[idx] = h;

  idx = (idx + 1) % SAMPLE_SIZE;
  if (idx == 0) filled = true;

  int count = filled ? SAMPLE_SIZE : idx;

  float tSum = 0, hSum = 0;
  for (int i = 0; i < count; i++) {
    tSum += tBuf[i];
    hSum += hBuf[i];
  }

  avgTemp = tSum / count;
  avgHum = hSum / count;
}

// ===== FEEL =====
String getFeel(float t, float h) {
  if (t > 35) return "🔥 Oi it's boiling AC where? 😵";
  if (t > 30) return "😎 Warm vibes stay hydrated";
  if (t > 25) return "🙂 Chill weather enjoy";
  if (t < 20) return "🥶 Cold Blanket time";

  if (h > 80) return "💦 Feels like swimming in air";
  if (h < 30) return "🌵 Dry air Lip balm needed";

  return "👌 Perfect weather";
}

// ===== HUMAN READBLE UPTIME =====
String formatUptime(unsigned long seconds) {
  unsigned long days = seconds / 86400;
  seconds %= 86400;

  unsigned long hours = seconds / 3600;
  seconds %= 3600;

  unsigned long minutes = seconds / 60;
  seconds %= 60;

  String result = "";

  if (days > 0) result += String(days) + "d ";
  if (hours > 0 || days > 0) result += String(hours) + "h ";
  if (minutes > 0 || hours > 0 || days > 0) result += String(minutes) + "m ";

  result += String(seconds) + "s";

  return result;
}

// ===== COMMAND =====
void handleCommand(String chat_id, String text, String name) {

  if (!isAllowed(chat_id)) {
    if (chat_id != lastBlockedID) {
      bot.sendMessage(chat_id, "⛔ Access Denied", "");
      lastBlockedID = chat_id;
    }
    return;
  }

  if (isnan(avgTemp)) {
    bot.sendMessage(chat_id, "⏳ Sensor warming up...", "");
    return;
  }

  if (text == "/start") {
    String msg = "👋 Hello " + name + "\n\n";
    msg += "ESP32 + DHT22 to Monitor Real-time Room 🌡 Temperature and Humidity 💧\n\n";
    msg += "/data 📊\n/feel 😄\n/temperature 🌡\n/humidity 💧";
    bot.sendMessage(chat_id, msg, "");
  }

  else if (text == "/temperature") {
    bot.sendMessage(chat_id, "🌡 " + String(avgTemp, 2) + " °C", "");
  }

  else if (text == "/humidity") {
    bot.sendMessage(chat_id, "💧 " + String(avgHum, 2) + " %", "");
  }

  else if (text == "/data") {
    String msg = "📊 *System Status*\n\n";
    msg += "🌡 Temp: *" + String(avgTemp, 2) + " °C*\n";
    msg += "💧 Humidity: *" + String(avgHum, 2) + " %*\n\n";
    msg += "📶 Signal: " + String(WiFi.RSSI()) + " dBm\n";
    msg += "⏱ Uptime: " + formatUptime(millis()/1000);
    bot.sendMessage(chat_id, msg, "Markdown");
  } else if (text == "/feel") {
    bot.sendMessage(chat_id, getFeel(avgTemp, avgHum), "");
  }
}

// ===== JSON =====
String getJSONData() {
  StaticJsonDocument<256> doc;

  if (isnan(avgTemp)) {
    doc["status"] = "warming";
    doc["temp"] = 0;
    doc["hum"] = 0;
  } else {
    doc["status"] = "ok";
    doc["temp"] = avgTemp;
    doc["hum"] = avgHum;
  }

  doc["wifi"] = WiFi.RSSI();
  doc["uptime"] = formatUptime(millis()/1000);

  String json;
  serializeJson(doc, json);
  return json;
}

// ===== TELEGRAM TRIGGER =====
void triggerTelegramData() {
  if (isnan(avgTemp)) return;

  String msg;
  msg.reserve(150);

  msg = "📊 *Live Status*\n\n";
  msg += "🌡 Temp: " + String(avgTemp, 2) + "°C\n";
  msg += "💧 Humidity: " + String(avgHum, 2) + "%\n";
  msg += "📶 WiFi: " + String(WiFi.RSSI()) + " dBm\n";
  msg += "⏱ Uptime: " + formatUptime(millis()/1000) + "\n\n";
  msg += "Feel: " + getFeel(avgTemp, avgHum);

  for (int i = 0; i < ALLOWED_COUNT; i++) {
    bot.sendMessage(ALLOWED_IDS[i], msg, "Markdown");
  }
}

// ===== WEB UI =====
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta name="theme-color" content="#0f0f0f">
<link href="https://fonts.googleapis.com/css2?family=Geist+Mono:wght@400;500;600;700&display=swap" rel="stylesheet">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<title>🌡Live Monitor Dashboard</title>
<style>
body {
  margin:0;
  font-family:'Geist Mono', monospace;
  background:#0f0f0f;
  color:#fff;
  text-align:center;
}
.header {
  background:#d32f2f;
  padding:18px;
  font-size:22px;
  font-weight:700;
  font-family:'Geist Mono', monospace;
}
.card {
  margin:16px;
  padding:20px;
  background:#1c1c1c;
  border-radius:16px;
  font-family:'Geist Mono', monospace;
}
.big {
  font-size:38px;
  font-weight:700;
}
.status {
  font-size:14px;
  font-family:'Geist Mono', monospace;
  color:#aaa;
}
.btn {
  width:100%;
  padding:14px;
  margin-top:10px;
  border:none;
  border-radius:10px;
  background:#d32f2f;
  color:#fff;
  font-family:'Geist Mono', monospace;
  font-size:16px;
  font-weight:600;
}
.row {
  display:flex;
  justify-content:space-around;
  font-family:'Geist Mono', monospace;
  font-size:16px;
}
</style>
</head>
<body>
<div class="header">🌡Live Monitor</div>
<div class="card">
  <div id="temp" class="big">-- °C</div>
  <div id="hum" class="big">-- %</div>
  <br>
  <div id="status" class="status">Press Refresh</div>
</div>
<div class="card">
  <button class="btn" onclick="loadData()">🔄 Refresh</button>
  <br><br>
  <div id="tg" class="status">Trigger Bot</div>
  <button class="btn" onclick="sendTelegram()">📤 Send Telegram</button>
</div>
<div class="card row">
  <div>📶 <span id="wifi">--</span> dBm</div>
  <div>⏱️ <span id="uptime">--</span></div>
</div>
<script>
async function loadData() {
  document.getElementById("status").innerText = "Loading...";
  try {
    let res = await fetch('/api/data');
    let data = await res.json();
    if (data.status === "warming") {
      document.getElementById("status").innerText = "Sensor warming Please Wait 5 Seconds and Click Refresh Again.";
      return;
    }
    document.getElementById("temp").innerText = data.temp.toFixed(2) + " °C";
    document.getElementById("hum").innerText = data.hum.toFixed(2) + " %";
    document.getElementById("wifi").innerText = data.wifi;
    document.getElementById("uptime").innerText = data.uptime;
    document.getElementById("status").innerText = "✓ Updated";
  } catch (e) {
    document.getElementById("status").innerText = "Error loading";
  }
}
async function sendTelegram() {
  const el = document.getElementById("tg");
  if (!el) return;
  el.innerText = "Triggering Telegram Bot...";
  try {
    const res = await fetch('/api/trigger');
    if (!res.ok) throw new Error("Request failed");
      setTimeout(() => {
        el.innerText = "Triggered Done ✓";
        setTimeout(() => {
          el.innerText = "Trigger Bot";
        }, 1500);
      }, 800);
  } catch (e) {
    console.error(e);
    el.innerText = "⚠ Error";
  }
}
</script>
</body>
</html>
)rawliteral";

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  dht.begin();

  client.setInsecure();
  connectWiFi();

  server.on("/", [](){ server.send_P(200,"text/html",index_html); });
  server.on("/api/data", [](){ server.send(200,"application/json",getJSONData()); });
  server.on("/api/trigger", [](){ triggerTelegramData(); server.send(200,"text/plain","OK"); });

  server.begin();
}

// ===== LOOP =====
void loop() {

  if (WiFi.status() != WL_CONNECTED &&
      millis() - lastReconnect > WIFI_RECONNECT_INTERVAL) {
    lastReconnect = millis();
    connectWiFi();
  }

  updateSensor();

  if (millis() - lastBotCheck > BOT_INTERVAL) {
    lastBotCheck = millis();

    int newMsg = bot.getUpdates(bot.last_message_received + 1);

    if (newMsg > 0) {
      int limit = min(newMsg, 3);
      for (int i = 0; i < limit; i++) {
        String text = bot.messages[i].text;
        if (text.length() == 0) continue;

        handleCommand(
          String(bot.messages[i].chat_id),
          text,
          bot.messages[i].from_name
        );
      }
    }
  }

  server.handleClient();

  delay(80);
}
