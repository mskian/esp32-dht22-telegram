#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <esp_wifi.h>

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

// ===== TIMERS =====
unsigned long lastBotCheck = 0;
unsigned long lastSensorRead = 0;
unsigned long lastReconnect = 0;

// ⚡ Optimized intervals
const unsigned long BOT_INTERVAL = 1500;        // fast response
const unsigned long SENSOR_INTERVAL = 10000;    // stable + low load
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

  // 🔋 Power optimization
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
    msg += "⏱ Uptime: " + String(millis()/1000) + " sec";

    bot.sendMessage(chat_id, msg, "Markdown");
  }

  else if (text == "/feel") {
    bot.sendMessage(chat_id, getFeel(avgTemp, avgHum), "");
  }
}

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  dht.begin();

  client.setInsecure();
  connectWiFi();
}

// ===== LOOP =====
void loop() {

  // 🔁 WiFi reconnect (controlled)
  if (WiFi.status() != WL_CONNECTED &&
      millis() - lastReconnect > WIFI_RECONNECT_INTERVAL) {
    lastReconnect = millis();
    connectWiFi();
  }

  updateSensor();

  // ⚡ FAST + SAFE polling
  if (millis() - lastBotCheck > BOT_INTERVAL) {
    lastBotCheck = millis();

    int newMsg = bot.getUpdates(bot.last_message_received + 1);

    if (newMsg > 0) {
      int limit = min(newMsg, 3);  // 🚀 limit load

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

  delay(80);
}
