#include "../inc/SimpleList.h"

/*
Simple program to demonstrate SimpleList functionality with simulated "trains" arriving as defined by random().
Designed to not require Arduino features (e.g. LEDs) for EpoxyDuino compilation and desktop unit tests.
*/




void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  Serial.println("Serial test");
  randomSeed(0);
}//END SETUP

void loop() {
  //Initialize list of waiting trains. Initializes with 1st station waiting
  SimpleList awaiting = SimpleList(); 

  while (true){

    for(int i=awaiting.getLen()-1; i>=0; i--){
      uint8_t r = 3;
      if(i == awaiting.getLen()-1) { r =2; }
      if(random(r) == 0){ /*Flawfinder: ignore */
        Serial.println("IN RANDOM");
        Serial.println("Current State");
        Serial.println(awaiting.getState());

        Serial.println("Arrived at station: ");
        Serial.println(awaiting.stations[i]);
        awaiting.arrived(i);

        Serial.println("Updated State: ");
        Serial.println(awaiting.getState());

        Serial.println();

      }// end if for arrived train
    }//end for loop iterating through waiting stations

    Serial.println();
    Serial.println("-------------------------------------------------");
    Serial.println();

  } //END while
}//END LOOP
