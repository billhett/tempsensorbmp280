#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Preferences.h>

// WiFi credentials
const char* ssid = "Blue";
const char* password = "tampa777";  // Update with your password

// MQTT Broker settings
const char* mqtt_server = "192.168.1.22";
const int mqtt_port = 1883;

// Dynamic MQTT prefix and topics (built from prefix)
Preferences prefs;
String prefix = "sensor";
String mqtt_client_id;
String topic_temperature;
String topic_pressure;
String topic_humidity;
String topic_unit_set;
String topic_status;
String topic_config_prefix;

void buildTopics() {
  mqtt_client_id = "ESP32_BME280-" + prefix;
  topic_temperature = prefix + "/temperature";
  topic_pressure = prefix + "/pressure";
  topic_humidity = prefix + "/humidity";
  topic_unit_set = prefix + "/unit/set";
  topic_status = prefix + "/status";
  topic_config_prefix = prefix + "/config/prefix";
}

// Timing
const unsigned long REPORT_INTERVAL = 60000;  // 1 minute in milliseconds
unsigned long lastReport = 0;

// LED Pin (usually GPIO 2 on ESP32-Dev boards)
const int LED_PIN = 2;

// Unit settings
enum TempUnit { CELSIUS, FAHRENHEIT };
TempUnit tempUnit = FAHRENHEIT;  // Default to Fahrenheit

// Objects
WiFiClient espClient;
PubSubClient mqttClient(espClient);
Adafruit_BME280 bme;
bool sensorOK = false;

// Connection tracking
unsigned long lastWiFiCheck = 0;
unsigned long lastMQTTCheck = 0;
const unsigned long CONNECTION_CHECK_INTERVAL = 5000;

void setup() {
  Serial.begin(115200);
  
  // Initialize LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // Load prefix from flash
  prefs.begin("config", false);
  prefix = prefs.getString("prefix", "sensor");
  buildTopics();
  Serial.print("MQTT prefix: ");
  Serial.println(prefix);

  // Connect to WiFi
  connectWiFi();

  // Setup MQTT
  mqttClient.setServer(mqtt_server, mqtt_port);
  mqttClient.setCallback(mqttCallback);

  // Connect to MQTT
  connectMQTT();

  // Initialize I2C with SDA on GPIO 23, SCL on GPIO 22
  Wire.begin(23, 22);

  // Initialize BME280
  initSensor();
}

void loop() {
  unsigned long currentMillis = millis();
  
  // Check WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost! Rebooting...");
    delay(1000);
    ESP.restart();
  }
  
  // Check MQTT connection
  if (!mqttClient.connected()) {
    if (currentMillis - lastMQTTCheck >= CONNECTION_CHECK_INTERVAL) {
      lastMQTTCheck = currentMillis;
      if (!connectMQTT()) {
        Serial.println("MQTT connection lost and reconnect failed! Rebooting...");
        delay(1000);
        ESP.restart();
      }
    }
  }
  
  mqttClient.loop();
  
  // Report sensor data or retry sensor init every minute
  if (currentMillis - lastReport >= REPORT_INTERVAL) {
    lastReport = currentMillis;
    if (sensorOK) {
      reportSensorData();
    } else {
      initSensor();
    }
  }
}

void initSensor() {
  if (bme.begin(0x76) || bme.begin(0x77)) {
    bme.setSampling(Adafruit_BME280::MODE_NORMAL,
                    Adafruit_BME280::SAMPLING_X1,  // temperature
                    Adafruit_BME280::SAMPLING_X1,  // pressure
                    Adafruit_BME280::SAMPLING_X1,  // humidity
                    Adafruit_BME280::FILTER_OFF);
    sensorOK = true;
    Serial.println("BME280 sensor initialized successfully");
    mqttClient.publish(topic_status.c_str(), "BME280 sensor initialized");
  } else {
    Serial.println("Could not find BME280 sensor!");
    mqttClient.publish(topic_status.c_str(), "ERROR: BME280 sensor not found");
  }
}

void connectWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  // Flash LED quickly while connecting
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    Serial.print(".");
    
    attempts++;
    if (attempts > 150) {  // 30 seconds timeout
      Serial.println("\nWiFi connection failed! Rebooting...");
      delay(1000);
      ESP.restart();
    }
  }
  
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  digitalWrite(LED_PIN, LOW);
}

bool connectMQTT() {
  Serial.print("Connecting to MQTT broker: ");
  Serial.println(mqtt_server);
  
  int attempts = 0;
  while (!mqttClient.connected()) {
    // Flash LED slowly while connecting to MQTT
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(500);
    
    Serial.print("Attempting MQTT connection...");
    
    if (mqttClient.connect(mqtt_client_id.c_str())) {
      Serial.println("connected");
      digitalWrite(LED_PIN, LOW);

      // Subscribe to unit setting and config prefix topics
      mqttClient.subscribe(topic_unit_set.c_str());
      mqttClient.subscribe(topic_config_prefix.c_str());

      // Publish status
      mqttClient.publish(topic_status.c_str(), "online");
      
      return true;
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println();
      
      attempts++;
      if (attempts > 10) {  // 10 attempts
        Serial.println("MQTT connection failed after 10 attempts");
        return false;
      }
    }
  }
  
  return true;
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
  
  String topicStr = String(topic);

  // Handle unit setting
  if (topicStr == topic_unit_set) {
    message.toLowerCase();
    if (message == "celsius" || message == "c") {
      tempUnit = CELSIUS;
      Serial.println("Temperature unit set to Celsius");
      mqttClient.publish(topic_status.c_str(), "Unit set to Celsius");
    } else if (message == "fahrenheit" || message == "f") {
      tempUnit = FAHRENHEIT;
      Serial.println("Temperature unit set to Fahrenheit");
      mqttClient.publish(topic_status.c_str(), "Unit set to Fahrenheit");
    }
  }

  // Handle prefix change
  if (topicStr == topic_config_prefix) {
    message.trim();
    if (message.length() > 0 && message != prefix) {
      Serial.print("Changing prefix from '");
      Serial.print(prefix);
      Serial.print("' to '");
      Serial.print(message);
      Serial.println("'");

      // Save new prefix to flash
      prefs.putString("prefix", message);

      // Confirm on old status topic before switching
      String confirmMsg = "Prefix changing to: " + message;
      mqttClient.publish(topic_status.c_str(), confirmMsg.c_str());
      mqttClient.loop();  // Flush outgoing message
      delay(100);

      // Disconnect, rebuild topics with new prefix, reconnect
      mqttClient.disconnect();
      prefix = message;
      buildTopics();
      connectMQTT();
    }
  }
}

void reportSensorData() {
  // Flash LED briefly while sending data
  digitalWrite(LED_PIN, HIGH);
  
  // Read sensor data
  float temperature = bme.readTemperature();
  float pressure = bme.readPressure() / 100.0F;  // Convert to hPa
  float humidity = bme.readHumidity();
  
  // Convert temperature if needed
  if (tempUnit == FAHRENHEIT) {
    temperature = temperature * 9.0 / 5.0 + 32.0;
  }
  
  // Create messages
  char tempStr[10];
  char pressStr[10];
  char humStr[10];
  
  dtostrf(temperature, 6, 2, tempStr);
  dtostrf(pressure, 7, 2, pressStr);
  dtostrf(humidity, 6, 2, humStr);
  
  // Publish data
  String tempMessage = String(tempStr);// + (tempUnit == CELSIUS ? " C" : " F");
  String pressMessage = String(pressStr);// + " hPa";
  String humMessage = String(humStr);// + " %";
  
  mqttClient.publish(topic_temperature.c_str(), tempMessage.c_str());
  mqttClient.publish(topic_pressure.c_str(), pressMessage.c_str());
  mqttClient.publish(topic_humidity.c_str(), humMessage.c_str());
  
  // Debug output
  Serial.println("--- Sensor Data ---");
  Serial.print("Temperature: ");
  Serial.println(tempMessage);
  Serial.print("Pressure: ");
  Serial.println(pressMessage);
  Serial.print("Humidity: ");
  Serial.println(humMessage);
  Serial.println("------------------");
  
  delay(50);  // Keep LED on for 50ms
  digitalWrite(LED_PIN, LOW);
}
