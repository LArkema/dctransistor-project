#line 2 "SimpeListTest.ino"

#include <AUnit.h>
#include "../../inc/SimpleList.h"

/*
Unit tests for SimpleList class. Validated by Aunit Tests Github Action on push pull (different paths than Arduino compiler).
Compiles for EpoxyDuino - can run on both Arduino and linux.
*/

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  //Serial.println("Serial test");
  randomSeed(0);
}//END SETUP

void loop() {
  aunit::TestRunner::run();
}//END LOOP


test(proper_init){
  SimpleList test = SimpleList();
  assertEqual(test.getLen(), (uint8_t)1);
}

test(add_station){
  SimpleList test = SimpleList();
  test.arrived(0);
  assertEqual(test.stations[1], 1);
  assertEqual(test.stations[0], 0);
  assertEqual(test.getState(), "0, 1");
}

test(increment_station){
  SimpleList test = SimpleList();
  test.arrived(0);
  test.arrived(1);
  assertEqual(test.stations[1], 2);
  assertEqual(test.stations[0], 0);
  assertEqual(test.getState(), "0, 2");
}

test(multiple_increments){
  SimpleList test = SimpleList();
  test.arrived(0);
  test.arrived(1);
  test.arrived(1);
  test.arrived(1);
  test.arrived(1);
  assertEqual(test.stations[1], 5);
  assertEqual(test.stations[0], 0);
  assertEqual(test.getState(), "0, 5");
}

test(multiple_stations){
  SimpleList test = SimpleList();
  test.arrived(0);
  test.arrived(1);
  test.arrived(0);
  assertEqual(test.stations[2], 2);
  assertEqual(test.stations[1], 1);
  assertEqual(test.stations[0], 0);
  assertEqual(test.getState(), "0, 1, 2");
}

test(multiple_stations_multiple_increments){
  SimpleList test = SimpleList();
  test.arrived(0);
  test.arrived(1);
  test.arrived(0);
  test.arrived(2);
  test.arrived(1);
  test.arrived(0);
  test.arrived(3);
  test.arrived(2);
  test.arrived(1);
  test.arrived(3);
  test.arrived(3);
  assertEqual(test.getState(), "0, 2, 3, 6");
}

//Fill list be incrementing each "train" positing from front to back
test(fill_list){
  SimpleList test = SimpleList();
  do {
    for(int i=test.getLen()-1; i>=0; i--){
      test.arrived(i);
    }
  }while(test.getLen() < MAX_NUM_STATIONS);

  assertEqual(test.getLen(), MAX_NUM_STATIONS);
  assertEqual(test.getState(), "0, 1, 2, 3, 4, 5, 6, 7, 8, 9");
}

//fill list then try to add more
test(overrun_test){
  int res;
  SimpleList test = SimpleList();
  do {
    for(int i=test.getLen()-1; i>=0; i--){
      test.arrived(i);
    }
  }while(test.getLen() < MAX_NUM_STATIONS);

  res = test.arrived(0);
  assertEqual(res, -2);
}

test(single_train_all_stations){
  SimpleList test = SimpleList();
  test.arrived(0);
  for(int i=1; i<MAX_NUM_STATIONS-1; i++){
    test.arrived(1);
  }
  assertEqual(test.stations[1], 9);
  assertEqual(test.getLen(), 2);

  test.arrived(1);

  assertEqual(test.getLen(), 1);
  assertEqual(test.stations[1], 0);
}

test(full_list_remove_then_add){
  SimpleList test = SimpleList();
  do {
    for(int i=test.getLen()-1; i>=0; i--){
      test.arrived(i);
    }
  }while(test.getLen() < MAX_NUM_STATIONS);

  assertEqual(test.getLen(), MAX_NUM_STATIONS);
  assertEqual(test.getState(), "0, 1, 2, 3, 4, 5, 6, 7, 8, 9");

  test.arrived(MAX_NUM_STATIONS-1);

  assertEqual(test.getLen(), MAX_NUM_STATIONS-1);
  assertEqual(test.stations[test.getLen()-1], MAX_NUM_STATIONS-2);

  for(int i=test.getLen()-1; i>=0; i--){
    test.arrived(i);
  }

  assertEqual(test.getLen(), MAX_NUM_STATIONS);
  assertEqual(test.getState(), "0, 1, 2, 3, 4, 5, 6, 7, 8, 9");
}

//Test various illegal arrival scenarios (out of bounds, list full, neighbor collision).
test(illegal_arrived){
  int res;
  SimpleList test = SimpleList();
  res = test.arrived(2);
  assertEqual(res, -1);

  test.arrived(0);
  test.arrived(1);
  res = test.arrived(2);
  assertEqual(res, -1);

  do {
    for(int i=test.getLen()-1; i>=0; i--){
      test.arrived(i);
    }
  }while(test.getLen() < MAX_NUM_STATIONS);

  for(int i=test.getLen()-2; i>=0; i--){
      res = test.arrived(i);
      assertLess(res, 0);
  }

  res = test.arrived(test.getLen()-2);
  assertLess(res, 0);

  test.arrived(test.getLen()-1);

  res = test.arrived(test.getLen()-2);
  assertLess(res, 0);

  res = test.arrived(test.getLen()-1);
  assertEqual(res, MAX_NUM_STATIONS-1);
}