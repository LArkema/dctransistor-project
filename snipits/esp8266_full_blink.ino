 #include <Arduino.h>

 //uint8_t leds[13] = {1, 3, 15, 13, 12, 14, 2, 0, 4, 5, 16, 10, 9};
 int LED_LENGTH = 12;
 int leds[12] = {1, 3, 15, 13, 12, 14, 2, 0, 4, 5, 16, 10}; //GPIO9 messes up all other pins if used.


void setup() {
  // put your setup code here, to run once:
  //pinMode(2, OUTPUT);
  Serial.begin(9600);
  for(int i=0; i<LED_LENGTH; i++){
    pinMode(leds[i], OUTPUT);
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  for(int i=0; i<LED_LENGTH; i++){
    Serial.println(i);
    Serial.println(leds[i]);
    digitalWrite(leds[i], 1);
    delay(1000);
    digitalWrite(leds[i], 0);
  }
  /*
  digitalWrite(2, HIGH);
  delay(1000);
  digitalWrite(2, LOW);
  delay(1000);
  */
}
