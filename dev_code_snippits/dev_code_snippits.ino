
#include <Arduino.h>

/*
Program to "emulate" train status on 10 leds using ESP8266 pinouts
Maintains a list of stations waiting for a "train" (random value).
When a "train" arrives, led for station is turned on, and list value is updated
to next station. 

First station is always waiting for a train. Upon arrival, station 2 is added to 2nd position in waiting list.
*/

const uint8_t MAX_NUM_STATIONS = 10; //Max number of stations per line
const uint8_t STATION_STR_LNGTH = 4; //Number of chars per station identifier string (3+NULL).

class SimpleList {
  public:
    uint8_t len;
    char stations[MAX_NUM_STATIONS][STATION_STR_LNGTH];
    
    SimpleList(){
      len = 0;
      memset(stations, 0, (MAX_NUM_STATIONS*STATION_STR_LNGTH));
    }

    int add(char* station){
      if( strlen(station) < STATION_STR_LNGTH ){
        if(len == 0 || len == 1){
          memcpy(stations+len, station, STATION_STR_LNGTH);
        }
        else{
          char* buf = new char[MAX_NUM_STATIONS*(len-1)];
          memcpy(buf, stations+1, (STATION_STR_LNGTH*(len-1)));
          memcpy(stations+1, station, STATION_STR_LNGTH);
          memcpy(stations+2, buf, (STATION_STR_LNGTH*(len-1)));
         //memcpy(stations+2, stations+1, (STATION_STR_LNGTH*(len-1))); //right-shift awaiting stations
         //memcpy(stations+1, station, STATION_STR_LNGTH);
         delete[] buf;
        }
        len++;
        return 0;
      }
      else{
        return - 1; 
      } 
    }//end add

    int remove(char* station){
      if( strlen(station) < STATION_STR_LNGTH){
        for(uint8_t i=0; i<len; i++){
          //Once station in list is found, overwrite it with all seceeding stations, set previous last element to NULL, and decrement length.
          if(strcmp(station,stations[i]) == 0){
            memcpy(stations+i, stations+i+1, (STATION_STR_LNGTH*(len-i-1)));
            memset(stations+len, NULL, STATION_STR_LNGTH);
            len--;
            return 0;
          }
        }//endfor
        return -1;
      }//endif
      return -1;
    }//end remove

/*
    void prnt(HardwareSerial* serial){
      if(len == 0){
        Serial.println("Empty");
        return;
      }
      for(uint8_t i=0; i<len; i++){
        Serial.println(stations[i]);
      }
    }
*/  
};//END SimpleList


 int LED_LENGTH = 10;
 int leds[10] = {15, 13, 12, 14, 2, 0, 4, 5, 16, 10}; //GPIO9 messes up all other pins if used. 1 and 2 used for serial. 
 uint16_t led_state = 0;
 //uint8_t offset = 16 - LED_LENGTH;

//Set LED's output to whatever it's state is in led_state bit array.
void setLED(int led){
  uint16_t mask = 1;
  digitalWrite(leds[led], ((led_state & (mask << led))>0)); //Move mask to led's position in array. bit-AND w/ mask returns 0 if state for led is 0 and > 0 otherwise. Need '>' b/c results > 2^7 overflow
}

void setup() {
  // put your setup code here, to run once:
  //pinMode(2, OUTPUT);
  Serial.begin(9600);
  Serial.println("Serial test");
  randomSeed(0);
  for(int i=0; i<LED_LENGTH; i++){
    pinMode(leds[i], OUTPUT);
  }
}//END SETUP

void loop() {
  // put your main code here, to run repeatedly:
  char stations[10][4] = {"A01", "A02", "A03", "A04", "A05", "A06", "A07", "A08", "A09", "A10"};

  SimpleList awaiting = SimpleList();
  awaiting.add(stations[0]);

  // /*
  uint16_t init_mask = 15; // 0b00001111 (first four leds)
  while (true){
    //Serial.println("Hello");

    //Serial.println(awaiting.len);
  
    if (awaiting.len == 0){
      Serial.println("Empty");
    }
    else{
      Serial.println("Waiting Station List:");
      //Serial.println(awaiting.stations[0]);
      for(int i=0; i<awaiting.len; i++){
        //Serial.println("Station: ");
        Serial.println(awaiting.stations[i]); 
      }
    }

    /*
    awaiting.remove(stations[0]);

    Serial.println("Station List: ");
    for (int i=0; i<awaiting.len; i++){
      Serial.println(awaiting.stations[i]);
    }
    */

    uint8_t tmp_len = awaiting.len;
    for(int i=awaiting.len-1; i>=0; i--){
      uint8_t r = 3;
      if(i == awaiting.len-1) { r =2; }
      if(random(r) == 0){
        Serial.println("IN RANDOM");
        char substr[3];
        strncpy(substr, &awaiting.stations[i][1], 3); //just get digits
        Serial.println(substr);
        uint8_t statn_idx = atoi(substr)-1;


        uint8_t nxt_idx = 11;
        if(i != awaiting.len-1){
          strncpy(substr, &awaiting.stations[i+1][1], 3);
          nxt_idx = atoi(substr)-1;
        }
        if( nxt_idx != (statn_idx + 1)){
          Serial.println("Station index: " + String(statn_idx));
          if(statn_idx == 0){
            awaiting.add(stations[statn_idx+1]);
            tmp_len++;
            digitalWrite(leds[statn_idx], 1);
          }
          else if(statn_idx == MAX_NUM_STATIONS-1){
            awaiting.remove(stations[MAX_NUM_STATIONS-1]);
            tmp_len--;
            digitalWrite(leds[statn_idx-1],0);
            
          }
          else{
            strncpy(awaiting.stations[i], stations[statn_idx+1], STATION_STR_LNGTH);
            digitalWrite(leds[statn_idx], 1);
            digitalWrite(leds[statn_idx-1], 0);
          }//end if block
        }//end check for current train
        //break;
        delay(1000);  
      }//end random
    }//end for
    awaiting.len=tmp_len;

    delay(5000);
    /*
    //If the first three LEDs are off, turn the first one on.
    if (!(init_mask & led_state)){
      led_state = led_state | 1;
      setLED(0);
    }
    //Serial.println(led_state);
    led_state <<=1; //Shift state
    //Serial.println(led_state);
    delay(2000);
    uint16_t tmp = led_state;
    for (short i=0; i<LED_LENGTH; i++){
      digitalWrite(leds[i], tmp & 1);
      tmp >>=1;
    }
    /*
    for (short i=0; i<LED_LENGTH; i++){
      setLED(i);
    }
    */
  } //END while
}//END LOOP
