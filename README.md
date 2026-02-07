# ESP32 BME280 WiFi Temperature Sensor

An ESP32-based environmental sensor that reads temperature, humidity, and barometric pressure from a BME280 sensor and publishes the data to an MQTT broker over WiFi.

## Hardware

- ESP32 development board
- BME280 sensor (I2C)

### Wiring

| BME280 Pin | ESP32 Pin |
|------------|-----------|
| SDA        | GPIO 23   |
| SCL        | GPIO 22   |
| VCC        | 3.3V      |
| GND        | GND       |

The onboard LED on GPIO 2 is used for status indication.

## Setup

1. Install the following libraries in the Arduino IDE:
   - `PubSubClient`
   - `Adafruit Unified Sensor`
   - `Adafruit BME280 Library`

2. Edit the configuration at the top of `tempsensor.ino`:
   - `ssid` / `password` — WiFi credentials
   - `mqtt_server` / `mqtt_port` — MQTT broker address
   - `mqtt_client_id` — unique client ID for this device
   - MQTT topic prefix (default `lr/`) — change to match the sensor's location

3. Select your ESP32 board in Arduino IDE, compile, and upload.

## MQTT Topics

All topics use a location prefix (default `lr/`).

| Topic             | Direction  | Description                                      |
|-------------------|------------|--------------------------------------------------|
| `lr/temperature`  | Published  | Temperature reading (numeric, Fahrenheit default) |
| `lr/pressure`     | Published  | Barometric pressure in hPa                       |
| `lr/humidity`     | Published  | Relative humidity as percentage                  |
| `lr/unit/set`     | Subscribed | Set temp unit: `celsius`/`c` or `fahrenheit`/`f` |
| `lr/status`       | Published  | Device status, unit changes, sensor errors       |

Sensor data is published every 60 seconds.

## LED Status

| Pattern             | Meaning               |
|---------------------|-----------------------|
| Rapid flash (100ms) | Connecting to WiFi    |
| Slow flash (500ms)  | Connecting to MQTT    |
| Brief flash (50ms)  | Publishing sensor data|
| Off                 | Normal operation      |

## Connection Recovery

- **WiFi lost** — device reboots immediately
- **MQTT lost** — retries up to 10 times (5s interval), then reboots
- **BME280 not found** — publishes error to `lr/status` and retries every 60 seconds (device stays online for remote diagnostics)
