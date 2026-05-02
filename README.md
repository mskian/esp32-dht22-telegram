# ESP32 DHT22 Telegram Bot

A real-time IoT project using ESP32 + DHT22 to monitor temperature and humidity and send data via a Telegram bot.  

## 🔥 Features

- 🌡 Temperature & 💧 Humidity monitoring
- 🤖 Telegram bot commands
- 📊 Live system status `(/data)`
- 😄 Smart “feel” messages `(/feel)`
- 🔐 Secure (only allowed users)
- ⚡ Async & non-blocking design
- 🧠 Moving average filtering for stable readings

## 📸 Hardware Setup

ESP32 Dev Board + DHT22 Sensor

**Wiring**

```sh
DHT22	 ESP32

+       3V3  
-       GND  
OUT      D4  
```

## ⚙️ Setup

1. Install Arduino IDE
2. Install ESP32 board support
3. Install ESP32 Board in Arduino IDE
4. Open Arduino IDE

```sh
Go to:
File → Preferences

Add this URL in Additional Board Manager URLs:
https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

Go to:
Tools → Board → Boards Manager
Search: esp32
Install: esp32 by Espressif
```

5. Install Libraries (Go to Library Manager)

```sh
DHT sensor library (Adafruit)
UniversalTelegramBot
ArduinoJson
```

6 - Select Your Board

```sh
Tools → Board → ESP32 Dev Module
Tools → Port → /dev/ttyUSB0  (or similar)
```

## 🤖 Telegram Bot Setup

- Open Telegram → @BotFather
- Run:

`/newbot`

- Copy your BOT TOKEN

#3 🔑 Configuration

**Edit the code:**

```c++
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
const char* BOT_TOKEN = "YOUR_BOT_TOKEN";
```

## 🚀 Upload

- Select ESP32 Dev Module

```sh
Tools → Board → ESP32 Dev Module
Select Port : /dev/ttyUSB0
```

- Upload via Arduino IDE - Click Upload  

## 📱 Telegram Bot Commands

- Command	Description

```sh
start -	Start bot
data - Full system info
feel - Funny weather response
temperature - Get temperature
humidity - Get humidity
```

## ⚠️ Notes

- DHT22 updates every `~2` seconds (hardware limitation)
- Use 3.3V (not 5V)
- Avoid rapid polling

## Credits

- Room temperature and humidity with ESP8266, DHT11, and a Telegram bot - <https://github.com/mcnaveen/esp8266-dht11-telegram>
- Universal Telegram Bot Library - <https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot>  

## LICENSE

MIT License
