#include <Arduino.h>
#include "secrets.h"

// include other necessary libraries here
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// function prototypes
void connectToWiFi();

void setup() {
  Serial.begin(115200);
  connectToWiFi();
 
}

void loop() {
 
}


// configure WiFi connection using credentials from secrets.h
void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  Serial.println(WIFI_SSID);

  int wifiConnectAttempts = 0;
  const int maxAttempts = 20;

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() !=WL_CONNECTED && wifiConnectAttempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    wifiConnectAttempts++;
  }

  // Check if connected
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi.");
  }
}
