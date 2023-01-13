/* File for working / test code (actively compiled and uploaded to Arduino).
 * Code from this file occasionally moved to "snipits" folder when substantive feature developed.
 * Logan Arkema, current date
*/

#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include "arduino_secrets.h"
#include "TrainLine.h"

/***********************************************/
/*             CONFIGURATION                   */
/***********************************************/

#define WAIT_SEC 20 //number of seconds to wait between requests to WMATA server (WMATA updates every ~20, per documentation)

//LED CONFIGURATIONS
//configurations for chained together WS2812B LEDS. Datasheet: https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf

#define LED_PIN  4 //GPIO pin sending data to 1st WS2812B LED
#define LED_COUNT 105 //total # LEDs
#define PWR_LED 104 //index of "Power" (should be last)
#define WIFI_LED 103 //indoex of "WiFi" (2nd to last)
#define WEB_LED 102 //index of "Web" (3rd to last)

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800); //object to control colors of all LEDs (i.e. a "strip" of WS2812Bs)

//WIFI CONFIGURATIONS
WiFiManager wifi_manager;
const char* wmata_fingerprint = "C5 14 29 8E E1 04 75 0C A3 B3 1C 9D BB 43 BA 13 A0 CA A0 F7"; //
String endpoint = "https://api.wmata.com/TrainPositions/TrainPositions?contentType=json";

//CONNECTION CONFIGURATIONS

//Define Wifi and HTTP client objects to initialize in setup and use in every loop
WiFiClientSecure client;
HTTPClient https;

//Define JSON Deserialization objects to initialize in setup and use in every loop
StaticJsonDocument<80> train_pos_filter; 
const uint16_t json_size = 8096; //space to allocate for data FROM WMATA API. Change to 3072 without Line Code, 2048 for just CircuitIDs


/***** TRAIN LINE CONFIGURATIONS *****/

/*
Each line is defined by the number of stations on the line, the color trains on that line should be as a WRGB 32-bit hex value,
And two arrays of circuit indecies that correlate with each station on that line (defined by WMATA).

stations_0 is for trains (generally) moving North / East and is Direction 1 in WMATA products.
stations_1 is for trains (generally) moving South / West and is Direction 2 in WMATA products.
The arrays are ordered IN THE DIRECTION THE TRAIN MOVES, 
so the first station circuit in one direction represents the same station as the last station circuit in the other direction

See misc_commands.sh for how each array was created from WMATA's information
*/

//Red Line
#define NUM_RD_STATIONS 27
#define RD_HEX_COLOR 0x00FF0000 
const uint16_t rstations_0[NUM_RD_STATIONS] = {7, 32, 53, 62, 80, 95, 109, 126, 133, 142, 154, 164, 179, 190, 203, 467, 477, 485, 496, 513, 527, 548, 571, 591, 611, 629, 652};
const uint16_t rstations_1[NUM_RD_STATIONS] = {868, 846, 828, 809, 785, 757, 731, 717, 700, 686, 677, 667, 661, 389, 378, 363, 356, 346, 336, 326, 309, 294, 278, 260, 251, 232, 210};

//Blue Line
#define NUM_BL_STATIONS 27
#define BL_HEX_COLOR 0x000000FF
const uint16_t bstations_0[NUM_BL_STATIONS] = {2604, 2634, 969, 976, 1010, 1024, 1036, 1052, 1070, 1092, 1105, 1117, 1126, 1135, 1384, 1393, 1400, 1406, 1418, 1424, 1436, 1443, 2420, 2434, 2449, 2469, 2487};
const uint16_t bstations_1[NUM_BL_STATIONS] = {2574, 2557, 2537, 2521, 2506, 1618, 1610, 1598, 1590, 1575, 1568, 1559, 1549, 1330, 1323, 1310, 1298, 1285, 1265, 1246, 1230, 1217, 1204, 1170, 1162, 2709, 2679};

//Orange line
#define NUM_OR_STATIONS 26
#define OR_HEX_COLOR 0x00FF8000
const uint16_t ostations_0[NUM_OR_STATIONS] = {2774, 2796, 2817, 2844, 2870, 2886, 2898, 2911, 1092, 1105, 1117, 1126, 1135, 1384, 1393, 1400, 1406, 1418, 1424, 1436, 1443, 1475, 1487, 1500, 1522, 1542};
const uint16_t ostations_1[NUM_OR_STATIONS] = {1711, 1692, 1670, 1657, 1643, 1618, 1610, 1598, 1590, 1575, 1568, 1559, 1549, 1330, 1323, 1310, 1298, 1285, 3061, 3048, 3037, 3023, 3001, 2976, 2954, 2933};

//Silver Line
#define NUM_SV_STATIONS 34
#define SV_HEX_COLOR 0x00808080
const uint16_t sstations_0[NUM_SV_STATIONS] = {3523, 3544, 3577, 3594, 3612, 3627, 3155, 3214, 3221, 3232, 3238, 2844, 2870, 2886, 2898, 2911, 1092, 1105, 1117, 1126, 1135, 1384, 1393, 1400, 1406, 1418, 1424, 1436, 1443, 2420, 2434, 2449, 2469, 2487};
const uint16_t sstations_1[NUM_SV_STATIONS] = {2574, 2557, 2537, 2521, 2506, 1618, 1610, 1598, 1590, 1575, 1568, 1559, 1549, 1330, 1323, 1310, 1298, 1285, 3061, 3048, 3037, 3023, 3001, 3377, 3370, 3359, 3352, 3290, 3741, 3724, 3706, 3689, 3657, 3637};

//Yellow Line
#define NUM_YL_STATIONS 21
#define YL_HEX_COLOR 0x00FFFF00             // !! PENTAGON & L'ENFANT STATION INDEXES AND BRIDGE CIRCUITS ARE HARDCODED IN setTrainState !!
const uint16_t ystations_0[NUM_YL_STATIONS] = {944, 955, 969, 976, 1010, 1024, 1036, 1052, 2231, 2241, 2246, 1753, 1764, 1773, 1782, 1796, 1809, 1833, 1850, 1871, 1894};
const uint16_t ystations_1[NUM_YL_STATIONS] = {2055, 2030, 2009, 1992, 1971, 1956, 1942, 1932, 1923, 1911, 1899, 2376, 2364, 1246, 1230, 1217, 1204, 1170, 1162, 1148, 1137};

//Green Line
#define NUM_GN_STATIONS 21
#define GN_HEX_COLOR 0x0000FF00
const uint16_t gstations_0[NUM_GN_STATIONS] = {2118, 2136, 2154, 2170, 2183, 2199, 2208, 2219, 2231, 2241, 2246, 1753, 1764, 1773, 1782, 1796, 1809, 1833, 1850, 1871, 1894};
const uint16_t gstations_1[NUM_GN_STATIONS] = {2055, 2030, 2009, 1992, 1971, 1956, 1942, 1932, 1923, 1911, 1899, 2376, 2364, 2352, 2342, 2333, 2317, 2303, 2291, 2272, 2255};

//LED arrays map each line's stations, in the same order as stations_0, to the index of that station in the continuous "string" of LEDs.
//See dctransistor.com/documentation for a reference diagram
const uint8_t rd_led_array[NUM_RD_STATIONS] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26};
const uint8_t bl_led_array[NUM_BL_STATIONS] = {101, 100, 97, 96, 94, 93, 92, 91, 90, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 52, 51, 50, 49, 48};
const uint8_t or_led_array[NUM_OR_STATIONS] = {87, 88, 89, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53};
const uint8_t sv_led_array[NUM_SV_STATIONS] = {86, 85, 84, 83, 82, 81, 80, 79, 78, 77, 76, 75, 74, 73, 72, 71, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 52, 51, 50, 49, 48};
const uint8_t yl_led_array[NUM_YL_STATIONS] = {99, 98, 97, 96, 94, 93, 92, 91, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27};
const uint8_t gn_led_array[NUM_GN_STATIONS] = {47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27};

//Create objects representing each line
TrainLine* redline = new TrainLine(NUM_RD_STATIONS, rstations_0, rstations_1, RD_HEX_COLOR, rd_led_array);
TrainLine* blueline = new TrainLine(NUM_BL_STATIONS, bstations_0, bstations_1, BL_HEX_COLOR, bl_led_array);
TrainLine* orangeline = new TrainLine(NUM_OR_STATIONS, ostations_0, ostations_1, OR_HEX_COLOR, or_led_array);
TrainLine* silverline = new TrainLine(NUM_SV_STATIONS, sstations_0, sstations_1, SV_HEX_COLOR, sv_led_array);
TrainLine* yellowline = new TrainLine(NUM_YL_STATIONS, ystations_0, ystations_1, YL_HEX_COLOR, yl_led_array);
TrainLine* greenline = new TrainLine(NUM_GN_STATIONS, gstations_0, gstations_1, GN_HEX_COLOR, gn_led_array);

//Create an array to hold all train lines to iterate through
const uint8_t num_lines = 6;
TrainLine* all_lines[num_lines] = {orangeline, silverline, blueline, yellowline, greenline, redline};

/***********************************************/
/*                SETUP CODE                   */
/***********************************************/

//SETUP WIFI CONNECTION
void setup() {
  Serial.begin(9600); //NodeMCU ESP8266 runs on 9600 baud rate. Use to send debugging messages

  //Set LED strip settings and turn power light on
  strip.begin();
  strip.setBrightness(10);
  strip.setPixelColor(PWR_LED,GN_HEX_COLOR);
  strip.show();

  //Connect to network defined in example_arduino_secrets.h
  strip.setPixelColor(WIFI_LED, YL_HEX_COLOR);
  strip.show();
  Serial.println("Connecting to WiFi");
  
  bool wifi_conn;
  wifi_conn = wifi_manager.autoConnect("DCTransistor");

  /*Wait for WiFi connection to establish, turn LED green once done
  while (WiFi.status() != WL_CONNECTED){
    delay(1000);
    strip.setPixelColor(WIFI_LED, YL_HEX_COLOR);
    strip.show();
    Serial.println("Connecting to WiFi...");
  }
  */
  if(wifi_conn){
    Serial.println("Wifi Connected");
    strip.setPixelColor(WIFI_LED, GN_HEX_COLOR);
    strip.show();
  }

  //Set HTTPS connection settings to WMATA API
  client.setFingerprint(wmata_fingerprint);
  client.setTimeout(15000); //recommended default
  https.useHTTP10(true); //enables more efficient Json deserialization per https://arduinojson.org/v6/how-to/use-arduinojson-with-httpclient/

  //Set options to filter WMATA API data to only include the current circuit, direction, and line for every train currently running
  JsonObject tmp_filter = train_pos_filter["TrainPositions"].createNestedObject();
  tmp_filter["CircuitId"] = true;
  tmp_filter["DirectionNum"] = true;
  tmp_filter["LineCode"] = true;
  
  //Leave setup and turn Web led yellow
  Serial.println("Leaving setup");
  strip.setPixelColor(WEB_LED, YL_HEX_COLOR);
  strip.show();

}//END SETUP

/***********************************************/
/*                LOOP  CODE                   */
/***********************************************/

// the loop function runs over and over again forever
void loop() {

  Serial.println("---- NEW LOOP ----");

  //Connect and confirm HTTPS connection to api.wmata.com
  if (https.begin(client, endpoint)) {

    //If successful, add API key and request station data
    https.addHeader("api_key", SECRET_WMATA_API_KEY);
    int httpCode = https.GET();

    //If successful HTTP request, deserialize the JSON data returned by the API
    if (httpCode > 0) {

      //Filter to only relevant data (set in setup function)
      DynamicJsonDocument doc(json_size);
      DeserializationError error = deserializeJson(doc, https.getStream(), DeserializationOption::Filter(train_pos_filter));

      //Check that JSON Deserialization didn't fail. If so, print errors and return.
      if (error) {
        strip.setPixelColor(WEB_LED, RD_HEX_COLOR);
        strip.show();

        Serial.println("JSON Deserialization Failed:");
        Serial.println(error.f_str());

        return;
      }

      //If no error, set Web pixel to green and begin iterating through each train
      strip.setPixelColor(WEB_LED, GN_HEX_COLOR);
      strip.show();

      Serial.println("Begin loop through trains");

      //For each train, determine if it is on a line (WMATA returns some that are not active), determine which line it is on,
      //then pass it to that line to determine what station the train is at or in-between
      for(JsonObject train : doc["TrainPositions"].as<JsonArray>()){ //from https://arduinojson.org/v6/api/jsonarray/begin_end/
      
        //Only work on trains that are on a line. JsonObject removes key if value is null.
        if(train["LineCode"]){

          //Isolate variables from JsonObject returned by API
          const uint16_t circID = train["CircuitId"].as<unsigned int>();
          const uint8_t train_dir = train["DirectionNum"].as<unsigned short>();
          const char* train_line = train["LineCode"];

          const char line_char = train_line[0]; //get first character of line, as switch statements work on chars but not strings.

          int res = 0; //store result of setting each train

          Serial.printf("Line: %s, Direction: %d, Circuit: %d, ", train_line, train_dir, circID); //continued after station determined

          //Update state for whichever line train is on.
          switch (line_char)
          {
            case 'R':
              res = redline->setTrainState(circID, train_dir-1);
              break;
            case 'B':
              res = blueline->setTrainState(circID, train_dir-1);
              break;
            case 'O':
              res = orangeline->setTrainState(circID, train_dir-1);
              break;
            case 'S':
              res = silverline->setTrainState(circID, train_dir-1);
              break;
            case 'Y':
              res = yellowline->setTrainState(circID, train_dir-1);
              break;
            case 'G':
              res = greenline->setTrainState(circID, train_dir-1);
              break;
            default:
              res = -1;
              break;
          }//end switch statement

          Serial.printf("Station Index: %d\n", res); //Finish debugging / output info

        }//end if train is on a line
      } //end loop through active trains

      //Trains at the end of each line are handled differently (to avoid lingering LEDs).
      //Check each line's last station and set the LED as appropriate.
      for(uint8_t l=0; l < num_lines; l++){
        all_lines[l]->setEndLED();
      }
      
      //For each LED, check if there is a train at the station represented by that LED.
      //If so, turn the station's LED that line's color. If no trains, turn the LED off.

      //"Collisions" with trains on different lines "at" the same station are determined by
      //the order lines are put into the all_lines array in the configuration section of this file.

      for(uint8_t k=0; k<strip.numPixels()-3; k++){ //do not turn off board status LEDs at end of "strip."
        bool ledOn = false;

        for(uint8_t l=0; l<num_lines; l++){
          if(all_lines[l]->trainAtLED(k)){
            ledOn = true;
            strip.setPixelColor(k, all_lines[l]->getLEDColor());
            break;
          }
        }//end check for every line on a single LED

        //If no trains at station, turn LED off
        if(!ledOn){
          strip.setPixelColor(k, 0); //turn LED off
        }

      }//end loop through each LED

      //Update the board with new state of the system
      strip.show();

      //Clear each line's internal state. Reset to reflect the data in a single API call.
      for(uint8_t l=0; l < num_lines; l++){
        all_lines[l]->clearState();
      }
      
    }//END HTTPCODE
  
    //If HTTP GET returns -1, print error and turn wEB LED red.
    else {
      strip.setPixelColor(WEB_LED, RD_HEX_COLOR);
      strip.show();

      Serial.println("GET Request failed");
      Serial.println(httpCode);
    }
      
  }//END https.begin

  //If unsuccessful connection to api.wmata.com, print error and turn wEB LED red.
  else {
    strip.setPixelColor(WEB_LED, RD_HEX_COLOR);
    strip.show();

    Serial.println("HTTPS connection failed");
  }

  delay(WAIT_SEC * 1000); //wait set number of seconds until next loop and API call.
}//END LOOP()
