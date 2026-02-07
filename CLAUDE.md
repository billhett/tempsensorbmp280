# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32 Arduino sketch that reads temperature, humidity, and pressure from a BME280 sensor over I2C and publishes readings to an MQTT broker every 60 seconds. Single-file project: `tempsensor.ino`.

## Build

This is an Arduino `.ino` sketch targeting ESP32. Compile and upload via:
- **arduino-cli**:
  ```
  arduino-cli compile --fqbn esp32:esp32:esp32 /Users/bill/tempsensor
  arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/tty.usbserial-0001 /Users/bill/tempsensor
  ```
- **Arduino IDE**: Open `tempsensor.ino`, select an ESP32 board, compile and upload

Serial monitor baud rate: **115200**

### Required Libraries
- `WiFi.h` (ESP32 built-in)
- `PubSubClient` (MQTT)
- `Adafruit_Sensor`
- `Adafruit_BME280`
- `Wire.h` (ESP32 built-in, I2C)
- `Preferences.h` (ESP32 built-in, NVS flash storage)

## Architecture

```
BME280 (I2C: 0x76 or 0x77) → ESP32 → WiFi → MQTT Broker (192.168.1.22:1883)
```

**Data flow**: `setup()` loads the MQTT topic prefix from NVS flash (default `sensor`), connects WiFi/MQTT, then attempts BME280 init via `initSensor()`. `loop()` checks connections (reboots on failure) and every 60s either calls `reportSensorData()` or retries `initSensor()` based on the `sensorOK` flag. Sensor errors are published to `<prefix>/status` via MQTT rather than halting the device.

**MQTT topics** (prefixed `<prefix>/`, default `sensor/`):
- `<prefix>/temperature`, `<prefix>/pressure`, `<prefix>/humidity` — published sensor data (numeric strings, no units)
- `<prefix>/unit/set` — subscribed; accepts `celsius`/`c` or `fahrenheit`/`f` to change temp unit
- `<prefix>/status` — publishes connection status, unit change confirmations, and sensor init errors
- `<prefix>/config/prefix` — subscribed; publish a new prefix string to change the topic prefix at runtime (persisted to NVS flash, survives reboots)

**I2C pins**: SDA on GPIO 23, SCL on GPIO 22.

**LED on GPIO 2** signals state: rapid flash = WiFi connecting, slow flash = MQTT connecting, brief flash = publishing.

**Connection recovery**: WiFi failure → immediate reboot. MQTT failure → retry up to 10 times with 5s check interval, then reboot.

## Configuration

WiFi SSID/password, MQTT broker IP/port, and report interval are hardcoded at the top of the `.ino` file.

The **MQTT topic prefix** defaults to `sensor` and can be changed at runtime by publishing to `<prefix>/config/prefix`. The new prefix is persisted to ESP32 NVS flash via `Preferences.h` and survives reboots. The MQTT client ID is derived as `ESP32_BME280-<prefix>`.

Default temp unit is `FAHRENHEIT`.
