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
HTTPClient https;

uint8_t data_failure_count; //Count failures getting live data
uint32_t total_run_count; //Count total iterations of run time

int16_t special_train_id = -1; //Constant to get the TrainID of a special train. If no special train, -1

//Define JSON Deserialization objects to initialize in setup and use in every loop
StaticJsonDocument<JSON_FILTER_SIZE> train_pos_filter; 
const uint16_t json_size = JSON_DOC_SIZE; //space to allocate for data FROM WMATA API. Change to 3072 without Line Code, 2048 for just CircuitIDs

//Array of different API keys associated with different "default tier" products on the WMATA developer page.
String wmata_api_keys[3] = {SECRET_WMATA_API_KEY_0, SECRET_WMATA_API_KEY_1, SECRET_WMATA_API_KEY_2};

//Create objects representing each line
TrainLine* redline = new TrainLine(NUM_RD_STATIONS, rstations_0, rstations_1, "RD", RD_HEX_COLOR, rd_led_array);
TrainLine* blueline = new TrainLine(NUM_BL_STATIONS, bstations_0, bstations_1, "BL", BL_HEX_COLOR, bl_led_array);
TrainLine* orangeline = new TrainLine(NUM_OR_STATIONS, ostations_0, ostations_1, "OR", OR_HEX_COLOR, or_led_array);
TrainLine* silverline = new TrainLine(NUM_SV_STATIONS, sstations_0, sstations_1, "SV", SV_HEX_COLOR, sv_led_array);
TrainLine* yellowline = new TrainLine(NUM_YL_STATIONS, ystations_0, ystations_1, "YL", YL_HEX_COLOR, yl_led_array);
TrainLine* greenline = new TrainLine(NUM_GN_STATIONS, gstations_0, gstations_1, "GR", GN_HEX_COLOR, gn_led_array);

//Create an array to hold all train lines to iterate through
TrainLine* all_lines[NUM_LINES] = {orangeline, silverline, blueline, yellowline, greenline, redline};

/***********************************************/
/*                SETUP CODE                   */
/***********************************************/

//SETUP WIFI CONNECTION
void setup() {

  #ifdef PRINT
    Serial.begin(BAUD_RATE); //NodeMCU ESP8266 runs on 9600 baud rate. Defined in config.
  #endif

  //Set LED strip settings, turn power light on and Wifi to Yellow.
  strip.begin();
  strip.setBrightness(LED_BRIGHTNESS);
  strip.setPixelColor(PWR_LED,GN_HEX_COLOR);
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

  //Check if there is a special campaign, and retrieve its TrainID if so (or -1 if no special)
  special_train_id = check_for_special_train(client);

  // Set Yello Web LED while checking for special trains
  strip.setPixelColor(WEB_LED, YL_HEX_COLOR);
  strip.show();

  //Set HTTPS connection settings in prep for main loop WMATA API
  client.setTimeout(15000); //recommended default
  client.flush();
  client.stopAll();
  https.useHTTP10(true); //enables more efficient Json deserialization per https://arduinojson.org/v6/how-to/use-arduinojson-with-httpclient/

  //Initialize mutli-loop counters
  data_failure_count = 0;
  total_run_count = 0;

  //Set options to filter WMATA API data to only include the current circuit, direction, and line for every train currently running
  // When loading entire Json document at once, filter must account for array, i.e. ["TrainPositions"][0]["CircuitId"]
  train_pos_filter["CircuitId"] = true;
  train_pos_filter["DirectionNum"] = true;
  train_pos_filter["LineCode"] = true;

  if(special_train_id != -1){
    train_pos_filter["TrainId"] = true;
  }
  
  //Leave setup and turn Web led yellow
  #ifdef PRINT
    Serial.println("Leaving setup");
  #endif

}//END SETUP

/***********************************************/
/*                LOOP  CODE                   */
/***********************************************/

// the loop function runs over and over again forever
void loop() {

  #ifdef PRINT
    Serial.println("---- NEW LOOP ----");
  #endif

  bool getting_live_trains = true;

  //check_for_special_train(client);


  // TEST CODE

  // int64_t ret = 0;
  // if (CIRC_COUNT > -1 ){
  //   silverline->setTrainState(SV_LINE_CIRCUITS_1[CIRC_COUNT], 1); 
  //   CIRC_COUNT -= 1;
  // }
 
  // if (TEST_COUNT < CIRC_COUNT ){
  //   ret = silverline->setTrainState(SV_LINE_CIRCUITS_0[TEST_COUNT], 0);
  //   TEST_COUNT += 1;
  // }

  // Serial.printf("Circuit: %d;  Ret: %d\n", SV_LINE_CIRCUITS_1[CIRC_COUNT], ret);


  //Connect and confirm HTTPS connection to api.wmata.com. If not, set LED red.
  client.setFingerprint(API_WMATA_COM_FINGERPRINT);
  if(!https.begin(client, WMATA_ENDPOINT)){
    strip.setPixelColor(WEB_LED, RD_HEX_COLOR);
    strip.show();
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

    getting_live_trains = false;

    #ifdef PRINT
      Serial.println("GET Request failed");
      Serial.println(httpCode);
    #endif
  }


  //large scoped vars to track the presence of special trains
  uint8_t special_train_index = 0;
  TrainLine* special_train_line = NULL;

  //counts for active trains across all lines
  uint8_t countfail=0;
  uint8_t total_count=0;

  //Use chunk filtering to only deserialize one train object at a time
  // https://arduinojson.org/v6/how-to/deserialize-a-very-large-document/
  uint16_t json_size = JSON_DOC_SIZE;
  DynamicJsonDocument doc(json_size);

  //Only load each train object into a JSON document at a time to preserve RAM by iterating through TCP stream.
  //WifiClient is actual consistent source of https stream. If can't find, create error. 
  if(!client.find("\"TrainPositions\":[")){
    strip.setPixelColor(WEB_LED, RD_HEX_COLOR);
    strip.show();
    getting_live_trains = false;

    #ifdef PRINT
      Serial.println("Unable to find '\"TrainPositions\":[' in HTTP response.");
    #endif
  };

  #ifdef PRINT
    Serial.println("Begin loop through trains");
  #endif

  do {

    DeserializationError error = deserializeJson(doc, client, DeserializationOption::Filter(train_pos_filter));

    if (error) {

      strip.setPixelColor(WEB_LED, RD_HEX_COLOR);
      strip.show();
      getting_live_trains = false;

      #ifdef PRINT
        Serial.printf("JSON Deserialization Failed: %s\n", error.f_str());
        Serial.printf("Intended JSON Document Size: %d\n", json_size);
        Serial.printf("JSON Doc Size: %d\n", doc.size());
        Serial.printf("Actual JSON Document Size: %d\n", doc.capacity());
        Serial.printf("HTTP Response Code %d\n", httpCode);
        Serial.printf("HTTP Body Size: %d\n", https.getSize());
        Serial.printf("Overflowed? %s\n", doc.overflowed());
      #endif
    }

    //Only work on trains that are on a line. JsonObject removes key if value is null.
    else if(doc["LineCode"]){

      //Isolate variables from JsonObject returned by API
      const uint16_t circID = doc["CircuitId"].as<unsigned int>();
      const uint8_t train_dir = doc["DirectionNum"].as<unsigned short>();
      const char* train_line = doc["LineCode"].as<const char*>();
      int16_t trainID = -1;

      if (special_train_id != -1){
        trainID = doc["TrainId"].as<int16_t>();
      }

      // Current TrainLocation tracking variables
      const char line_char = train_line[0]; //get first character of line, as switch statements work on chars but not strings.
      int res = -1; //store result of setting each train
      TrainLine* cur_train_line = NULL; //store which TrainLine object has current line

      #ifdef PRINT
        Serial.printf("Line: %s, Direction: %d, Circuit: %d, ", train_line, train_dir, circID); //continued after station determined
      #endif

      // Find the line the train is on by color, and update that line with the train.
      for (uint8_t i=0; i<NUM_LINES; i++){
        if (strcmp(all_lines[i]->getColor(), train_line) == 0){
          cur_train_line = all_lines[i];
          res = cur_train_line->setTrainState(circID, train_dir-1);
          break;
        }
      }

      //If current line not set among all lines, update failure count
      if (cur_train_line == NULL){
        countfail++;
      }

      #ifdef PRINT
        Serial.printf("Station Index: %d\n", res); //Finish debugging / output info
      #endif

      // Check for special train. If special train is -1, ensure it fails.
      if( special_train_id != -1 && special_train_id == trainID){

        special_train_index = res;
        special_train_line = cur_train_line;

        #ifdef PRINT
          Serial.printf("Setting Special Train on line: %c index: %d\n", line_char, res);
        #endif
      }

    }//end if train is on a line

    doc.clear();

  } while (client.findUntil("," , "]")); //Iterate through end of Json Array.

  https.end();

  // Get total output by adding trains, and print totals if printing debug output
  for (uint8_t i=0; i<NUM_LINES; i++){
    total_count+=all_lines[i]->getTrainCount();
    #ifdef PRINT
      Serial.printf("%s Count: %d\n", all_lines[i]->getColor(), all_lines[i]->getTrainCount());
    #endif
  }

  #ifdef PRINT
    Serial.printf("Total Count: %d\n", total_count);
    Serial.printf("Fail Count: %d\n", countfail);
  #endif
 
  // If WMATA API returns empty array, show failure
  if (total_count == 0){ //Was `doc["TrainPositions"].size()` when loading entire doc at once
    strip.setPixelColor(WEB_LED, RD_HEX_COLOR);
    strip.show();
    getting_live_trains = false;

    #ifdef PRINT
      Serial.println("Empty Train Positions Array");
    #endif
  }

  //If no error, set Web pixel to green and reset data failure count.
  if (getting_live_trains){
    strip.setPixelColor(WEB_LED, GN_HEX_COLOR);
    strip.show();
    data_failure_count = 0;
  }

  // Pattern display to default to if there is an error with live data
  else {

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

    data_failure_count++;

  } //end shift display for whenable to get live data

  //Trains at the end of each line are handled differently (to avoid lingering LEDs).
  //Check each line's last station and set the LED as appropriate.
  for(uint8_t l=0; l < NUM_LINES; l++){
    all_lines[l]->setEndLED();
  }
  // } // END live train data

  //For each LED, check if there is a train at the station represented by that LED.
  //If so, turn the station's LED that line's color. If no trains, turn the LED off.

  //"Collisions" with trains on different lines "at" the same station are determined by
  //the order lines are put into the all_lines array in the configuration section of this file.

  #ifdef PRINT
    Serial.printf("Setting Strip LEDs\n");
  #endif
  
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
  if(special_train_id != -1){

    #ifdef PRINT
      Serial.printf("Setting special train LED\n");
      Serial.printf("Train ID: %d;   Train Index: %d;\n", special_train_id, special_train_index);
    #endif

    if(special_train_line != NULL){
      uint8_t special_led = special_train_line->getLEDForIndex(special_train_index);
      strip.setPixelColor(special_led, SPECIAL_TRAIN_HEX[0]);
    }
  }

  #ifdef PRINT
    Serial.printf("Updating Strip with new State\n");
  #endif

  //Update the board with new state of the system
  strip.show();

  // If there is a special train with multiple colors (pride), make it strobe.
  bool strobe = false;
  if(special_train_line != NULL && SPECIAL_TRAIN_HEX_COUNT > 1){
    strobe = true;
    uint delay_count = 0;
    while(delay_count < (WAIT_SEC * 2)){
      delay(1000);
      uint8_t special_led = special_train_line->getLEDForIndex(special_train_index);
      strip.setPixelColor(special_led, SPECIAL_TRAIN_HEX[delay_count % SPECIAL_TRAIN_HEX_COUNT]);
      strip.show();
      delay_count++;
    }
  }

  //Clear each line's internal state. Reset to reflect the data in a single API call.
  if(getting_live_trains){
    for(uint8_t l=0; l < NUM_LINES; l++){
      all_lines[l]->clearState();
    }
  }

  //Update overall run counts and do time-based checks for special trains and board update. Count should never actually be when it reaches checks.
  total_run_count++;

  if (AUTOUPDATE && total_run_count % ((UPDATE_CHECK_HOURS * 3600) / WAIT_SEC) == 0){
    check_for_update(client);
  }

  if ( total_run_count % ((SPECIAL_TRAIN_CHECK_HOURS * 3600) / WAIT_SEC) == 0){
    special_train_id = check_for_special_train(client);
  }

  if (total_run_count == (48 * 3600) / WAIT_SEC){
    total_run_count = 0;
  }

  #ifdef PRINT
    Serial.printf("End of loop\n");
  #endif
    
  //wait set number of seconds (default 20) until next loop and API call. If strobing, waiting done already
  if (!strobe){
    delay(WAIT_SEC * 1000);
  }

}//END LOOP()
