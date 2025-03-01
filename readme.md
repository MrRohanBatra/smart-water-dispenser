# Smart Water Dispenser with ESP32

## Overview
This project is a **Smart Water Dispenser** using an **ESP32** microcontroller. It supports:
- **Remote control via SinricPro** (Alexa & Google Assistant support)
- **Web-based control interface** (via ESP32 WebServer & SPIFFS)
- **Automatic dispensing using a touch sensor**
- **Over-the-Air (OTA) firmware updates**

## Features
- **Remote Control**: Control water dispensing via SinricPro and a web-based UI.
- **Manual Dispensing**: Press the touch sensor to dispense water.
- **Adjustable Water Flow**: Dispense a user-defined amount of water.
- **LED Indicator**: Lights up during dispensing.
- **WebServer API**: Supports HTTP requests for controlling the dispenser.
- **OTA Updates**: Update firmware remotely.

## Hardware Requirements
- ESP32 Dev Module
- Relay Module / Motor Driver
- Water Pump
- Flow Sensor (optional for precise measurement)
- LED Indicator
- Touch Sensor (GPIO 32)

## Pin Configuration
| Pin | Function |
|----|------------|
| 13 | Motor Enable (PWM) |
| 12 | Motor Control 1 |
| 14 | Motor Control 2 |
| 2  | LED Indicator |
| 32 | Touch Sensor |

## Software Requirements
- **PlatformIO** (Recommended for development)
- **Arduino Framework**
- **Libraries**:
  - `SinricPro`
  - `WiFi`
  - `WebServer`
  - `ESPmDNS`
  - `SPIFFS`
  - `SemVer`

## Installation & Setup
### 1. Clone Repository
```sh
git clone https://github.com/your-repo/smart-water-dispenser.git
cd smart-water-dispenser
```
### 2. Install Dependencies
Make sure you have **PlatformIO** installed, then run:
```sh
pio run
```
### 3. Upload Firmware
Connect your ESP32 via USB and run:
```sh
pio run --target upload
```
### 4. Configure WiFi & SinricPro
Modify `setup()` in `main.cpp` to set **WiFi SSID & Password**:
```cpp
WiFi.begin("YOUR_WIFI_SSID", "YOUR_WIFI_PASSWORD");
```
Also, replace **SinricPro credentials** with your API key and secret:
```cpp
SinricPro.begin("YOUR_APP_KEY", "YOUR_APP_SECRET");
```
### 5. Upload Web Files to SPIFFS
```sh
pio run --target uploadfs
```

## API Endpoints
| Endpoint | Method | Description |
|---------|--------|-------------|
| `/` | GET | Serves `index.html` (Web UI) |
| `/dispense?ml=200` | GET | Dispenses 200mL of water |
| `/state` | GET | Returns current device state |

## OTA Firmware Updates
The ESP32 can be updated **remotely** via SinricPro.
- The function `handleOTAUpdate()` checks for new firmware versions and downloads them.
- To trigger an OTA update, use SinricPro’s **update command**.

## License
This project is open-source under the **MIT License**.

## Credits
Developed by **Rohan Batra**.

