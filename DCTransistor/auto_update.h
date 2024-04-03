#include "config.h"

//Download and update to current binary version on github
void update_arduino(WiFiClientSecure &client, String cur_version){

  client.setFingerprint(RAW_GITHUBUSERCONTENT_COM_FINGERPRINT);
  
  if(!client.connect(UPDATE_HOST, HTTPS_PORT)){
    client.setInsecure(); //Needed in case board has not turned on and downloaded binary with updated github fingerprints
    client.connect(UPDATE_HOST, HTTPS_PORT);
  }
  
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, UPDATE_BIN_URL);

  #ifdef PRINT
   switch (ret) {
      case HTTP_UPDATE_FAILED: Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str()); break;

      case HTTP_UPDATE_NO_UPDATES: Serial.println("HTTP_UPDATE_NO_UPDATES"); break;

      case HTTP_UPDATE_OK: Serial.println("HTTP_UPDATE_OK"); break;
    } 
  #endif

}//END update_arduino

//Check GitHub releases for most recent version, compared to value in current software, and update if different.
void check_for_update(WiFiClientSecure &client){

  HTTPClient https;

  //Send HTTP request to /releases/latest on GitHub page, which always returns a 302
  client.setFingerprint(GITHUB_COM_FINGERPRINT);
  if(!client.connect(GITHUB_HOST, HTTPS_PORT)){
    client.setInsecure(); //Needed in case board has not turned on and downloaded binary with updated github fingerprints
    client.connect(GITHUB_HOST, HTTPS_PORT);
  }

  https.begin(client, LATEST_VERSION_URL);
  https.collectHeaders(github_header_keys, github_num_headers); //collect location header (defined in config)
  https.GET();

  //Get version based /releases/latest redirecting to /releases/tag/<cur_version>
  String release_loc = https.header("location");
  int version_index = release_loc.lastIndexOf("/")+1;
  String latest_version = release_loc.substring(version_index);

  #ifdef PRINT
    Serial.printf("Latest release URL: %d\n", release_loc);
    Serial.printf("Latest version: %s\n", latest_version);
    Serial.printf("Current version: %s\n", VERSION);
  #endif

  //If version doesn't match software's hardcoded version string, trigger update function.
  if (latest_version != VERSION){
      #ifdef PRINT
        Serial.println("Versions don't match. Updating");
      #endif
      update_arduino(client, latest_version);
  }

  //https.end();
}//END check_for_update

// Use GIS Services Train Location API to get Epoch Time (technically Epoch of most recent position update on first train object). Returns 0 on error.
time_t get_todays_date(WiFiClientSecure &client){


  HTTPClient https;
  https.useHTTP10(true);

  #ifdef PRINT
    Serial.println("GETTING TODAY'S DATE");
  #endif

  //Get today's date based on DATE_TIME values returned in train positions API
  client.setFingerprint(GISSERVICES_WMATA_COM_FINGERPRINT);

  if(!https.begin(client, GIS_TRAIN_LOC_ENDPOINT)){
    #ifdef PRINT
      Serial.printf("Unable to connect to WMATA Train Locations Endpoint\n");
    #endif
  }

  https.begin(client, GIS_TRAIN_LOC_ENDPOINT);
  int response = https.GET();

  //HTTP Response Error Handling
  if(response < 200 || response >= 400){
    #ifdef PRINT
        Serial.printf("HTTP Response Code %d\n", response);
        Serial.printf("HTTP Body Size: %d\n", https.getSize());
    #endif
    return 0;
  }

  //First four "ETIME" strings are in document headers. Fifth gets to first acutal EPOCH Time reference.
  for(uint8_t i=0; i<5; i++){
    client.find("\"ETIME\":");
  }

  //Use bytes in stream to get Epoch Time and parse to time_t variable
  char time_string_buf[50] = {0}; /*FlawFinder: Ignore */
  client.readBytesUntil(',', time_string_buf, 50);
  time_t time = strtoll(time_string_buf, NULL, 0);

  // Code from using ArduinoJson. Caused memory issues.
  //Filter to only relevant data (set in setup function)
  // uint16_t json_size = 6044;
  // DynamicJsonDocument doc(json_size);
  // DeserializationError error = deserializeJson(doc, https.getStream(), DeserializationOption::Filter(date_time_filter), DeserializationOption::NestingLimit(14)) ;

  // const time_t time = doc["features"][0]["attributes"]["ETIME"].as<const time_t>();

  // doc.clear();

  https.end();

  #ifdef PRINT
    Serial.printf("Returning today's date as epoch time: %lld\n", time);
  #endif

  return time;
}

// Function to convert "MM/DD/YYYY date strings from appconfig.json to a time_t epoch variable."
time_t parse_config_date(const char* date){

  struct tm datetime = {0}; 
  strptime(date, "%m/%d/%y", &datetime);

  //Ensure standard non-day values and standard Unix Epoch year.
  datetime.tm_hour = 0;
  datetime.tm_min = 0;
  datetime.tm_sec = 0;
  datetime.tm_year -= 1900; // Years since 1900, for some reason (not 1970 standard) - https://mikaelpatel.github.io/Arduino-RTC/d8/d5a/structtm.html#a933e733942822b2def4aa966ee811293

  time_t epoch_time = mktime(&datetime);

  #ifdef PRINT
    Serial.printf("Converting date %s to epoch time %lld\n", date, epoch_time);
  #endif

  return epoch_time;

  /*
  *
    OLD CODE TO PARSE OUT INDIVIDUAL VALUES AND USE ADDITIONAL LIBRARY
  *
  */

  // const char* slash_delim = "/";
  // char tmp_date_string[50] = {0};

  // char* year = NULL;
  // char* month = NULL;
  // char* day = NULL;

  //Create temporary working string, and parse based on '/' in MM/DD/YYYY format from appconfig.json file
  // strncpy(tmp_date_string, date, strlen(date)+1);

  // month = strtok(tmp_date_string, slash_delim);
  // day = strtok(NULL, slash_delim);
  // year = strtok(NULL, slash_delim);

  // setTime(0, 0, 0, atoi(day), atoi(month), atoi(year));
  // time_t datetime = now();

  // #ifdef PRINT
  //   Serial.printf("Parsed Month: %s; Day: %s; Year: %s\n", month, day, year);
  //   Serial.printf("Converted date %s to epoch (not accurate) %d\n", date, datetime);
  // #endif

  // return datetime;
}

// Get TrainID for special train cars. Return -1 if error
int16_t get_special_train_id(WiFiClientSecure &client, uint16_t* special_cars, uint8_t num_special_cars){

  // StaticJsonDocument<112> special_train_filter;
  // special_train_filter["DataTable"]["diffgr:diffgram"]["DocumentElement"]["CurrentConsists"][0]["Cars"] = true; //Get DateTime from Special Train endpoint
  // special_train_filter["DataTable"]["diffgr:diffgram"]["DocumentElement"]["CurrentConsists"][0]["LinkTti"] = true; //NOT TRAINID FIELD. "LinkTti" / "ITT" on GIS = "TrainID" on API. "TRAINID" on GIS = "TrainNumber" on API.

  //Special, simple filter because deserializing stream one train object at a time.
  StaticJsonDocument<80> chunk_filter;
  chunk_filter["LinkTti"] = true;
  chunk_filter["Cars"] = true;

  HTTPClient https;
  https.useHTTP10(true);

  #ifdef PRINT
    Serial.println("Getting TrainID for Special Train");
  #endif

  //Get train information from WMATA special train endpoint
  client.setFingerprint(GIS_WMATA_COM_FINGERPRINT);

    if(!https.begin(client, GIS_SPECIAL_TRAIN_ENDPOINT)){
    #ifdef PRINT
      Serial.printf("Unable to connect to Special Train WMATA Endpoint\n");
    #endif
    return -1;
  }

  //Get Train data from Special Train endpoint
  https.begin(client, GIS_SPECIAL_TRAIN_ENDPOINT);
  https.addHeader("Accept-Encoding", "gzip, deflate");
  https.addHeader("Accept", "application/json,text/html");
  int response = https.GET();

  //Use chunk filtering to only deserialize one train object at a time
  // https://arduinojson.org/v6/how-to/deserialize-a-very-large-document/
  uint16_t json_size = 1024;
  DynamicJsonDocument doc(json_size);
  const char* delimiters = ".-"; //For parsing Cars string

  //Only load each train object into a JSON document at a time to preserve RAM by iterating through TCP stream.
  //WifiClient is actual consistent source of https stream. 
  client.find("\"CurrentConsists\":[");
  do {

    DeserializationError error = deserializeJson(doc, client, DeserializationOption::Filter(chunk_filter));

    if (error) {
      #ifdef PRINT
        Serial.printf("JSON Deserialization Failed: %s\n", error.f_str());
        Serial.printf("Intended JSON Document Size: %d\n", json_size);
        Serial.printf("JSON Doc Size: %d\n", doc.size());
        Serial.printf("Actual JSON Document Size: %d\n", doc.capacity());
        Serial.printf("HTTP Response Code %d\n", response);
        Serial.printf("HTTP Body Size: %d\n", https.getSize());
        Serial.printf("Overflowed? %s\n", doc.overflowed());
      #endif
    }

    else{

      uint16_t train_id = atoi(doc["LinkTti"].as<const char*>()); /*FlawFinder: Ignore */
      const char* cars = doc["Cars"].as<const char*>();

      uint8_t car_match_count = 0;

      //Setup tokenization of car string
      char tmp_car_string[50] = {0}; /*FlawFinder: Ignore */ 
      strncpy(tmp_car_string, cars, strlen(cars)+1); /*FlawFinder: Ignore */

      //Loop through each Car ID in train string. See if each car is a special train car and increment count if so. 
      //If not, break and move to next train
      char* token = strtok(tmp_car_string, delimiters);
      while(token != NULL){

        bool match = false;
        uint16_t car_num = atoi(token); /*FlawFinder: Ignore */

        for(uint8_t i=0; i < num_special_cars; i++){
          if(car_num == special_cars[i]){
            car_match_count++;
            match = true; 
            break;
          }
        }

        if(!match){
          break;
        }

        token = strtok(NULL, delimiters);
      }

      // If all the cars match the special train cars, we have the special train ID :)
      if(car_match_count == num_special_cars){

        doc.clear();
        https.end();

        #ifdef PRINT
          Serial.printf("Found special train id %d with cars %s\n", train_id, cars);
        #endif

        return train_id;
      }

      doc.clear();
    }//end if no DeserializationError

  } while (client.findUntil("," , "]"));

  https.end();

  return -1;
}

//Check WMATA Data If Special Train in place. Returns TrainID for special train or -1 if no special train.
int16_t check_for_special_train(WiFiClientSecure &client){

  #ifdef PRINT
    Serial.println("Beginning WMATA Config File Parsing");
  #endif

  uint16_t special_train_id = -1;
  const time_t today = get_todays_date(client);

  StaticJsonDocument<120> config_filter;
  config_filter["prd_settings"]["special"]["campaigns"][0]["start"] = true;
  config_filter["prd_settings"]["special"]["campaigns"][0]["end"] = true;
  config_filter["prd_settings"]["special"]["campaigns"][0]["cars"] = true;

  HTTPClient https;
  https.useHTTP10(true);

  // Connect to GIS Config File to get campaign info on special trains
  client.setFingerprint(GIS_WMATA_COM_FINGERPRINT);
  if(!https.begin(client, GIS_CONFIG_ENDPOINT)){
    #ifdef PRINT
      Serial.printf("Unable to connect to WMATA GIS Configuration File\n");
    #endif
  }

  https.begin(client, GIS_CONFIG_ENDPOINT);
  https.addHeader("Accept-Encoding", "gzip, deflate");
  https.addHeader("Accept", "application/json,text/html");
  int response = https.GET();

  //Discard junk characters at start of response
  char *tmp = (char*)malloc(10 * sizeof(char));
  https.getStream().readBytes(tmp, 3); 
  free(tmp);

  // Old code to troubleshoot byte stream
  // for (uint8_t i=0; i<49; i++){
  //   Serial.printf("%d:%c\n",i, tmp[i]);
  // }
  // Serial.println(tmp);

  //Filter to only relevant data (set at top of function)
  uint16_t json_size = 2048;
  DynamicJsonDocument doc(json_size);
  DeserializationError error = deserializeJson(doc, https.getStream(), DeserializationOption::Filter(config_filter));

  if (error) {
    #ifdef PRINT
      Serial.printf("JSON Deserialization Failed: %s\n", error.f_str());
      Serial.printf("Intended JSON Document Size: %d\n", json_size);
      Serial.printf("JSON Doc Size: %d\n", doc.size());
      Serial.printf("Actual JSON Document Size: %d\n", doc.capacity());
      Serial.printf("HTTP Response Code %d\n", response);
      Serial.printf("HTTP Body Size: %d\n", https.getSize());
      Serial.printf("Overflowed? %s\n", doc.overflowed());
    #endif
    return -1;
  }

  https.end();

  uint16_t special_cars[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  uint8_t num_special_cars = 0;

  //For each campaign, get dates for campaign, check if current date is within those dates, and get the train cars for the campaign if so
  for(JsonObject campaign : doc["prd_settings"]["special"]["campaigns"].as<JsonArray>()){

    const char* start_date = campaign["start"].as<const char*>();
    const char* end_date = campaign["end"].as<const char*>();
    JsonArray cars = campaign["cars"].as<JsonArray>();

    #ifdef PRINT
      Serial.printf("Comparing start day %lld, today %lld and end day %lld\n", parse_config_date(start_date), today, parse_config_date(end_date));
    #endif

    // If within the range for a campaign, get the train cars from that campaign and break loop
    if( parse_config_date(start_date) <= today && today <= parse_config_date(end_date)){

      num_special_cars = cars.size();

      for(uint8_t i=0; i<num_special_cars; i++){

        special_cars[i] = cars[i].as<uint16_t>();

        #ifdef PRINT
          Serial.printf("%d,",special_cars[i]);
        #endif

      }

      #ifdef PRINT
        Serial.println();
      #endif

      break;
    }

  }
  doc.clear();

  // If there are active special cars (set if date within campaign dates), get the TrainId for those cars
  if(num_special_cars > 0){
    special_train_id = get_special_train_id(client, special_cars, num_special_cars);
  }

  #ifdef PRINT
    Serial.printf("Out of Special Train. Special Train ID: %d\n", special_train_id);
  #endif

  // Return -1 or TrainId from above
  return special_train_id;
}