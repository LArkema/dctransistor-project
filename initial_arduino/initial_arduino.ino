/*
 * Initial Arduino for WMATA_PCB project
 * Based on BasicHTTPSClient.ino in ESP8266 example sketches
 * Main difference is connection to api.wmata.com with api_key header
 * Logan Arkema, 10/22/2021
*/


#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include "arduino_secrets.h"

char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;
const char* station_fingerprint = "D2 1C A6 D1 BE 10 18 B3 74 8D A2 F5 A2 DE AB 13 7D 07 63 BE";
String wmata_host = "https://api.wmata.com";

ESP8266WiFiMulti WiFiMulti;

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output

  
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);
  
  while (WiFiMulti.run() != WL_CONNECTED){
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  Serial.println("Wifi Connected");
  
}

// the loop function runs over and over again forever
void loop() {

  WiFiClientSecure client;

  client.setFingerprint(station_fingerprint);
  client.setTimeout(15000);

  HTTPClient https;

  String station_code = "A01";
  String endpoint = wmata_host + "/StationPrediction.svc/json/GetPrediction/" + station_code;

  Serial.println(endpoint);

  if (https.begin(client, endpoint)) {
    https.addHeader("api_key", SECRET_WMATA_API_KEY);

    int httpCode = https.GET();

    if (httpCode > 0) {
      String payload = https.getString();
      Serial.println(httpCode);
      Serial.println(payload);
    }
    else {
      Serial.println("GET Request failed");
      Serial.println(httpCode);
    }
  }

  else {
    Serial.println("HTTPS connection failed");
  }

  delay(3000);
}
