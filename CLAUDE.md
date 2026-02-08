# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ESP32 Arduino sketch that auto-detects a BME280 or BMP280 sensor over I2C, reads temperature and pressure (plus humidity if BME280), and publishes readings to an MQTT broker every 60 seconds. Single-file project: `tempsensorbmp280.ino`.

## Build

This is an Arduino `.ino` sketch targeting ESP32. Compile and upload via:
- **arduino-cli**:
  ```
  arduino-cli compile --fqbn esp32:esp32:esp32 /Users/bill/tempsensorbmp280
  arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/tty.usbserial-0001 /Users/bill/tempsensorbmp280
  ```
- **Arduino IDE**: Open `tempsensorbmp280.ino`, select an ESP32 board, compile and upload

Serial monitor baud rate: **115200**

### Required Libraries
- `WiFi.h` (ESP32 built-in)
- `PubSubClient` (MQTT)
- `Adafruit_Sensor`
- `Adafruit_BME280`
- `Adafruit_BMP280`
- `Wire.h` (ESP32 built-in, I2C)
- `Preferences.h` (ESP32 built-in, NVS flash storage)

## Architecture

```
BME280/BMP280 (I2C: 0x76 or 0x77) → ESP32 → WiFi → MQTT Broker (192.168.1.22:1883)
```

**Data flow**: `setup()` loads the MQTT topic prefix from NVS flash (default `sensor`), connects WiFi/MQTT, then attempts sensor init via `initSensor()` which tries BME280 first, then falls back to BMP280. `loop()` checks connections (reboots on failure) and every 60s either calls `reportSensorData()` or retries `initSensor()` based on the `sensorOK` flag. Sensor errors are published to `temps/<prefix>/status` via MQTT rather than halting the device.

**MQTT topics** (all under `temps/` top-level namespace, then `<prefix>/`, default `sensor/`):
- `temps/<prefix>/temperature`, `temps/<prefix>/pressure` — always published (numeric strings, no units)
- `temps/<prefix>/humidity` — published only when a BME280 is detected (BMP280 has no humidity sensor)
- `temps/<prefix>/unit/set` — subscribed; accepts `celsius`/`c` or `fahrenheit`/`f` to change temp unit
- `temps/<prefix>/status` — publishes connection status, unit change confirmations, and sensor init errors
- `temps/<prefix>/config/prefix` — subscribed; publish a new prefix string to change the topic prefix at runtime (persisted to NVS flash, survives reboots)

**I2C pins**: SDA on GPIO 23, SCL on GPIO 22.

**LED on GPIO 2** signals state: rapid flash = WiFi connecting, slow flash = MQTT connecting, brief flash = publishing.

**Connection recovery**: WiFi failure → immediate reboot. MQTT failure → retry up to 10 times with 5s check interval, then reboot.

## Configuration

WiFi SSID/password, MQTT broker IP/port are defined in `secrets.h` (gitignored). Copy `secrets.h.example` to `secrets.h` and fill in your values. Report interval is hardcoded at the top of the `.ino` file.

The **MQTT topic prefix** defaults to `sensor` and can be changed at runtime by publishing to `temps/<prefix>/config/prefix`. The new prefix is persisted to ESP32 NVS flash via `Preferences.h` and survives reboots. All topics live under the `temps/` top-level namespace (e.g. `temps/sensor/temperature`). The MQTT client ID is derived as `ESP32_BME280-<prefix>`.

Default temp unit is `FAHRENHEIT`.
