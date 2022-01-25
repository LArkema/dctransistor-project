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
#include "SimpleList.h" 

char* ssid = SECRET_SSID;
const char* password = SECRET_PASS; 
const char* wmata_fingerprint = "D2 1C A6 D1 BE 10 18 B3 74 8D A2 F5 A2 DE AB 13 7D 07 63 BE"; //Expires 10/22/2022 
String wmata_host = "https://api.wmata.com";
String station_endpoint = "/StationPrediction.svc/json/GetPrediction/";

ESP8266WiFiMulti WiFiMulti;

const char red_stations[10][4] = {"B02", "B03", "B04", "B05", "B06", "B07", "B08", "B09", "B10", "B11"}; /* Flawfinder: ignore */

int LED_LENGTH = 10;
int leds[10] = {15, 13, 12, 14, 2, 0, 4, 5, 16, 10};

void setup() {
  Serial.begin(9600); //NodeMCU ESP8266 runs on 9600 baud rate

  //Limit WiFi to station mode and connect to network defined in arduino_secrets.h
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);

  //Connect to WiFi
  while (WiFiMulti.run() != WL_CONNECTED){
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Wifi Connected");

  for(int i=0; i<LED_LENGTH; i++){
    pinMode(leds[i], OUTPUT);
  }
  
}//END SETUP

// the loop function runs over and over again forever
void loop() {

  //Define client responsible for managing secure connection api.wmata.com
  WiFiClientSecure client;
  client.setFingerprint(wmata_fingerprint);
  client.setTimeout(15000);

  HTTPClient https;
  https.useHTTP10(true); //enables more efficient Json deserialization per https://arduinojson.org/v6/how-to/use-arduinojson-with-httpclient/

  SimpleList redline = SimpleList();

  //redline.arrived(0);
  //redline.arrived(1);

  //Define filter to apply to WMATA API response - only put relevant values in memory.
  StaticJsonDocument<100> normal_filter;

  JsonObject filter_Trains_0 = normal_filter["Trains"].createNestedObject();
  filter_Trains_0["Destination"] = true;
  filter_Trains_0["LocationCode"] = true;
  filter_Trains_0["Group"] = true;
  filter_Trains_0["Min"] = true;

  StaticJsonDocument<48> train_pos_fiter;
  train_pos_fiter["TrainPositions"][0]["CircuitId"] = true;

  while (true){

    for(int i=redline.getLen()-1; i>=0; i--){

      Serial.println(redline.getState());
      uint16_t json_size = 800;
      StaticJsonDocument<100> filter = normal_filter;
      

      //Define URL for GET request and confirm correctness
      String endpoint = "";
      if(stations[i] == stations.last_station){
        endpoint = wmata_host + "/TrainPositions/TrainPositions?contentType=json";
        json_size = 2048;
        filter = train_pos_fiter;
      }
      //Generate station code based on value of waiting station
      else{
        String station_code = "B";
        if (redline.stations[i]+2 < 10) {station_code += "0";}
        station_code += String(redline.stations[i]+2);

        endpoint = wmata_host + station_endpoint + station_code;
      }

      //Connect and confirm HTTPS connection to api.wmata.com
      if (https.begin(client, endpoint)) {
        //Serial.println("In https connection");
        //If successful, add API key and request station data
        https.addHeader("api_key", SECRET_WMATA_API_KEY);
        int httpCode = https.GET();

        //Print out HTTP code and, if successful, JSON station data
        if (httpCode > 0) {
          //String payload = https.getString();
          //Serial.println(payload);

          DynamicJsonDocument doc(json_size);
          DeserializationError error = deserializeJson(doc, https.getStream(), DeserializationOption::Filter(filter));

          //Relevant JSON variables: Group (1 or 2 - direction) ; Destination (short) / DestinationCode (A13) / DestinationName (full) ; Min (positive int, "ARR" or "BRD")
      

          //serializeJsonPretty(doc, Serial); //print out all outputs from deserialization
          uint8_t j = 0;
          char d = ' ';
          
          while(d != '1'){
            const char* dest = doc["Trains"][j]["Group"].as<const char*>();
            //Serial.println(dest);
            d = dest[0];
            if(d != '1'){j++;}
          };

          
          const char* location0 = doc["Trains"][j]["LocationCode"].as<char*>();
          const char* destination0 = doc["Trains"][j]["Destination"].as<char*>();
          const char* min0 = doc["Trains"][j]["Min"].as<char*>();

          if(strcmp(doc["Trains"][j]["Min"].as<const char*>(), "ARR") == 0 || 
              strcmp(doc["Trains"][j]["Min"].as<const char*>(), "BRD") == 0){
            
            digitalWrite(leds[redline.stations[i]], 1);
            if(redline.stations[i] != 0){digitalWrite(leds[redline.stations[i]-1], 0);}
            
            redline.arrived(i);
            //Serial.println("Arrived!");
            Serial.println(redline.getState());
          }

          /*
          const char* direction1 = doc["Trains"][1]["Group"].as<char*>();
          const char* destination1 = doc["Trains"][1]["Destination"].as<char*>();
          const char* min1 = doc["Trains"][1]["Min"].as<char*>();
          */

          Serial.printf("At %s, towards %s in %s minutes\n", location0, destination0, min0);
          //Serial.printf("Direction: %s, towards %s in %s minutes\n", direction1, destination1, min1);
          

        }
        else {
          Serial.println("GET Request failed");
          Serial.println(httpCode);
        }//end httpCode
        
      }//END https.begin

      else {
        Serial.println("HTTPS connection failed");
      }

        }//end for (per station check)
      delay(3000);
  }//end while(true) - all waiting stations loop
}//END LOOP
