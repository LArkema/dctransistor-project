#include "config.h"

HTTPClient https;

//If versions don't match, download current version of binary from main branch of github repo (will always have most recent binary)
void update_arduino(WiFiClientSecure &client, String cur_version){

  client.setFingerprint(ghub_content_fingerprint);
  client.connect(update_host, HTTPS_PORT);
  
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, update_bin_url);

  #ifdef PRINT
   switch (ret) {
      case HTTP_UPDATE_FAILED: Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str()); break;

      case HTTP_UPDATE_NO_UPDATES: Serial.println("HTTP_UPDATE_NO_UPDATES"); break;

      case HTTP_UPDATE_OK: Serial.println("HTTP_UPDATE_OK"); break;
    } 
  #endif

}//END update_arduino


void check_for_update(WiFiClientSecure &client){

    //Send HTTP request to /releases/latest on GitHub page, which always returns a 302
    client.setFingerprint(github_fingerprint);
    https.begin(client, latest_version_url);
    https.collectHeaders(github_header_keys, github_num_headers);

  
    //Get version based /releases/latest redirecting to /releases/tag/<cur_version>
    String release_loc = https.header("location");
    int version_index = release_loc.lastIndexOf("/")+1;
    String latest_version = release_loc.substring(version_index);

    #ifdef PRINT
      Serial.printf("Latest release URL: %d\n", release_loc);
      Serial.printf("Latest version: %s\n", latest_version);
    #endif

    //If version doesn't match software's hardcoded version string, trigger update function.
    if (latest_version != VERSION){
        #ifdef PRINT
          Serial.println("Versions don't match. Updating");
        #endif
        update_arduino(client, latest_version);
    }
}//END check_for_update