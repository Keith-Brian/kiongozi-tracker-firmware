/*
  MIT License

  Copyright (c) 2025 Keith Brian

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

// include the necessary Arduino libraries (Arduino framework)
#include <Arduino.h>
#include "secrets.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h> // for storing WiFi credentials and configuration settings

// function prototypes
void connectToWiFi();
void sendSimpleGetRequest();
void sendSimplePostRequest(const char *url, const char *jsonPayload);

// creating object instances
HTTPClient http;
Preferences prefs;

String userToken = "";
bool tokenAvailable = false;
String tokenPostRequestResponse = "";

void saveAuthToken(const char *generatedToken);
{ // saving the authToken to memory
  prefs.begin("auth", false);
  prefs.putString("token", userToken);
}

void retrieveAuthToken()
{ // get the authToken from memory
  prefs.begin("auth", true);
  String storedToken = prefs.getString("token", "");

  if (storedToken.length() > 0)
  {
    userToken = storedToken;
    tokenAvailable = true;
    Serial.print("Auth Token: ");
    Serial.println(userToken);
  }
  else
  {
    tokenAvailable = false;
    Serial.println("Error getting token");
  }
}

bool handShakeAuthentication(const char *url, const char *jsonPayLoad)
{ // checks if the generated token is active
  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + userToken);

  int httpResponseCode = http.POST((uint8_t *)jsonPayLoad, strlen(jsonPayLoad));
  Serial.print("HTTP handshake code: ");
  Serial.println(httpResponseCode);

  if (httpResponseCode == 400 || httpResponseCode == 401)
  {
    Serial.println("Invalid Token");
    return (httpResponseCode == 200);
    // TODO: make a request to generate a new token
  }
  else if (httpResponseCode == 200)
  {
    Serial.println("Valid Token Success");
    return (httpResponseCode == 200);
  }
}

bool generateNewAuthenticationToken()
{ // generate token based on deviceId and userId => stored in secrets
  JsonDocument doc;

  doc["device-id"] = DEVICE_ID;
  doc["user-id"] = USER_ID;

  char authPayLoad[100];
  serializeJson(doc, authPayLoad, sizeof(authPayLoad));

  http.begin("//auth-request"); // TODO: add the /endpoint
  http.addHeader("Content-Type", "application/json");

  int httpAuthRequestResponseCode = http.POST((uint8_t *)authPayLoad, strlen(authPayLoad));

  if (httpAuthRequestResponseCode == 200)
  {
    tokenPostRequestResponse = http.getString();
    Serial.print("Token Generated");
    Serial.println(tokenPostRequestResponse);
  }

  return (httpAuthRequestResponseCode == 200);
}

void extractAndSaveAuthenticationToken(){ // extract the authToken and save to memory 
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, tokenPostRequestResponse);
  
  if(error){
    Serial.print("Token Extraction failed: ");
    Serial.println(error.c_str());
    return;
  }

  //extract and save it to the prefs library
  const char *userToken = doc["token"];

  //TODO: Save this to prefs memory 
  
}


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
  // sendSimpleGetRequest();
  // deSerializeJsonResponse(response);
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

// making a simple POST request with JSON payload

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
