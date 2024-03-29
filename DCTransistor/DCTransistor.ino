/* Main file for running DCTransistor program. 

 * Configuration values defined in config.h
 * Class for representing state of each WMATA line defined in TrainLine.h
 * Functions to auto-update software in auto_update.h
 * 
 * This file sets up WiFi and access to WMATA API, 
 * then loops through calls to WMATA API for current train positions,
 * uses returned values to convert positions into nearest station LED,
 * then updates LEDs with new position / state data.
 * 
 * Code from this file occasionally moved to "snipits" folder when substantive feature developed.
 * (c) Logan Arkema, 1/7/2024
*/

#include "TrainLine.h"

//Global object variables
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800); //object to control colors of all LEDs (i.e. a "strip" of WS2812Bs)
WiFiManager wifi_manager; //WiFi manager to auto-connect to wifi
WiFiClientSecure client; //One client used to connect to all webservers. HTTP client defined in auto_update.h
uint8_t data_failure_count; //Count API failures / empty responses

//Define JSON Deserialization objects to initialize in setup and use in every loop
StaticJsonDocument<JSON_FILTER_SIZE> train_pos_filter; 
const uint16_t json_size = JSON_DOC_SIZE; //space to allocate for data FROM WMATA API. Change to 3072 without Line Code, 2048 for just CircuitIDs

//Array of different API keys associated with different "default tier" products on the WMATA developer page.
String wmata_api_keys[3] = {SECRET_WMATA_API_KEY_0, SECRET_WMATA_API_KEY_1, SECRET_WMATA_API_KEY_2};

//Create objects representing each line
TrainLine* redline = new TrainLine(NUM_RD_STATIONS, rstations_0, rstations_1, RD_HEX_COLOR, rd_led_array);
TrainLine* blueline = new TrainLine(NUM_BL_STATIONS, bstations_0, bstations_1, BL_HEX_COLOR, bl_led_array);
TrainLine* orangeline = new TrainLine(NUM_OR_STATIONS, ostations_0, ostations_1, OR_HEX_COLOR, or_led_array);
TrainLine* silverline = new TrainLine(NUM_SV_STATIONS, sstations_0, sstations_1, SV_HEX_COLOR, sv_led_array);
TrainLine* yellowline = new TrainLine(NUM_YL_STATIONS, ystations_0, ystations_1, YL_HEX_COLOR, yl_led_array);
TrainLine* greenline = new TrainLine(NUM_GN_STATIONS, gstations_0, gstations_1, GN_HEX_COLOR, gn_led_array);

//Create an array to hold all train lines to iterate through
TrainLine* all_lines[NUM_LINES] = {orangeline, silverline, blueline, yellowline, greenline, redline};

/***********************************************/
/*                SETUP CODE                   */
/***********************************************/

//SETUP WIFI CONNECTION
void setup() {

  //#ifdef PRINT
  Serial.begin(BAUD_RATE); //NodeMCU ESP8266 runs on 9600 baud rate. Defined in config.
  //#endif

  //Set LED strip settings and turn power light on
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.setPixelColor(PWR_LED,GN_HEX_COLOR);
  strip.show();

  //Connect to network configured by user throug WiFi Manager
  strip.setPixelColor(WIFI_LED, YL_HEX_COLOR);
  strip.show();

  #ifdef PRINT
    Serial.println("Connecting to WiFi");
  #endif

  bool wifi_conn;
  wifi_conn = wifi_manager.autoConnect(WIFI_NAME, WIFI_PASSWORD);

  if(wifi_conn){
    #ifdef PRINT
      Serial.println("Wifi Connected");
    #endif
    strip.setPixelColor(WIFI_LED, GN_HEX_COLOR);
    strip.show();
  }

  //After connecting to WiFi, check for software update and download if possible
  if(AUTOUPDATE){
    strip.setPixelColor(WEB_LED, BL_HEX_COLOR);
    strip.show();
    check_for_update(client);
  }

  //Set HTTPS connection settings to WMATA API
  client.setFingerprint(API_WMATA_COM_FINGERPRINT);
  client.setTimeout(15000); //recommended default
  https.useHTTP10(true); //enables more efficient Json deserialization per https://arduinojson.org/v6/how-to/use-arduinojson-with-httpclient/
  data_failure_count = 0;

  //Set options to filter WMATA API data to only include the current circuit, direction, and line for every train currently running
  //JsonObject tmp_filter = train_pos_filter["TrainPositions"].createNestedObject();
  train_pos_filter["TrainPositions"][0]["CircuitId"] = true;
  train_pos_filter["TrainPositions"][0]["DirectionNum"] = true;
  train_pos_filter["TrainPositions"][0]["LineCode"] = true;

  if(SPECIAL_TRAIN){
    train_pos_filter["TrainPositions"][0]["TrainId"] = true;
  }
  
  //Leave setup and turn Web led yellow
  #ifdef PRINT
    Serial.println("Leaving setup");
  #endif
  strip.setPixelColor(WEB_LED, YL_HEX_COLOR);
  strip.show();

}//END SETUP

/***********************************************/
/*                LOOP  CODE                   */
/***********************************************/

// the loop function runs over and over again forever
void loop() {

  #ifdef PRINT
    Serial.println("---- NEW LOOP ----");
  #endif

  bool getting_live_trains = true; // Set boolean that can be set to false if any error condition met.

  //Connect and confirm HTTPS connection to api.wmata.com. If not, set LED red.
  if(!https.begin(client, WMATA_ENDPOINT)){
    strip.setPixelColor(WEB_LED, RD_HEX_COLOR);
    strip.show();

    data_failure_count++; //increment number of failures for programatic display
    getting_live_trains = false;

    #ifdef PRINT
      Serial.println("HTTPS connection failed");
    #endif
  }

  //Use one of three WMATA API Keys to stay under usage quota. Actuall randomness not important, just variance in key usage.
  https.addHeader("api_key", wmata_api_keys[random(3)]); /* Flawfinder: ignore */

  //Request train data from server. If unsuccessful, set LED red. If successful, deserialize the JSON data returned by the API
  int httpCode = https.GET();
  if (httpCode < 200 || httpCode >= 300) {
    strip.setPixelColor(WEB_LED, RD_HEX_COLOR);
    strip.show();

    data_failure_count++; //increment number of failures for programatic display
    getting_live_trains = false;

    #ifdef PRINT
      Serial.println("GET Request failed");
      Serial.println(httpCode);
    #endif
  } 

  //Filter to only relevant data (set in setup function)
  DynamicJsonDocument doc(json_size);
  DeserializationError error = deserializeJson(doc, https.getStream(), DeserializationOption::Filter(train_pos_filter));

  //Constants to track the presence of special trains
  uint16_t special_train_id = 0;
  uint8_t special_train_index = 0;
  TrainLine* special_train_line = NULL;

  //Check that JSON Deserialization didn't fail. If so, print errors and return.
  if (error) {
    strip.setPixelColor(WEB_LED, RD_HEX_COLOR);
    strip.show();

    data_failure_count++; //increment number of failures for programatic display
    getting_live_trains = false;

    #ifdef PRINT
      Serial.printf("JSON Deserialization Failed: %s\n", error.f_str());
      Serial.printf("Intended JSON Document Size: %d\n", json_size);
      Serial.printf("Actual JSON Document Size: %d\n", doc.capacity());
      Serial.printf("HTTP Body Size: %d\n", https.getSize());
      Serial.printf("Overflowed? %s\n", doc.overflowed());
    #endif
  }

  // If WMATA API returns empty array
  if (doc["TrainPositions"].size() == 0){
    strip.setPixelColor(WEB_LED, RD_HEX_COLOR);
    strip.show();
    data_failure_count++; //increment number of failures for programatic display
    getting_live_trains = false;

    #ifdef PRINT
      Serial.println("Empty Array");
    #endif
  }

  // If connection to live data has repeatedly failed and still failing, resort to generated display
  if (getting_live_trains == false){

    #ifdef PRINT
      Serial.printf("Failure Count: %d\n", data_failure_count);
    #endif

    // Put trains on the board in increments of 3 (one train every three cycles)
    uint8_t start_time = data_failure_count % 3;

    // Start blue line one ahead of others so it doesn't conflict with Yellow / Silver
    for (uint8_t l=0; l< NUM_LINES; l++){
      if (all_lines[l]->getLEDColor() == BL_HEX_COLOR){
        all_lines[l]->defaultShiftDisplay((start_time == 2));
      }
      else{
        all_lines[l]->defaultShiftDisplay( (start_time == 0));
      }
    }

    if (data_failure_count == 255) {data_failure_count = 3;} //reset to prevent overflow

  }

  // Normal operations if no errors
  else{

    //If no error, set Web pixel to green and begin iterating through each train
    strip.setPixelColor(WEB_LED, GN_HEX_COLOR);
    strip.show();
    data_failure_count = 0; //Reset data failure count on success.
    getting_live_trains = true;

    #ifdef PRINT
    Serial.println("Begin loop through trains");
    #endif

    //counts for trains on different lines
    uint8_t countr=0;
    uint8_t countb=0;
    uint8_t counto=0;
    uint8_t county=0;
    uint8_t counts=0;
    uint8_t countg=0;
    uint8_t countfail=0;



    //For each train, determine if it is on a line (WMATA returns some that are not active), determine which line it is on,
    //then pass it to that line to determine what station the train is at or in-between
    for(JsonObject train : doc["TrainPositions"].as<JsonArray>()){ //from https://arduinojson.org/v6/api/jsonarray/begin_end/
    
      //Only work on trains that are on a line. JsonObject removes key if value is null.
      if(train["LineCode"]){

        //Isolate variables from JsonObject returned by API
        const uint16_t circID = train["CircuitId"].as<unsigned int>();
        const uint8_t train_dir = train["DirectionNum"].as<unsigned short>();
        const char* train_line = train["LineCode"];
        uint16_t trainID = 0;

        if (SPECIAL_TRAIN){
          trainID = train["TrainId"].as<unsigned int>();
        }

        const char line_char = train_line[0]; //get first character of line, as switch statements work on chars but not strings.

        int res = 0; //store result of setting each train
        TrainLine* cur_train_line = NULL; //store which TrainLine object has current line

        #ifdef PRINT
          Serial.printf("Line: %s, Direction: %d, Circuit: %d, ", train_line, train_dir, circID); //continued after station determined
        #endif

        //Update state for whichever line train is on.
        switch (line_char)
        {
          case 'R':
            res = redline->setTrainState(circID, train_dir-1);
            cur_train_line = redline;
            countr++;
            break;
          case 'B':
            res = blueline->setTrainState(circID, train_dir-1);
            cur_train_line=blueline;
            countb++;
            break;
          case 'O':
            res = orangeline->setTrainState(circID, train_dir-1);
            cur_train_line=orangeline;
            counto++;
            break;
          case 'S':
            res = silverline->setTrainState(circID, train_dir-1);
            cur_train_line=silverline;
            counts++;
            break;
          case 'Y':
            res = yellowline->setTrainState(circID, train_dir-1);
            cur_train_line=yellowline;
            county++;
            break;
          case 'G':
            res = greenline->setTrainState(circID, train_dir-1);
            cur_train_line=greenline;
            countg++;
            break;
          default:
            res = -1;
            break;
        }//end switch statement

        #ifdef PRINT
          Serial.printf("Station Index: %d\n", res); //Finish debugging / output info
        #endif

        // If looking for special trains, make sure train is valid then check if current train is special train and set info appropriately
        if (SPECIAL_TRAIN == true){ 
          if (res != -1){
            for (uint8_t i=0; i < NUM_SPECIAL_TRAIN_IDS; i++){

              #ifdef PRINT
                Serial.printf("Checking trainID %d against special train ID %d\n", trainID, special_train_ids[i]);
              #endif

              if(special_train_ids[i] == trainID){

                #ifdef PRINT
                  Serial.printf("Setting Special Train on line: %c index: %d\n", line_char, res);
                #endif

                special_train_id = trainID;
                special_train_index = res;
                special_train_line = cur_train_line;
                break;
              }
            }
          }
        }// end special train detection logic

        if (res == -1){countfail++;}

      }//end if train is on a line
    } //end loop through active trains

    #ifdef PRINT
      Serial.printf("Red Count: %d\n", countr);
      Serial.printf("Blue Count: %d\n", countb);
      Serial.printf("Orange Count: %d\n", counto);
      Serial.printf("Silver Count: %d\n", counts);
      Serial.printf("Green Count: %d\n", countg);
      Serial.printf("Yellow Count: %d\n", county);
      Serial.printf("Fail Count: %d\n", countfail);
    #endif

    //Trains at the end of each line are handled differently (to avoid lingering LEDs).
    //Check each line's last station and set the LED as appropriate.
    for(uint8_t l=0; l < NUM_LINES; l++){
      all_lines[l]->setEndLED();
    }
  
  }//END normal operations loop

  //For each LED, check if there is a train at the station represented by that LED.
  //If so, turn the station's LED that line's color. If no trains, turn the LED off.

  //"Collisions" with trains on different lines "at" the same station are determined by
  //the order lines are put into the all_lines array in the configuration section of this file.
  
  for(uint8_t k=0; k<strip.numPixels()-3; k++){ //do not turn off board status LEDs at end of "strip."
    bool ledOn = false;

    for(uint8_t l=0; l<NUM_LINES; l++){
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

  //If setting special LED color for a special train, do so, assuming the train is active. 
  if(SPECIAL_TRAIN){

    #ifdef PRINT
      Serial.printf("Setting special train LED\n");
      Serial.printf("Train ID: %d;   Train Index: %d;\n", special_train_id, special_train_index);
    #endif

    if(special_train_line != NULL){
      uint8_t special_led = special_train_line->getLEDForIndex(special_train_index);
      strip.setPixelColor(special_led, SPECIAL_TRAIN_HEX);
    }
  }

  //Update the board with new state of the system
  strip.show();

  //Clear each line's internal state. Reset to reflect the data in a single API call.
  if (getting_live_trains){
    for(uint8_t l=0; l < NUM_LINES; l++){
      all_lines[l]->clearState();
    }
  }
    
  //wait set number of seconds (default 20) until next loop and API call.
  delay(WAIT_SEC * 1000);

}//END LOOP()
