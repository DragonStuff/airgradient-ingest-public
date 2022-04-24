/*
This is modified code for the AirGradient DIY Air Quality Sensor with an ESP8266 Microcontroller. Original code was written by AirGradient under the MIT license.

---
Changes by Alexander (https://github.com/DragonStuff).
- Use HTTPs to connect to ingest service.
- Add additional catch logic to protect microcontroller from crashing.
--

The codes needs the following libraries installed:
"WifiManager by tzapu, tablatronix" tested with Version 2.0.3-alpha
"ESP8266 and ESP32 OLED driver for SSD1306 displays by ThingPulse, Fabrice Weinberg" tested with Version 4.1.0

Configuration:
Please set in the code below which sensors are connected. 
You can also switch PM2.5 from ug/m3 to US AQI and Celcius to Fahrenheit.
WiFi is obviously required if you want to send the data somewhere.

*/

#include <AirGradient.h>

#include <WiFiManager.h>

#include <ESP8266WiFi.h>

#include <ESP8266HTTPClient.h>

#include <Wire.h>

#include "SSD1306Wire.h"

AirGradient ag = AirGradient();

SSD1306Wire display(0x3c, SDA, SCL);

// set sensors that you do not use to false
boolean hasPM = true;
boolean hasCO2 = true;
boolean hasSHT = true;

// set to true to switch PM2.5 from ug/m3 to US AQI
boolean inUSaqi = false;

// set to true to switch from Celcius to Fahrenheit
boolean inF = false;

// set to true if you want to connect to wifi. The display will show values only when the sensor has wifi connection
boolean connectWIFI = true;

// Replace the following address with your Lambda function's URL.
String APIROOT = "https://your-lambda-function-url.lambda-url.ap-northeast-1.on.aws/";

void setup() {
  Serial.begin(9600);

  display.init();
  display.flipScreenVertically();
  showTextRectangle("Init", String(ESP.getChipId(), HEX), true);

  if (hasPM) ag.PMS_Init();
  if (hasCO2) ag.CO2_Init();
  if (hasSHT) ag.TMP_RH_Init(0x44);

  if (connectWIFI) connectToWifi();
  delay(2000);
}

void loop() {

  // create payload

  String payload = "{\"wifi\":" + String(WiFi.RSSI()) + ",";

  if (hasPM) {
    int PM2 = ag.getPM2_Raw();
    payload = payload + "\"pm02\":" + String(PM2);

    if (inUSaqi) {
      showTextRectangle("AQI", String(PM_TO_AQI_US(PM2)), false);
    } else {
      showTextRectangle("PM2", String(PM2), false);
    }

    delay(3000);

  }

  if (hasCO2) {
    if (hasPM) payload = payload + ",";
    int CO2 = ag.getCO2_Raw();
    payload = payload + "\"rco2\":" + String(CO2);
    showTextRectangle("CO2", String(CO2), false);
    delay(3000);
  }

  if (hasSHT) {
    if (hasCO2 || hasPM) payload = payload + ",";
    TMP_RH result = ag.periodicFetchData();
    payload = payload + "\"atmp\":" + String(result.t) + ",\"rhum\":" + String(result.rh);

    if (inF) {
      showTextRectangle(String((result.t * 9 / 5) + 32), String(result.rh) + "%", false);
    } else {
      showTextRectangle(String(result.t), String(result.rh) + "%", false);
    }

    delay(3000);
  }

  payload = payload + "}";

  // send payload
  if (connectWIFI) {
    std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);

    client->setInsecure();

    Serial.println(payload);
    String POSTURL = APIROOT + "sensors/airgradient:" + String(ESP.getChipId(), HEX) + "/measures";
    Serial.println(POSTURL);

    HTTPClient https;

    if (https.begin(*client, POSTURL)) {
      https.addHeader("content-type", "application/json");
      int httpCode = https.POST(payload);
      if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTPS] POST... code: %d\n", httpCode);

        // file found at server
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();
          Serial.println(payload);
        }
      } else {
        Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
    https.end();
    } else {
      Serial.printf("[HTTPS] Unable to connect\n");
    }
    delay(21000);
  }
}

// DISPLAY
void showTextRectangle(String ln1, String ln2, boolean small) {
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  if (small) {
    display.setFont(ArialMT_Plain_16);
  } else {
    display.setFont(ArialMT_Plain_24);
  }
  display.drawString(32, 16, ln1);
  display.drawString(32, 36, ln2);
  display.display();
}

// Wifi Manager
void connectToWifi() {
  WiFiManager wifiManager;
  //WiFi.disconnect(); //to delete previous saved hotspot
  String HOTSPOT = "AIRGRADIENT-" + String(ESP.getChipId(), HEX);
  wifiManager.setTimeout(120);
  if (!wifiManager.autoConnect((const char * ) HOTSPOT.c_str())) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
  }

}

// Calculate PM2.5 US AQI
int PM_TO_AQI_US(int pm02) {
  if (pm02 <= 12.0) return ((50 - 0) / (12.0 - .0) * (pm02 - .0) + 0);
  else if (pm02 <= 35.4) return ((100 - 50) / (35.4 - 12.0) * (pm02 - 12.0) + 50);
  else if (pm02 <= 55.4) return ((150 - 100) / (55.4 - 35.4) * (pm02 - 35.4) + 100);
  else if (pm02 <= 150.4) return ((200 - 150) / (150.4 - 55.4) * (pm02 - 55.4) + 150);
  else if (pm02 <= 250.4) return ((300 - 200) / (250.4 - 150.4) * (pm02 - 150.4) + 200);
  else if (pm02 <= 350.4) return ((400 - 300) / (350.4 - 250.4) * (pm02 - 250.4) + 300);
  else if (pm02 <= 500.4) return ((500 - 400) / (500.4 - 350.4) * (pm02 - 350.4) + 400);
  else return 500;
};
