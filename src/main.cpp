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

//declaring the various pins used for GPIO
#define CAR_POWER_PIN 14
#define BATTERY_VOLTAGE_PIN 34
#define LED_INDICATOR_PIN 27
#define BUZZER_PIN 15

//declaring other variables
unsigned long lastGPSUpdate = 0; // for timing
unsigned long gpsUpdateInterval = 2000; // update GPS data every 2 seconds


// function prototypes
void connectToWiFi();
void sendSimpleGetRequest();
void sendSimplePostRequest(const char *url, const char *jsonPayload);

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

//function to read the gps-data and device-status and send it to the server

void readGPSLocation(){

  while (gpsSerial.available() > 0) {
    gps.encode(gpsSerial.read());
    
    // If a new GPS fix is available, print out some data
    if (gps.location.isUpdated()) {

      // saving the location to the char arrays
      snprintf(gpsLatitude, sizeof(gpsLatitude), "%.6f", gps.location.lat());
      snprintf(gpsLongitude, sizeof(gpsLongitude), "%.6f", gps.location.lng());
      snprintf(gpsAltitude, sizeof(gpsAltitude), "%.2f", gps.altitude.meters());
      snprintf(gpsSpeed, sizeof(gpsSpeed), "%.2f", gps.speed.kmph ());

      Serial.print("Latitude= "); 
      Serial.print(gpsLatitude); 
      Serial.print(" Longitude= "); 
      Serial.println(gpsLongitude);
      
      Serial.print("Altitude= ");
      Serial.println(gpsAltitude);
      
      Serial.print("Speed= ");
      Serial.println(gpsSpeed);  // speed in km/h
    }
  }
  
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
void initializeGPS(){
  gpsSerial.begin(9600, SERIAL_8N1, 13, 12); // RX, TX pins
  delay(1000); // wait for GPS module to initialize
}

void checkDeviceStatus(){
  int carPowerStatus = digitalRead(CAR_POWER_PIN);
  int batteryAnalogValue = analogRead(BATTERY_VOLTAGE_PIN);
  // Add logic to handle the status readings

  if(carPowerStatus == HIGH){
    Serial.println("Car Power: ON");
  } else {
    Serial.println("Car Power: OFF");
  }

  float batteryVoltage = (batteryAnalogValue / 4095.0) * 3.3 * 2; // assuming a voltage divider
  float batteryPercentage = constrain((batteryVoltage / 4.2) * 100, 0, 100); // assuming 4.2V is full charge
  Serial.print("Battery Voltage: "); 
  Serial.print(batteryVoltage);
  Serial.println(" V");
  Serial.print("Battery Percentage: "); 
  Serial.print(batteryPercentage);  
  Serial.println(" %");
}


void setup()
{
  Serial.begin(115200);
  connectToWiFi(); // connect to WiFi network
  initializeGPS(); // initialize the GPS module
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
  if (currentMillis - lastGPSUpdate >= gpsUpdateInterval) {
    lastGPSUpdate = currentMillis;
    readGPSLocation();
  }
  // check the device status
  checkDeviceStatus();
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
