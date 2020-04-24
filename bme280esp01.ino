// Author: Will Barksdale
// Date: April 24 2020
// File: bme280esp01.ino

// ESP-01 pin configuration
// SDA = GPI0
// SCL = GPI2

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <BME280I2C.h>

// Network and MQTT broker information
const char* ssid = "Network";
const char* password = "Password";
const char* mqtt_server = "MQTT Server IP";

// Initialize espCLient
WiFiClient espClient;
PubSubClient client(espClient);

// Reusable char array to pass MQTT payload
char msg[512]; 

// Create BME object
BME280I2C bme;

// Connects ESP-01 to network
void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - IP address: ");
  Serial.println(WiFi.localIP());
}

// Reconnects ESP to MQTT broker
void reconnect() {
  while (!client.connected()) {
    // Attempt to connect
    if (client.connect("ESP8266_Sensor_XX")) { 
      // Change espClient name for multiple ESP's on same network
      // Must match loop statement
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// Move read code to here once complete
void getMeasurement() {
  // Declare floats to assign measurements to
  float temp(NAN), hum(NAN), pres(NAN);

  // Define measurement units
  BME280::TempUnit tempUnit(BME280::TempUnit_Fahrenheit);
  BME280::PresUnit presUnit(BME280::PresUnit_Pa);

  // Take measurement readings
  bme.read(pres, temp, hum, tempUnit, presUnit);

  // Report temperature reading to MQTT broker
  dtostrf(temp + 1, 2, 0, msg); // Adjust hum offset for calibration
  client.publish("room/temperature", msg);

  // Report humidity reading to MQTT broker
  dtostrf(hum + 7, 2, 0, msg); // Adjust hum offset for calibration
  client.publish("room/humidity", msg);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Connecting to WiFi");
  setup_wifi();
  Serial.println("Connecting to MQTT broker");
  client.setServer(mqtt_server, 1883);

  // I2C setup for ESP8266-ESP01
  Serial.println("Setting up I2C");
  Wire.pins(0, 2);
  Wire.begin();

  // Loop until bme.begin succeeds
  if (!bme.begin())
  {
    Serial.println("Sensor not found...trying again");
    delay(1000);
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  if (!client.loop())
    client.connect("ESP8266_Sensor_XX"); // Change espClient name for multiple ESP's on same network

  getMeasurement();
  delay(2000); // Set measurement interval in milliseconds
}
