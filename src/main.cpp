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
#include <HardwareSerial.h>
#include "secrets.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <TinyGPS++.h>
#include <Preferences.h> // for storing WiFi credentials and configuration settings

// declaring the various pins used for GPIO
#define CAR_POWER_PIN 14
#define BATTERY_VOLTAGE_PIN 34
#define LED_INDICATOR_PIN 27
#define BUZZER_PIN 15

// declaring other variables
unsigned long lastGPSUpdate = 0;        // for timing
unsigned long gpsUpdateInterval = 2000; // update GPS data every 2 seconds

unsigned long lastBatteryReading = 0;        // for timing battery readings
unsigned long batteryReadingInterval = 1000; // read battery status every 1 second

// creating object instances
HTTPClient http;
Preferences prefs;
HardwareSerial gpsSerial(1); // using UART1 for GPS module

TinyGPSPlus gps;

String userToken = "";
bool tokenAvailable = false;
String tokenPostRequestResponse = "";

String messagePayload = "";

char gpsLatitude[15];
char gpsLongitude[15];
char gpsAltitude[10];
char gpsSpeed[10];

char battVoltage[8];
char battPercentage[8];

bool gpsFix = false;

void saveAuthToken(const char *generatedToken)
{ // saving the authToken to memory

  // TODO: convert it from Char/String and store it;
  prefs.begin("auth", false);
  prefs.putString("token", generatedToken);
  prefs.end();
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

bool handShakeAuthentication()
{ // checks if the generated token is active
  http.begin("http://34.244.3.57:3000/api/devices/handshake");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + userToken);

  // TODO: Add a local JSON payload for authentication
  char handShakePayLoad[100];

  int httpResponseCode = http.GET();
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

  http.end(); // free resources
  return (httpResponseCode == 200);
}

bool generateNewAuthenticationToken()
{ // generate token based on deviceId and userId => stored in secrets

  http.begin("http://34.244.3.57:3000/api/devices/token?deviceId=glowTracker-002"); // TODO: add the /endpoint
  http.addHeader("Content-Type", "application/json");

  int httpAuthRequestResponseCode = http.GET();

  if (httpAuthRequestResponseCode == 200)
  {
    tokenPostRequestResponse = http.getString();
    Serial.print("Token Generated");
    Serial.println(tokenPostRequestResponse);
  }
  else
  {
    Serial.print("Error generating token: ");
    Serial.println(httpAuthRequestResponseCode);
    tokenPostRequestResponse = "";
  }

  http.end(); // free resources
  return (httpAuthRequestResponseCode == 200);
}

void extractAndSaveAuthenticationToken()
{ // extract the authToken and save to memory
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, tokenPostRequestResponse);

  if (error)
  {
    Serial.print("Token Extraction failed: ");
    Serial.println(error.c_str());
    return;
  }

  // extract and save it to the prefs library
  const char *userToken = doc["token"];
  // save the token to memory
  saveAuthToken(userToken);
}

// function to read the gps-data and device-status and send it to the server

bool readGPSLocation()
{
  bool gpsFix = false;

  while (gpsSerial.available() > 0)
  {
    gps.encode(gpsSerial.read());

    // If a new GPS fix is available, print out some data
    if (gps.location.isUpdated())
    {
      gpsFix = true;
      // saving the location to the char arrays
      snprintf(gpsLatitude, sizeof(gpsLatitude), "%.6f", gps.location.lat());
      snprintf(gpsLongitude, sizeof(gpsLongitude), "%.6f", gps.location.lng());
      snprintf(gpsAltitude, sizeof(gpsAltitude), "%.2f", gps.altitude.meters());
      snprintf(gpsSpeed, sizeof(gpsSpeed), "%.2f", gps.speed.kmph());

      Serial.print("Latitude= ");
      Serial.print(gpsLatitude);
      Serial.print(" Longitude= ");
      Serial.println(gpsLongitude);

      Serial.print("Altitude= ");
      Serial.println(gpsAltitude);

      Serial.print("Speed= ");
      Serial.println(gpsSpeed); // speed in km/h
    }
  }
  return gpsFix;
}

// send gps data to the server
bool sendDeviceData(const char *payLoad) {
  http.begin("http://34.244.3.57:3000/api/locations/updateLocation");
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", String("Bearer ") + userToken);

  //make a POST request with the payload
  int httpResponseCode = http.POST((uint8_t *)payLoad, strlen(payLoad));

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    String response = http.getString();
    Serial.println("Response from server:");
    Serial.println(response);
  } else {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }

  return (httpResponseCode == 200);

  http.end(); // free resources
}


// send a device heartbeat to the server
void sendDeviceHeartbeat() {
  JsonDocument doc;

  doc["deviceId"] = DEVICE_ID;
  doc["status"] = "heartbeat";
  doc["gpsFix"] = gpsFix; 

  // Nested battery object
  JsonObject battery = doc["battery"].to<JsonObject>();
  battery["percent"] = battPercentage;
  battery["voltage"] = battVoltage;
  battery["charging"] = false;

  doc["signal"] = -72;

  char heartBeatPayLoad[200];
  // Serialize JSON to string
  serializeJsonPretty(doc, heartBeatPayLoad);

  // upload the heartbeat data
  sendDeviceData(heartBeatPayLoad);
  delay(10000);

}

//upload full-device data to the server
void updateLiveLocationData(){
  
  JsonDocument doc;

  doc["deviceId"] = DEVICE_ID;
  
  // Location object
  JsonObject location = doc["location"].to<JsonObject>();
  location["type"] = "Point";

  // Coordinates array
  JsonArray coordinates = location["coordinates"].to<JsonArray>();
  coordinates.add(atof(gpsLongitude)); // longitude first
  coordinates.add(atof(gpsLatitude));  // latitude second

  // Other fields
  doc["speed"] = atof(gpsSpeed);
  doc["gpsFix"] = gpsFix;

  // Battery object
  JsonObject battery = doc["battery"].to<JsonObject>();
  battery["percent"] = atof(battPercentage);
  battery["voltage"] = atof(battVoltage);
  battery["charging"] = false;

  doc["signal"] = -72;

  size_t jsonSize = measureJson(doc);

  Serial.print("JSON Size: ");
  Serial.println(jsonSize);

  // Serialize to JSON
  serializeJsonPretty(doc, Serial);
  
}

void initializeGPIO()
{
  pinMode(CAR_POWER_PIN, INPUT);
  pinMode(BATTERY_VOLTAGE_PIN, INPUT);
  pinMode(LED_INDICATOR_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
}

// function to read the battery status

// setting up sensors and the modules
void initializeGPS()
{
  gpsSerial.begin(9600, SERIAL_8N1, 13, 12); // RX, TX pins
  delay(1000);                               // wait for GPS module to initialize
}

// read the battery voltage
float readBatteryVoltage()
{

  uint32_t batterySampleReadings = 0;
  const uint8_t numSamples = 10;

  for (uint8_t i = 0; i < numSamples; i++)
  {
    batterySampleReadings += analogRead(BATTERY_VOLTAGE_PIN);
    delay(10); // small delay between samples
  }

  float averageBatteryAnalogValue = batterySampleReadings / (float)numSamples;

  float batteryVoltage = (averageBatteryAnalogValue / 4095.0) * 3.3 * 2; // assuming a voltage divider
  return batteryVoltage;
}

// send gps data to the server

// send device-heartbeat

void checkDeviceStatus()
{
  int carPowerStatus = digitalRead(CAR_POWER_PIN);
  uint16_t batteryAnalogValue = analogRead(BATTERY_VOLTAGE_PIN); // assuming a voltage divider

  if (carPowerStatus == HIGH)
  {
    Serial.println("Car Power: ON");
  }
  else
  {
    Serial.println("Car Power: OFF");
  }

  float batteryVoltage = readBatteryVoltage();
  int batteryPercentage = constrain((batteryVoltage / 4.2) * 100, 0, 100); // assuming 4.2V is full charge

  snprintf(battVoltage, sizeof(battVoltage), "%.2f", batteryVoltage);
  snprintf(battPercentage, sizeof(battPercentage), "%d", batteryPercentage);

  Serial.print("Battery Voltage: ");
  Serial.print(battVoltage);
  Serial.println(" V");
  Serial.print("Battery Percentage: ");
  Serial.print(battPercentage);
  Serial.println(" %");
}

void setup()
{
  Serial.begin(115200);
  connectToWiFi();  // connect to WiFi network
  initializeGPS();  // initialize the GPS module
  initializeGPIO(); // initialize GPIO pins

  retrieveAuthToken(); // get the saved auth token

  if (handShakeAuthentication())
  {
    Serial.println("Token is valid, proceeding...");
    retrieveAuthToken();
  }
  else
  {
    Serial.println("Token is invalid or not present, generating a new one...");

    if (generateNewAuthenticationToken())
    {
      extractAndSaveAuthenticationToken();
    }
    else
    {
      Serial.println("Error doing that!");
    }
    // retrieve the saved token
    retrieveAuthToken();
  }
}

void loop()
{
  // read data from the GPS module
  unsigned long currentMillis = millis();
  if (currentMillis - lastGPSUpdate >= gpsUpdateInterval)
  {
    lastGPSUpdate = currentMillis;
    gpsFix = readGPSLocation();
  }

  Serial.print("Device GPS Fix: ");
  Serial.println(gpsFix);

  // send Device dataHeartbeat
  //sendDeviceHeartbeat();

  // check the device status
  checkDeviceStatus();

  // update the live-location data
  updateLiveLocationData();
}
