# ESP32 BME280/BMP280 MQTT Sensor

An ESP32-based environmental sensor that auto-detects a BME280 or BMP280 over I2C and publishes temperature, pressure, and humidity (BME280 only) to an MQTT broker every 60 seconds.

## Hardware

- ESP32 development board
- BME280 or BMP280 sensor (I2C)

### Wiring

| Sensor Pin | ESP32 Pin |
|------------|-----------|
| SDA        | GPIO 23   |
| SCL        | GPIO 22   |
| VCC        | 3.3V      |
| GND        | GND       |

The onboard LED on GPIO 2 is used for status indication.

## Setup

1. Install the following libraries in the Arduino IDE or via `arduino-cli lib install`:
   - `PubSubClient`
   - `Adafruit Unified Sensor`
   - `Adafruit BME280 Library`
   - `Adafruit BMP280 Library`

2. Edit the configuration at the top of `tempsensorbmp280.ino`:
   ```cpp
   const char* ssid = "YourSSID";
   const char* password = "YourPassword";
   const char* mqtt_server = "192.168.1.22";
   const int mqtt_port = 1883;
   ```

3. Compile and upload:
   ```bash
   arduino-cli compile --fqbn esp32:esp32:esp32 .
   arduino-cli upload --fqbn esp32:esp32:esp32 --port /dev/tty.usbserial-0001 .
   ```
   Or open `tempsensorbmp280.ino` in Arduino IDE, select an ESP32 board, and upload.

Serial monitor baud rate: **115200**

## Sensor Auto-Detection

On startup the sketch tries BME280 first (I2C address 0x76, then 0x77). If neither responds, it falls back to BMP280 at the same addresses. The detected sensor type is reported via serial and the MQTT status topic.

| Sensor | Temperature | Pressure | Humidity |
|--------|-------------|----------|----------|
| BME280 | Yes         | Yes      | Yes      |
| BMP280 | Yes         | Yes      | No       |

## MQTT Topics

All topics live under a `temps/` top-level namespace, followed by a configurable device prefix (default `sensor/`). Subscribe to `temps/#` to receive data from all devices.

| Topic                          | Direction  | Description                                      |
|--------------------------------|------------|--------------------------------------------------|
| `temps/sensor/temperature`     | Published  | Temperature reading (numeric string)             |
| `temps/sensor/pressure`        | Published  | Barometric pressure in hPa (numeric string)      |
| `temps/sensor/humidity`        | Published  | Relative humidity % (BME280 only)                |
| `temps/sensor/status`          | Published  | Device status, sensor info, errors               |
| `temps/sensor/unit/set`        | Subscribed | Set temp unit: `celsius`/`c` or `fahrenheit`/`f` |
| `temps/sensor/config/prefix`   | Subscribed | Change topic prefix at runtime (persisted to flash) |

Default temperature unit is Fahrenheit.

## LED Status

| Pattern             | Meaning                |
|---------------------|------------------------|
| Rapid flash (100ms) | Connecting to WiFi     |
| Slow flash (500ms)  | Connecting to MQTT     |
| Brief flash (50ms)  | Publishing sensor data |
| Off                 | Normal operation       |

## Connection Recovery

- **WiFi lost** -- device reboots immediately
- **MQTT lost** -- retries up to 10 times (5s interval), then reboots
- **Sensor not found** -- publishes error to status topic and retries every 60 seconds (device stays online for remote diagnostics)

## License

MIT
