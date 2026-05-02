#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <DHT.h>

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

const unsigned long BOT_INTERVAL = 1200;
const unsigned long SENSOR_INTERVAL = 2200;
const unsigned long WIFI_RECONNECT_INTERVAL = 10000;

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

// ===== WIFI =====
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

  idx++;
  if (idx >= SAMPLE_SIZE) {
    idx = 0;
    filled = true;
  }

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
  if (t > 30) return "😎 Warm vibes, stay hydrated";
  if (t > 25) return "🙂 Chill weather, enjoy";
  if (t < 20) return "🥶 Cold Blanket time";

  if (h > 80) return "💦 Feels like swimming in air";
  if (h < 30) return "🌵 Dry air Lip balm needed";

  return "👌 Perfect weather!";
}

// ===== COMMAND HANDLER =====
void handleCommand(String chat_id, String text, String name) {

  // 🔐 AUTH
  if (!isAllowed(chat_id)) {
    if (chat_id != lastBlockedID) {
      bot.sendMessage(chat_id, "⛔ Access Denied", "");
      lastBlockedID = chat_id;
    }
    return;
  }

  if (text != "/start" &&
      text != "/temperature" &&
      text != "/humidity" &&
      text != "/data" &&
      text != "/feel") {
    return;
  }

  if (isnan(avgTemp)) {
    bot.sendMessage(chat_id, "⏳ Sensor warming up...", "");
    return;
  }

  if (text == "/start") {
    String msg = "👋 Hello " + name + "\n\n";
    msg += "🤖 Smart ESP32 and DHT22 Bot For Temperature and Humidity.\n\n";
    msg += "/data 📊\n";
    msg += "/feel 😄\n";
    msg += "/temperature 🌡\n";
    msg += "/humidity 💧";

    bot.sendMessage(chat_id, msg, "");
  }

  else if (text == "/temperature") {
    bot.sendMessage(chat_id,
      "🌡 " + String(avgTemp,2) + " °C", "");
  }

  else if (text == "/humidity") {
    bot.sendMessage(chat_id,
      "💧 " + String(avgHum,2) + " %", "");
  }

  else if (text == "/data") {
    String msg = "📊 *System Status*\n\n";
    msg += "🌡 Temp: *" + String(avgTemp,2) + " °C*\n";
    msg += "💧 Humidity: *" + String(avgHum,2) + " %*\n\n";
    msg += "📡 WiFi: ";
    msg += (WiFi.status() == WL_CONNECTED) ? "✅ Connected\n" : "❌ Disconnected\n";

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
  WiFi.setSleep(false);
}

// ===== LOOP =====
void loop() {

  // WiFi reconnect (controlled)
  if (WiFi.status() != WL_CONNECTED &&
      millis() - lastReconnect > WIFI_RECONNECT_INTERVAL) {
    lastReconnect = millis();
    connectWiFi();
  }

  updateSensor();

  if (millis() - lastBotCheck > BOT_INTERVAL) {
    lastBotCheck = millis();

    int newMsg = bot.getUpdates(bot.last_message_received + 1);

    while (newMsg) {
      for (int i = 0; i < newMsg; i++) {

        String text = bot.messages[i].text;
        if (text.length() == 0) continue;

        handleCommand(
          String(bot.messages[i].chat_id),
          text,
          bot.messages[i].from_name
        );
      }
      newMsg = bot.getUpdates(bot.last_message_received + 1);
    }
  }
}
