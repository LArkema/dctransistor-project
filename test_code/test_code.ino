/* File for working / test code (actively compiled and uploaded to Arduino).
 * Code from this file occasionally moved to "snipits" folder when substantive feature developed.
 * Logan Arkema, current date
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
String station_endpoint = "/TrainPositions/TrainPositions?contentType=json";

ESP8266WiFiMulti WiFiMulti;

//List of WMATA CircuitIDs for Judiciary Square - Wheaton (towards Glenmont)
const uint16_t red_stations[10] = {477, 485, 496, 513, 527, 548, 571, 591, 611, 629}; /* Flawfinder: ignore */

int LED_LENGTH = 10;
int leds[10] = {15, 13, 12, 14, 2, 0, 4, 5, 16, 10};

//Function for setting stations state at boot-time
void SetInitialState(uint16_t* train_positions, SimpleList &line_state){



}//end SetInitialState




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


  //Filters data from TrainPositions API to just get CircuitIds with Train
  StaticJsonDocument<48> train_pos_fiter;
  train_pos_fiter["TrainPositions"][0]["CircuitId"] = true;

  while (true){

    for(int i=redline.getLen()-1; i>=0; i--){

      Serial.println(redline.getState());
      uint16_t json_size = 2048;
      

      //Define URL for GET request and confirm correctness
      String endpoint = wmata_host + station_endpoint;
      
      //Connect and confirm HTTPS connection to api.wmata.com
      if (https.begin(client, endpoint)) {
        //If successful, add API key and request station data
        https.addHeader("api_key", SECRET_WMATA_API_KEY);
        int httpCode = https.GET();

        //Print out HTTP code and, if successful, parse data returned from API and update staion state
        if (httpCode > 0) {
          //String payload = https.getString();
          //Serial.println(payload);

          DynamicJsonDocument doc(json_size);
          DeserializationError error = deserializeJson(doc, https.getStream(), DeserializationOption::Filter(train_pos_fiter));

          //serializeJsonPretty(doc, Serial); //print out all outputs from deserialization
        

         //Iterate through all returned train positions, put the ones on target section of redline track into an array.
         uint16_t array_size = doc["TrainPositions"].size();
         uint16_t train_positions[20] = {};
         uint8_t k = 0;
         uint16_t j = 0;
         for(j=0; j<array_size; j++){ //TODO: replace with Iterators - https://arduinojson.org/v6/api/jsonarray/begin_end/
           const uint16_t circID = doc["TrainPositions"][j]["CircuitId"].as<unsigned int>();
           if (circID >= red_stations[0] && circID <= red_stations[9]){
             train_positions[k] = circID;
             k++;
           }
         }
         
          Serial.println("All trains in test range");
          for(int t=0; t<k; t++){
            Serial.printf("%d, ",train_positions[t]); /*Flawfinder: ignore */
          }
          Serial.println();

          Serial.println("Setting initial state");
          redline.setInitialState(train_positions, k, red_stations, 10);
          Serial.println(redline.getState());

          //Put target station in local circuit
          uint16_t station_circuit = red_stations[redline.stations[i]]; //redline stores station indexes.

          //See if any of the current train positions are arriving, at, or just leaving destination station.
          //If so, update station state.
          for(int t=0; t<array_size; t++){
            if(train_positions[t] > (station_circuit - 3) && train_positions[t] < (station_circuit + 1) ){

              Serial.printf("Station Circuit: %d\n", station_circuit); /*Flawfinder: ignore */
              Serial.printf("Train Circuit:   %d\n", train_positions[t]); /*Flawfinder: ignore */

              digitalWrite(leds[redline.stations[i]], 1);
              if(redline.stations[i] != 0){digitalWrite(leds[redline.stations[i]-1], 0);}
              redline.arrived(i);
              break;
            }
          }

          

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
