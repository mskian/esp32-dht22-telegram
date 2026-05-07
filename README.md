# ESP32 DHT22 Telegram Bot

A real-time IoT project using ESP32 + DHT22 to monitor temperature and humidity and send data via a Telegram bot.   

<img width="1536" height="1024" alt="A real-time IoT project using ESP32 + DHT22 to monitor temperature and humidity" src="https://github.com/user-attachments/assets/d930656c-433c-4930-8657-105cfcc0a18f" /><br />

<img width="500" height="580" alt="A real-time IoT project using ESP32 + DHT22 to monitor temperature and humidity" src="https://github.com/user-attachments/assets/43f74c9f-8efa-420f-a1ea-a029f3a83eac" /><br />

<img width="702" height="1280" alt="Telegram Bot Update" src="https://github.com/user-attachments/assets/ebff3560-e19b-4143-9255-ff76233c40a7" /><br>

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

#### Wiring:

**it requires 3 Jumper Wires** <br>

<img width="500" height="580" alt="Wiring" src="https://github.com/user-attachments/assets/625a3775-e1e4-4a98-b768-0719c31e2608" /><br />


```sh
DHT22(sensor)	 ESP32(module)

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

## 🔑 Configuration

**Choose Anyone of the Code below based on your usage:**  

- `esp32_dht22_telegram_bot.ino`: for faster response and consume more energy
- `esp32_dht22_optimized_bot.ino`: Optimized and deplay in bot response (recommended)
- `esp32_dht22_web_bot.ino`: Telegram Bot + web view with Real-time Live Data + Telegram Trigger Button (recommended)  

**Edit the code:**

```c++
const char* ssid = "YOUR_WIFI_NAME"; // it support 2G WIFI Only
const char* password = "YOUR_PASSWORD";
const char* BOT_TOKEN = "YOUR_BOT_TOKEN";
const char* ALLOWED_IDS[] = {
  "YOUR CHAT ID"
};
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
start - 👍 Start the bot
data - 🔌 Show system + sensor status
feel - 😛 Funny weather feeling
temperature - ☀️ Get temperature
humidity - 🔥 Get humidity
```

## ⚠️ Notes

- DHT22 updates every `~2` seconds (hardware limitation)
- Use 3.3V (not 5V)
- Avoid rapid polling

## Credits

- Room temperature and humidity with ESP8266, DHT11, and a Telegram bot - <https://github.com/mcnaveen/esp8266-dht11-telegram>
- Universal Telegram Bot Library - <https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot>
- ESP32 and DHT22: Learning Hardware Selection the Hard Way - <https://medium.com/@ummugulsun/esp32-and-dht22-learning-hardware-selection-the-hard-way-49d2589050c2>
- Youtube Demo Video: **<https://www.youtube.com/shorts/qvT4jK5xQgs>**  

## LICENSE

MIT License
