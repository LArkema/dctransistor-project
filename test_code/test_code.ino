/* File for working / test code (actively compiled and uploaded to Arduino).
 * Code from this file occasionally moved to "snipits" folder when substantive feature developed.
 * Logan Arkema, current date
*/


#include <ESP8266WiFi.h>
//#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "arduino_secrets.h"
#include "miscFuncs.h" 


//Connection (WiFi and API endpoint) configuration
String ssid = SECRET_SSID;
String password = SECRET_PASS; 
const char* wmata_fingerprint = "D2 1C A6 D1 BE 10 18 B3 74 8D A2 F5 A2 DE AB 13 7D 07 63 BE"; //Expires 10/22/2022 
String endpoint = "https://api.wmata.com/TrainPositions/TrainPositions?contentType=json";


TrainLine redline = TrainLine();
//const bool direction = 1;

//Define client responsible for managing secure connection api.wmata.com
WiFiClientSecure client;
HTTPClient https;


//Filters data from TrainPositions API to just get CircuitIds with Train
StaticJsonDocument<48> train_pos_fiter; 
const uint16_t json_size = 2048; //change to 3072 if add DirectionNum

//SETUP WIFI CONNECTION
void setup() {
  Serial.begin(9600); //NodeMCU ESP8266 runs on 9600 baud rate

  //Connect to network defined in example_arduino_secrets.h
  WiFi.begin(ssid, password);

  //Connect to WiFi
  while (WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Wifi Connected");

  //Set connection and filter settings.
  client.setFingerprint(wmata_fingerprint);
  client.setTimeout(15000);

  https.useHTTP10(true); //enables more efficient Json deserialization per https://arduinojson.org/v6/how-to/use-arduinojson-with-httpclient/

  train_pos_fiter["TrainPositions"][0]["CircuitId"] = true; //Possible TODO: Add "DirectionNum" to filter.
  
}//END SETUP

// the loop function runs over and over again forever
void loop() {

  Serial.println(redline.getState());

  //Connect and confirm HTTPS connection to api.wmata.com
  if (https.begin(client, endpoint)) {
    //If successful, add API key and request station data
    https.addHeader("api_key", SECRET_WMATA_API_KEY);
    int httpCode = https.GET();

    //Print out HTTP code and, if successful, parse data returned from API and update staion state
    if (httpCode > 0) {

      DynamicJsonDocument doc(json_size);
      DeserializationError error = deserializeJson(doc, https.getStream(), DeserializationOption::Filter(train_pos_fiter));

      //serializeJsonPretty(doc, Serial); //print out all outputs from deserialization
    

      //Iterate through all returned train positions, put the ones on target section of redline track into an array.
      uint16_t train_positions[2][10] = {};
      uint8_t train_len[2] = {0,0};
      //uint16_t array_size = doc["TrainPositions"].size();
      //uint16_t j = 0;

      //JsonArray arr = doc["TrainPositions"].as<JsonArray>();

      for(JsonObject train : doc["TrainPositions"].as<JsonArray>()){ //from https://arduinojson.org/v6/api/jsonarray/begin_end/
      
        // Eventually loop through train lines
        for(uint8_t dir=0; dir<2; dir++){

        
          int8_t cf = (dir * -2) + 1; //turns 0 to 1 and 1 to -1 - allows for same comparison logic when train moves negatively
          const int16_t circID = train["CircuitId"].as<unsigned int>() * cf;

          //uint16_t start_circuit = 0; //redline.getStationCircuit(0, direction) * coefficient;
          //uint16_t end_circuit = 0; //redline.getStationCircuit(redline.getTotalNumStations()-1, direction) * coefficient;


          //Serial.printf("Circuit %d must be greater than %d and less than %d\n", circID, start_circuit, end_circuit);

          //Add train that's in given line's circuit range, or opp direction's 1st circuit, to a list to inspect.
          //Optional TODO: Add "DirectionNum" to filter and logic here. If so, add to filter and change size.
          if ( (circID >= (redline.getStationCircuit(0, dir)*cf) && circID <= (redline.getLastCID(dir)*cf) ) || 
          (circID == redline.getOppCID(dir))  ){

            train_positions[dir][train_len[dir]] = circID * cf; //reverts negative numbers back to positive
            train_len[dir]++;
            break;
          }

        }//end direction loop
      }//end loop through active trains
      

      for(uint8_t direction=0; direction<2; direction++){

        Serial.printf("All trains in test range for dir %d\n", direction);
        for(int t=0; t<train_len[direction]; t++){
          Serial.printf("%d, ",train_positions[direction][t]); /*Flawfinder: ignore */
        }
        Serial.println();

        if (redline.getLen(direction) < 2){
          Serial.printf("Setting initial state for dir %d\n", direction);
          redline.setInitialStations(train_positions[direction], train_len[direction], direction);
          Serial.println(redline.getState());
        }

        //Loop through stations waiting in a given direction, and see if one of the active trains has reached them yet.
        for(int8_t i=redline.getLen(direction)-1; i >= 0; i--){

          //Put target station in local circuit
          uint16_t station_circuit = redline.getWaitingStationCircuit(i, direction); //redline stores station indexes.

          //See if any of the current train positions are arriving, at, or just leaving destination station.
          //If so, update station state.
          for(int t=0; t<train_len[direction]; t++){

            if(train_positions[direction][t] > (station_circuit - 5) && train_positions[direction][t] < (station_circuit + 5) 
            && station_circuit != redline.getLastCID(direction) ){

              Serial.printf("Station Circuit: %d\n", station_circuit); /*Flawfinder: ignore */
              Serial.printf("Train Circuit:   %d\n", train_positions[direction][t]); /*Flawfinder: ignore */

              redline.arrived(i, direction);

              break;
            }
          }
        }//end station iteration loop

        checkEndOfLine(redline, train_positions[direction], train_len[direction], direction);

      }//end direction loop
      //update LEDs
      redline.updateLEDS();
    }//END HTTPCODE
  
    else {
      Serial.println("GET Request failed");
      Serial.println(httpCode);
    }
      
  }//END https.begin

  else {
    Serial.println("HTTPS connection failed");
  }

  delay(3000);
}//END LOOP()