// include an MIT License here

#include <Arduino.h>
#include "secrets.h"

// include other necessary libraries here
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// function prototypes
void connectToWiFi();
void createJsonDoc (char*, size_t size);

// creating objects for WiFi and HTTPClient
HTTPClient http;

void setup() {
  Serial.begin(115200);
  connectToWiFi();
  // create a JSON buffer
  char payLoad[100];
  createJsonDoc(payLoad, sizeof(payLoad));
  Serial.println("JSON Payload:");
  Serial.println(payLoad);

  // Example of sending HTTP GET request
  http.begin("http://jsonplaceholder.typicode.com/todos/1");
  int httpResponseCode = http.GET();
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  // get the response payload
  if (httpResponseCode > 0) { 
    String response = http.getString();
    Serial.println("Response payload:");
    Serial.println(response);
  } else {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpResponseCode);
  }
  http.end(); // free resources

}

void loop() {
 
}

// creating a json document 
void createJsonDoc (char* payLoad, size_t size) {
  JsonDocument doc;

  doc["email"] = "devkaybee@gmail.com";
  doc["password"] = "d3kaybee";

  serializeJsonPretty(doc, payLoad, size);
  
  //to know exact size of json to be allocated
  size_t jsonSize = measureJson(doc);
  Serial.print("Measured JSON size: ");  
  Serial.println(jsonSize);
}



// configure WiFi connection using credentials from secrets.h
void connectToWiFi() {
  Serial.print("Connecting to WiFi...");
  Serial.println(WIFI_SSID);

  uint8_t wifiConnectAttempts = 0;
  const uint8_t maxAttempts = 20;

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
