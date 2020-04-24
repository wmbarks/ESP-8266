#include <stdio.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <BME280.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// WiFi configuration
const char* ssid     = "WIFI_SSID";
const char* password = "WIFIT_PASSWORD";

// IFTTT Maker channel setting
const char* host = "maker.ifttt.com";
const char* url = "/trigger/EVENT_NAME/with/key/REPLACE_THIS_WITH_KEY";

uint32_t timeoutCounterMsec = 0;
const uint32_t TIMEOUT_COUNTER_MSEC  = 10e3;
const uint32_t TIMEOUT_DURATION_MSEC = 1000;

// Global variables
bool wifiConnected = false;
bool iftttTriggerSucceeded = false;
char strBuffer[128];

// Global instances
BME280 bme280;
Adafruit_SSD1306 display;
WiFiClient client;

struct BME280Data {
  // Temperature degC
  double t;
  // Pressure hPa
  double p;
  // Humidity %
  double h;
};
BME280Data bme280Data;

void setup() {
  Serial.begin(115200);
  delay(10);

  initializeI2CDevices();
  clearDisplay();
  connectToWiFi();
  readBME280Data();
  if (wifiConnected) {
    triggerIFTTT();
  }
  displayBME280Data();
}

void initializeI2CDevices() {
  // Initialize I2C pins
  // SDA = GPIO0
  // SCL = GPIO2
  Wire.begin(0, 2);
  // Initialize BME280
  bme280.setMode(BME280_MODE_NORMAL, BME280_TSB_1000MS, BME280_OSRS_x1, BME280_OSRS_x1, BME280_OSRS_x1, BME280_FILTER_OFF);
  // Initialize OLED Display
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
}

void clearDisplay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);

  display.println("EPS01 + OLED + BME280");
  display.println("");
  display.display();
}

void readBME280Data() {
  bme280.getData(
    &bme280Data.t,
    &bme280Data.h,
    &bme280Data.p);
}

// Provide sufficiently large buffer
void doubleToStr(double value, char* buffer) {
  sprintf(buffer, "%d.%1d", (uint32_t)value, (uint32_t)(value * 10) % 10);
}

void displayBME280Data() {
  clearDisplay();
  doubleToStr(bme280Data.t, strBuffer);
    display.printf("WiFi : %s\n", (wifiConnected? "Connected":"Disconnected"));
    display.printf("IFTTT: %s\n", (iftttTriggerSucceeded? "Triggered":"Not triggered"));
  
  display.printf("T = %s degC\n", strBuffer);
  doubleToStr(bme280Data.p, strBuffer);
  display.printf("P = %s hPa\n", strBuffer);
  doubleToStr(bme280Data.h, strBuffer);
  display.printf("H = %s %%\n", strBuffer);
  display.display();
}

void connectToWiFi() {
  display.println("Connecting to WiFi");
  display.print("SSID: ");
  display.println(ssid);
  display.display();

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(TIMEOUT_DURATION_MSEC);
    display.print(".");
    display.display();
    if (timeoutCounterMsec >= TIMEOUT_COUNTER_MSEC) {
      timeoutCounterMsec += TIMEOUT_DURATION_MSEC;
      break;
    }
  }

  display.println();

  // If connected, execute HTTP POST
  if (WiFi.status() == WL_CONNECTED) {
    display.println("Connected");
    display.print("IP: ");
    display.println(WiFi.localIP());
    display.display();

    wifiConnected = true;
  } else {
    // WiFi connection timeout
    display.println("Timed out");
    display.display();
    delay(2000);
  }
}

void enterDeepSleep() {
  // Sleep (10s)
  Serial.println("Entering deep sleep :)");
  ESP.deepSleep(0);
  delay(500);
}

void triggerIFTTT() {
  Serial.print("Connecting to ");
  Serial.println(host);

  // Use WiFiClient class to create TCP connections
  const int httpPort = 80;
  if (!client.connect(host, httpPort)) {
    Serial.println("Connection failed");
    return;
  }

  // We now create a URI for the request
  Serial.print("Requesting URL: ");
  Serial.println(url);

  String json = "{";
  doubleToStr(bme280Data.t, strBuffer);
  json += "\"value1\":\"" + String(strBuffer) + "\",";
  doubleToStr(bme280Data.p, strBuffer);
  json += "\"value2\":\"" + String(strBuffer) + "\",";
  doubleToStr(bme280Data.h, strBuffer);
  json += "\"value3\":\"" + String(strBuffer) + "\"}";

  // This will send the request to the server
  clearDisplay();
  display.println("Triggering IFTTT...");
  display.display();
  client.println(String("POST ")  + url + " HTTP/1.1");
  client.println(String("Host: ") + host);
  client.println("Cache-Control: no-cache");
  client.println("Content-Type: application/JSON");
  client.print("Content-Length: ");
  client.println(json.length());
  client.println();
  client.println(json);

  delay(1000);

  // Read all the lines of the reply from server and print them to Serial
  while (client.available()) {
    String line = client.readStringUntil('\r');
    display.print(line);
    display.display();

    if(line.indexOf("OK")>=0 && line.indexOf("HTTP")>=0){
      iftttTriggerSucceeded = true;
    }
  }

  Serial.println();
  Serial.println("Closing connection");
  delay(1000);
  clearDisplay();
}

void loop() {
  display.display();
  // Wait 10sec
  delay(10000);
  // Clear display
  display.clearDisplay();
  display.display();
  display.ssd1306_command(SSD1306_DISPLAYOFF);
  // Enter deep sleep
  enterDeepSleep();
}
