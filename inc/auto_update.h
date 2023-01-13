#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include "config.h"

//WiFiClientSecure client;
HTTPClient https;

const char* github_header_keys[] = {"location"};
const int github_num_headers = 1;

const char* github_fingerprint = "1E 16 CC 3F 84 2F 65 FC C0 AB 93 2D 63 8A C6 4A 95 C9 1B 7A";
const char* ghub_content_fingerprint = "8F 0E 79 24 71 C5 A7 D2 A7 46 76 30 C1 3C B7 2A 13 B0 01 B2";

const String version = "0.2.0";

const String update_bin_url = "https://raw.githubusercontent.com/LArkema/dctransistor-project/main/dctransistor.bin.gz";
const String update_host = "raw.githubusercontent.com";

//If versions don't match, download current version of binary from main branch of github repo (will always have most recent binary)
void update_arduino(WiFiClientSecure &client, String cur_version){

  client.setFingerprint(ghub_content_fingerprint);
  client.connect(update_host, 443);
  
  t_httpUpdate_return ret = ESPhttpUpdate.update(client, update_bin_url);

/*
   switch (ret) {
      case HTTP_UPDATE_FAILED: Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", ESPhttpUpdate.getLastError(), ESPhttpUpdate.getLastErrorString().c_str()); break;

      case HTTP_UPDATE_NO_UPDATES: Serial.println("HTTP_UPDATE_NO_UPDATES"); break;

      case HTTP_UPDATE_OK: Serial.println("HTTP_UPDATE_OK"); break;
    } 

*/

}//END update_arduino


void check_for_update(WiFiClientSecure &client){

    //Send HTTP request to /releases/latest on GitHub page, which always returns a 302
    client.setFingerprint(github_fingerprint);
    https.begin(client, "https://github.com/LArkema/dctransistor-project/releases/latest");
    https.collectHeaders(github_header_keys, github_num_headers);
    Serial.println(https.GET());
  
    //Get version based /releases/latest redirecting to /releases/tag/<cur_version>
    String release_loc = https.header("location");
    int version_index = release_loc.lastIndexOf("/")+1;
    String latest_version = release_loc.substring(version_index);
    Serial.println(latest_version);

    //If version doesn't match software's hardcoded version string, trigger update function.
    if (latest_version != version){
        Serial.println("Versions don't match. Updating");
        update_arduino(client, latest_version);
    }
}//END check_for_update