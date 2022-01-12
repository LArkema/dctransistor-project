/*
 * Initial Arduino for WMATA_PCB project
 * Based on BasicHTTPSClient.ino in ESP8266 example sketches
 * Main difference is connection to api.wmata.com with api_key header
 * Logan Arkema, 10/22/2021
*/


#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "arduino_secrets.h"

char* ssid = SECRET_SSID;
const char* password = SECRET_PASS; 
const char* wmata_fingerprint = "D2 1C A6 D1 BE 10 18 B3 74 8D A2 F5 A2 DE AB 13 7D 07 63 BE"; //Expires 10/22/2022 
String wmata_host = "https://api.wmata.com";
String station_endpoint = "/StationPrediction.svc/json/GetPrediction/";

ESP8266WiFiMulti WiFiMulti;

void setup() {
  Serial.begin(9600); //NodeMCU ESP8266 runs on 9600 baud rate

  //Limit WiFi to station mode and connect to network defined in arduino_secrets.h
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);

  //Connect to WiFi
  while (WiFiMulti.run() != WL_CONNECTED){
    delay(1000);
    Serial.println("Connecting to WiFi...");
    yield();
  }
  Serial.println("Wifi Connected");
  
}

// the loop function runs over and over again forever
void loop() {

  //Define client responsible for managing secure connection api.wmata.com
  WiFiClientSecure client;
  client.setFingerprint(wmata_fingerprint);
  client.setTimeout(15000);

  HTTPClient https;

/*
  //String tmpstring = "";
  const char* direction = new char[16];
  const char* destination = new char [2];
  const char* min = new char [4];
  */


  StaticJsonDocument<500> doc;

  //Define URL for GET request and confirm correctness
  String station_code = "A01";
  String endpoint = wmata_host + station_endpoint + station_code;

  //Connect and confirm HTTPS connection to api.wmata.com
  if (https.begin(client, endpoint)) {
    //Serial.println("In https connection");
    yield();
    //If successful, add API key and request station data
    https.addHeader("api_key", SECRET_WMATA_API_KEY);
    int httpCode = https.GET();

    //Print out HTTP code and, if successful, JSON station data
    if (httpCode > 0) {
      String payload = https.getString();
      //Serial.println(payload);

      DeserializationError error = deserializeJson(doc, payload);

      //Relevant JSON variables: Group (1 or 2 - direction) ; Destination (short) / DestinationCode (A13) / DestinationName (full) ; Min (positive int, "ARR" or "BRD")
      yield();
      
      //train = doc["Trains"][0];

      //Serial.println(train["Destination"]);

      const char* direction0 = doc["Trains"][0]["Group"].as<char*>();
      const char* destination0 = doc["Trains"][0]["Destination"].as<char*>();
      const char* min0 = doc["Trains"][0]["Min"].as<char*>();

      const char* direction1 = doc["Trains"][1]["Group"].as<char*>();
      const char* destination1 = doc["Trains"][1]["Destination"].as<char*>();
      const char* min1 = doc["Trains"][1]["Min"].as<char*>();

      Serial.printf("Direction: %s, towards %s in %s minutes\n", direction0, destination0, min0);
      Serial.printf("Direction: %s, towards %s in %s minutes\n", direction1, destination1, min1);
    }
    else {
      Serial.println("GET Request failed");
      Serial.println(httpCode);
    }//end httpCode
    
  }//END https.begin

  else {
    Serial.println("HTTPS connection failed");
  }

  delay(3000);
}//END LOOP
