// include an MIT License here

#include <Arduino.h>
#include "secrets.h"

// include other necessary libraries here
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// function prototypes
void connectToWiFi();
void createJsonDoc(char *, size_t size);
void sendSimpleGetRequest();
void deSerializeJsonResponse(const String Json);
void sendSimplePostRequest(const char *url, const char *jsonPayload);

// creating objects for WiFi and HTTPClient
HTTPClient http;

String response = "";

void setup()
{
  Serial.begin(115200);
  connectToWiFi();
  // create a JSON buffer
  char payLoad[100];
  createJsonDoc(payLoad, sizeof(payLoad));
  Serial.println("JSON Payload:");
  Serial.println(payLoad);

  // sending a sample POST request
  sendSimplePostRequest("http://34.244.3.57:3000/api/auth/login", payLoad);

  // Example of sending HTTP GET request
  //sendSimpleGetRequest();
  //deSerializeJsonResponse(response);
}

void loop()
{
}

// simple GET request example
void sendSimpleGetRequest()
{
  http.begin("http://34.244.3.57:3000/api/devices/");
  int httpResponseCode = http.GET();
  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  // get the response payload
  if (httpResponseCode > 0)
  {
    response = http.getString();
    Serial.println("Response payload:");
    Serial.println(response);
  }
  else
  {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpResponseCode);
  }
  http.end(); // free resources
}

// making a simple POST request with JSON payload
void sendSimplePostRequest(const char *url, const char *jsonPayload)
{
  http.begin(url);
  http.addHeader("Content-Type", "application/json");



  int httpResponseCode = http.POST((uint8_t *)jsonPayload, strlen(jsonPayload));

  Serial.print("HTTP Response code: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode > 0)
  {
    String response = http.getString();
    Serial.println("Response payload:");
    Serial.println(response);
  }
  else
  {
    Serial.print("Error on HTTP request: ");
    Serial.println(httpResponseCode);
  }
  http.end(); // free resources
}


// Simple JSON deSerialization from the request
void deSerializeJsonResponse(const String Json)
{
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, Json);

  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char *title = doc["title"];
  bool completed = doc["completed"];

  Serial.print("Title: ");
  Serial.println(title);
  Serial.print("Completed: ");
 // Serial.println(completed ? "true" : "false");
}


// creating a json document
void createJsonDoc(char *payLoad, size_t size)
{
  JsonDocument doc;

  doc["email"] = "devkaybee@gmail.com";
  doc["password"] = "d3vkaybee";

  serializeJsonPretty(doc, payLoad, size);

  // to know exact size of json to be allocated
  size_t jsonSize = measureJson(doc);
  Serial.print("Measured JSON size: ");
  Serial.println(jsonSize);
}

// configure WiFi connection using credentials from secrets.h
void connectToWiFi()
{
  Serial.print("Connecting to WiFi...");
  Serial.println(WIFI_SSID);

  uint8_t wifiConnectAttempts = 0;
  const uint8_t maxAttempts = 20;

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  while (WiFi.status() != WL_CONNECTED && wifiConnectAttempts < maxAttempts)
  {
    delay(500);
    Serial.print(".");
    wifiConnectAttempts++;
  }

  // Check if connected
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("\nWiFi connected successfully!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("\nFailed to connect to WiFi.");
  }
}
