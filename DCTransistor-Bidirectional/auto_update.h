#include "config.h"

HTTPClient https;

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
}//END check_for_update