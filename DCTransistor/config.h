/*
*   CONFIGURATION FILE FOR DCTRANSISTOR BINARY
*
*   Arranged by configuration values most likely to be relevant to users    
*   Followed by debug values 
*   Followed by values that may occasionally change, but are still required
*   Followed by required values
*
*   (c) Logan Arkema 2023
*/

//Include Standard and Custom libraries and classes
#include <Arduino.h>
#include <Adafruit_LEDBackpack.h>
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>
#include <WiFiManager.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

/*
*   USER CONFIGURATION VALUES
*/

//If using own WMATA API key, enter here. Otherwise, one built into downloaded binary
#define SECRET_WMATA_API_KEY "abcdef0123456789abcdef0123456789"

//Whether or not to check for automatic updates every time board powers on (turning to false may break board when web TLS certificates expire)
#define AUTOUPDATE true

//Uncomment below line to print program text output to Serial output (requires attaching board to computer via USB cable)
//#define PRINT

//Number of seconds to wait between requests to WMATA server (WMATA updates every ~20, per documentation)
#define WAIT_SEC 20

//Name of WiFi Network (SSID) Board Creates when unable to connect to wifi
#define WIFI_NAME "DCTransistor"
#define WIFI_PASSWORD "trainsareneat"

// ----  LED Configuration Values ----
#define LED_BRIGHTNESS 10 //Range of 0-100. Can get very bright very fast

//Define LED color for each train line. Defined as WWRRGGBB values where white (first byte) is always set to 0.
//Pick own colors using a tool like https://www.w3schools.com/colors/colors_picker.asp
#define RD_HEX_COLOR 0x00FF0000 
#define BL_HEX_COLOR 0x000000FF
#define OR_HEX_COLOR 0x00FF8000
#define SV_HEX_COLOR 0x00808080
#define YL_HEX_COLOR 0x00FFFF00
#define GN_HEX_COLOR 0x0000FF00


/*
*   DEBUG CONFIGURATION VALUES
*/

//JSON document sizes must be predefined, and may need to be increased if amount of data increases
#define JSON_FILTER_SIZE 80 //Bytes of filter to apply to JSON data returned from WMATA
#define JSON_DOC_SIZE 8096 //Bytes of parsed and filtered JSON data returned from WMATA (All trian line, position, and circuitIDs)


/*
*   OCCASIONALLY CHANGING VALUES
*/

//Version string. Changes with every software version
#define VERSION "0.2.0"

//Web server certificate SHA1 fingerprints for TLS connections. Need to update as certs expire
#define GITHUB_FINGERPRINT "1E 16 CC 3F 84 2F 65 FC C0 AB 93 2D 63 8A C6 4A 95 C9 1B 7A" //7A
#define GHUB_CONTENT_FINGERPRINT "8F 0E 79 24 71 C5 A7 D2 A7 46 76 30 C1 3C B7 2A 13 B0 01 B2" //B2
#define WMATA_FINGERPRINT "C5 14 29 8E E1 04 75 0C A3 B3 1C 9D BB 43 BA 13 A0 CA A0 F7" //F7

/*
*   REQUIRED CONFIGURATION VALUES
*   (Things will break or not work right if changed)
*/

//Headers to grab from response to request to /releases/latest (returns redirect to URL containing version umber)
const char* github_header_keys[] = {"location"};
const int github_num_headers = 1;

//URLs and remote hosts for software updates and WMATA data
#define LATEST_VERSION_URL "https://github.com/LArkema/dctransistor-project/releases/latest"
#define UPDATE_BIN_URL "https://raw.githubusercontent.com/LArkema/dctransistor-project/main/dctransistor.bin.gz"
#define UPDATE_HOST "raw.githubusercontent.com"
#define WMATA_ENDPOINT "https://api.wmata.com/TrainPositions/TrainPositions?contentType=json"
#define HTTPS_PORT 443

//Frequency for sending debug messages from ESP8266 chip to computer
#define BAUD_RATE 9600

// ----- LED CONFIGURATIONS ----
//configurations for chained together WS2812B LEDS. Datasheet: https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf
#define LED_PIN  4 //GPIO pin sending data to 1st WS2812B LED
#define LED_COUNT 105 //total # LEDs
#define PWR_LED 104 //index of "Power" (should be last)
#define WIFI_LED 103 //indoex of "WiFi" (2nd to last)
#define WEB_LED 102 //index of "Web" (3rd to last)

// Number of circuits before a Station's exact circuit to count a train as "at" that station
#define CIRCS_BEFORE_STATION 2

// ----- TRAIN LINE CONFIGURATIONS -----

/*
Each line is defined by the number of stations on the line, the color trains on that line should be as a WRGB 32-bit hex value,
And two arrays of circuit indecies that correlate with each station on that line (defined by WMATA).

stations_0 is for trains (generally) moving North / East and is Direction 1 in WMATA products.
stations_1 is for trains (generally) moving South / West and is Direction 2 in WMATA products.
The arrays are ordered IN THE DIRECTION THE TRAIN MOVES, 
so the first station circuit in one direction represents the same station as the last station circuit in the other direction

See misc_commands.sh for how each array was created from WMATA's information
*/

#define TOTAL_SYSTEM_STATIONS 104 //Includes 2 stations for intersectionS (not in LED count) and future stations.

#define NUM_LINES 6

//Red Line
#define NUM_RD_STATIONS 27
const uint16_t rstations_0[NUM_RD_STATIONS] = {7, 32, 53, 62, 80, 95, 109, 126, 133, 142, 154, 164, 179, 190, 203, 467, 477, 485, 496, 513, 527, 548, 571, 591, 611, 629, 652};
const uint16_t rstations_1[NUM_RD_STATIONS] = {868, 846, 828, 809, 785, 757, 731, 717, 700, 686, 677, 667, 661, 389, 378, 363, 356, 346, 336, 326, 309, 294, 278, 260, 251, 232, 210};

//Blue Line
#define NUM_BL_STATIONS 27
const uint16_t bstations_0[NUM_BL_STATIONS] = {2604, 2634, 969, 976, 1010, 1024, 1036, 1052, 1070, 1092, 1105, 1117, 1126, 1135, 1384, 1393, 1400, 1406, 1418, 1424, 1436, 1443, 2420, 2434, 2449, 2469, 2487};
const uint16_t bstations_1[NUM_BL_STATIONS] = {2574, 2557, 2537, 2521, 2506, 1618, 1610, 1598, 1590, 1575, 1568, 1559, 1549, 1330, 1323, 1310, 1298, 1285, 1265, 1246, 1230, 1217, 1204, 1170, 1162, 2709, 2679};

//Orange line
#define NUM_OR_STATIONS 26
const uint16_t ostations_0[NUM_OR_STATIONS] = {2774, 2796, 2817, 2844, 2870, 2886, 2898, 2911, 1092, 1105, 1117, 1126, 1135, 1384, 1393, 1400, 1406, 1418, 1424, 1436, 1443, 1475, 1487, 1500, 1522, 1542};
const uint16_t ostations_1[NUM_OR_STATIONS] = {1711, 1692, 1670, 1657, 1643, 1618, 1610, 1598, 1590, 1575, 1568, 1559, 1549, 1330, 1323, 1310, 1298, 1285, 3061, 3048, 3037, 3023, 3001, 2976, 2954, 2933};

//Silver Line
#define NUM_SV_STATIONS 34
const uint16_t sstations_0[NUM_SV_STATIONS] = {3523, 3544, 3577, 3594, 3612, 3627, 3155, 3214, 3221, 3232, 3238, 2844, 2870, 2886, 2898, 2911, 1092, 1105, 1117, 1126, 1135, 1384, 1393, 1400, 1406, 1418, 1424, 1436, 1443, 2420, 2434, 2449, 2469, 2487};
const uint16_t sstations_1[NUM_SV_STATIONS] = {2574, 2557, 2537, 2521, 2506, 1618, 1610, 1598, 1590, 1575, 1568, 1559, 1549, 1330, 1323, 1310, 1298, 1285, 3061, 3048, 3037, 3023, 3001, 3377, 3370, 3359, 3352, 3290, 3741, 3724, 3706, 3689, 3657, 3637};

//Yellow Line
#define NUM_YL_STATIONS 21               // !! PENTAGON & L'ENFANT STATION INDEXES AND BRIDGE CIRCUITS ARE HARDCODED IN setTrainState !!
const uint16_t ystations_0[NUM_YL_STATIONS] = {944, 955, 969, 976, 1010, 1024, 1036, 1052, 2231, 2241, 2246, 1753, 1764, 1773, 1782, 1796, 1809, 1833, 1850, 1871, 1894};
const uint16_t ystations_1[NUM_YL_STATIONS] = {2055, 2030, 2009, 1992, 1971, 1956, 1942, 1932, 1923, 1911, 1899, 2376, 2364, 1246, 1230, 1217, 1204, 1170, 1162, 1148, 1137};

//Green Line
#define NUM_GN_STATIONS 21
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
