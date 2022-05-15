/* File for working / test code (actively compiled and uploaded to Arduino).
 * Code from this file occasionally moved to "snipits" folder when substantive feature developed.
 * Logan Arkema, current date
*/


#include <ESP8266WiFi.h>
//#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include "arduino_secrets.h"
#include "miscFuncs.h" 

#define LED_PIN  4
#define LED_COUNT 40

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

//Connection (WiFi and API endpoint) configuration
String ssid = SECRET_SSID;
String password = SECRET_PASS; 
const char* wmata_fingerprint = "D2 1C A6 D1 BE 10 18 B3 74 8D A2 F5 A2 DE AB 13 7D 07 63 BE"; //Expires 10/22/2022 
String endpoint = "https://api.wmata.com/TrainPositions/TrainPositions?contentType=json";

//Red Line Configuration
TrainLine* redline;
#define NUM_RD_STATIONS 27
#define RD_HEX_COLOR 0x00FF0000
const uint16_t rstations_0[NUM_RD_STATIONS] = {7, 32, 53, 62, 80, 95, 109, 126, 133, 142, 154, 164, 179, 190, 203, 467, 477, 485, 496, 513, 527, 548, 571, 591, 611, 629, 652};
const uint16_t rstations_1[NUM_RD_STATIONS] = {868, 846, 828, 809, 785, 757, 731, 717, 700, 686, 677, 667, 661, 389, 378, 363, 356, 346, 336, 326, 309, 294, 278, 260, 251, 232, 210};
const uint8_t rd_led_array[NUM_RD_STATIONS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26};

//Blue Line Configuration
TrainLine* blueline;
#define NUM_BL_STATIONS 25
#define BL_HEX_COLOR 0x000000FF
const uint16_t bstations_0[NUM_BL_STATIONS] = {969, 976, 1010, 1024, 1036, 1052, 1070, 1092, 1105, 1117, 1126, 1135, 1384, 1393, 1400, 1406, 1418, 1424, 1436, 1443, 2420, 2434, 2449, 2469, 2487};
const uint16_t bstations_1[NUM_BL_STATIONS] = {2574, 2557, 2537, 2521, 2506, 1618, 1610, 1598, 1590, 1575, 1568, 1559, 1549, 1330, 1323, 1310, 1298, 1285, 1265, 1246, 1230, 1217, 1204, 1170, 1162};

//Orange line configuration
TrainLine* orangeline;
#define NUM_OR_STATIONS 18
#define OR_HEX_COLOR 0x00FF4000
const uint16_t ostations_0[NUM_OR_STATIONS] = {1092, 1105, 1117, 1126, 1135, 1384, 1393, 1400, 1406, 1418, 1424, 1436, 1443, 1475, 1487, 1500, 1522, 1542};
const uint16_t ostations_1[NUM_OR_STATIONS] = {1711, 1692, 1670, 1657, 1643, 1618, 1610, 1598, 1590, 1575, 1568, 1559, 1549, 1330, 1323, 1310, 1298, 1285};

//Silver Line Configuration
TrainLine* silverline;
#define NUM_SV_STATIONS 18
#define SV_HEX_COLOR 0x00808080
const uint16_t sstations_0[NUM_SV_STATIONS] = {1092, 1105, 1117, 1126, 1135, 1384, 1393, 1400, 1406, 1418, 1424, 1436, 1443, 2420, 2434, 2449, 2469, 2487};
const uint16_t sstations_1[NUM_SV_STATIONS] = {2574, 2557, 2537, 2521, 2506, 1618, 1610, 1598, 1590, 1575, 1568, 1559, 1549, 1330, 1323, 1310, 1298, 1285};

//CHANGE LED ARRAYS for Orange / Blue
const uint8_t bl_led_array[NUM_BL_STATIONS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
const uint8_t or_led_array[NUM_OR_STATIONS] = {7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 27, 28, 29, 30, 31};
const uint8_t sv_led_array[NUM_SV_STATIONS] = {7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};

/*
Adafruit_LEDBackpack matrix = Adafruit_LEDBackpack();
const uint8_t addr = 0x70;
*/

//Define client responsible for managing secure connection api.wmata.com
WiFiClientSecure client;
HTTPClient https;


//Filters data from TrainPositions API to just get CircuitIds with Train
StaticJsonDocument<80> train_pos_filter; 
const uint16_t json_size = 4096; //change to 3072 if add DirectionNum, 2048 for just CircuitIDs, 4096 for line code

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

  //Set HTTPS connection settings
  client.setFingerprint(wmata_fingerprint);
  client.setTimeout(15000);

  //Set LED strip settings
  strip.begin();
  strip.show();
  strip.setBrightness(50);

  
  redline = new TrainLine(NUM_RD_STATIONS, rstations_0, rstations_1, RD_HEX_COLOR, rd_led_array);
  blueline = new TrainLine(NUM_BL_STATIONS, bstations_0, bstations_1, BL_HEX_COLOR, bl_led_array);
  orangeline = new TrainLine(NUM_OR_STATIONS, ostations_0, ostations_1, OR_HEX_COLOR, or_led_array);
  silverline = new TrainLine(NUM_SV_STATIONS, sstations_0, sstations_1, SV_HEX_COLOR, sv_led_array);

  https.useHTTP10(true); //enables more efficient Json deserialization per https://arduinojson.org/v6/how-to/use-arduinojson-with-httpclient/

  //Only store trains' current circuit, direction, and line from WMATA API
  JsonObject tmp_filter = train_pos_filter["TrainPositions"].createNestedObject();
  tmp_filter["CircuitId"] = true;
  tmp_filter["DirectionNum"] = true;
  tmp_filter["LineCode"] = true;
  
  Serial.println("Leaving setup");
}//END SETUP

// the loop function runs over and over again forever
void loop() {

  Serial.println("---- NEW LOOP ----");
  //Serial.println(redline.getState());

  //Connect and confirm HTTPS connection to api.wmata.com
  if (https.begin(client, endpoint)) {
    //If successful, add API key and request station data
    https.addHeader("api_key", SECRET_WMATA_API_KEY);
    int httpCode = https.GET();

    //Print out HTTP code and, if successful, parse data returned from API and update staion state
    if (httpCode > 0) {

      DynamicJsonDocument doc(json_size);
      DeserializationError error = deserializeJson(doc, https.getStream(), DeserializationOption::Filter(train_pos_filter));

      //serializeJsonPretty(doc, Serial); //print out all outputs from deserialization
    

      //Iterate through all returned train positions, put the ones on target section of redline track into an array.

      //uint16_t train_positions[2][10] = {};
      //uint8_t train_len[2] = {0,0};
      //uint16_t array_size = doc["TrainPositions"].size();
      //uint16_t j = 0;

      //JsonArray arr = doc["TrainPositions"].as<JsonArray>();

      Serial.println("Begin loop through trains");

      for(JsonObject train : doc["TrainPositions"].as<JsonArray>()){ //from https://arduinojson.org/v6/api/jsonarray/begin_end/
      
        //Only work on trains that are on a line. JsonObject removes key if value is null.
        if(train["LineCode"]){

          //Isolate variables from JsonObject returned by API
          const uint16_t circID = train["CircuitId"].as<unsigned int>();
          const uint8_t train_dir = train["DirectionNum"].as<unsigned short>();
          const char* train_line = train["LineCode"];

          const char line_char = train_line[0]; //get first character of line, as switch statements work on chars but not strings.

          //Update state for whichever line train is on.
          switch (line_char)
          {
            case 'R':
              //Serial.printf("Line: %s, Direction: %d, Circuit: %d\n", train_line, train_dir, circID);
             // redline->setTrainState(circID, train_dir-1);
              break;
            case 'B':
              Serial.printf("Line: %s, Direction: %d, Circuit: %d\n", train_line, train_dir, circID);
              blueline->setTrainState(circID, train_dir-1);
              break;
            case 'O':
              Serial.printf("Line: %s, Direction: %d, Circuit: %d\n", train_line, train_dir, circID);
              orangeline->setTrainState(circID, train_dir-1);
              break;
            case 'S':
              Serial.printf("Line: %s, Direction: %d, Circuit: %d\n", train_line, train_dir, circID);
              silverline->setTrainState(circID, train_dir-1);

            default:
              break;

          }//end switch statement
        }//end if train is on a line
      } //end loop through active trains
      
      //redline->updateLEDS3(strip);
      //blueline->updateLEDS3(strip);

      const uint8_t num_lines = 3;
      TrainLine* all_lines[num_lines] = {orangeline, silverline, blueline};

      for(uint8_t l=0; l < num_lines; l++){
        all_lines[l]->setEndLED();
      }
      

      //Loop through each led to represent state
      for(uint8_t k=0; k<strip.numPixels(); k++){
        bool ledOn = false;

        //If train is at station represented by led for any line, set led to line's color.
        for(uint8_t l=0; l<num_lines; l++){
          if(all_lines[l]->trainAtLED(k)){
            ledOn = true;
            strip.setPixelColor(k, all_lines[l]->getLEDColor());
            break;
          }
        }

        //If no trains at station, turn led off
        if(!ledOn){
          strip.setPixelColor(k, 0); //turn led off
        }


      }//end loop through each led

      strip.show();


      for(uint8_t l=0; l < num_lines; l++){
        all_lines[l]->clearState();
      }


      //code that looped through trains on a line and determined station location removed. Now done in TrainLine
      
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