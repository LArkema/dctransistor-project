#include <Arduino.h>
#include "Adafruit_LEDBackpack.h"

/*
    Defines TrainLine class - an evolution of SimpleList. 
    Data structure optimized to store which stations on a line are waiting for a train to arrive.
    and updates accordingly. Represents stations using index (position of station on given line) for simplicity.

    Current: Developing bi-directional state representation.

    Designed to compile on Desktop (using EpoxyDuino) and Arduino for easy and integrated unit testing.
*/

#define TOTAL_SYSTEM_STATIONS 104 //Includes 2 stations for intersection and future stations.

//Define TrainLine member variables and functions.
class TrainLine {

  private:
    //variables
    uint8_t total_num_stations; //number of stations on the line

    //INDEXES ARE # OF STATIONS FROM START - e.g. waiting_stations_0[0] == waiting_stations_1[total_num_stations-1]
    uint8_t* waiting_stations[2]; //simple array to point to the list of waiting stations for each direction
    uint8_t* waiting_stations_0; //dynamically allocated list of integers representing directional train states ...
    uint8_t* waiting_stations_1; //... through waiting station indexes

    //List of circuitIDs for stations in both directions.
    // INDEXES ARE REVERSED for direction 1 (direction 2 in WMATA API)
    uint16_t* station_circuits[2]; //REVERSE INDEXES - simple array to point to the lists of station circuit IDs for each direction
    uint16_t* station_circuits_0; //dynamically allocated list for station circuitIDs, trains move positively along circuits
    uint16_t* station_circuits_1; //stores station circuits for other directions, trains move negatively along circuits

    uint8_t* station_leds;

    int32_t state;

    //uint8_t first_led;

    uint32_t led_color;

    int8_t led_to_station_map[TOTAL_SYSTEM_STATIONS];

    //uint8_t last_led;

    /*
    uint8_t num_clusters;
    uint16_t* cluster_edges_0;
    uint16_t* cluster_edges_1;
    */


    uint8_t lens[2]; //array to hold length of active train array for both directions.
    uint8_t cycles_at_end[2]; //hold how many cycles a train has been at respective dir's last station
    uint16_t opp_dir_1st_cid[2]; //hold opposive of a given dir's 1st CircuitID. 
    bool last_station_waiting[2];

    //uint8_t* leds; //One list of LEDs represents trains going both directions

    //state updating functions
    int add(bool dir);
    int insert(uint8_t index, bool dir);
  
  public:

    //Constructors and Destructor
    TrainLine();
    TrainLine(uint8_t num_stations, const uint16_t* circuit_list_0, const uint16_t* circuit_list_1, uint32_t hex_color, const uint8_t* led_list);
    ~TrainLine();

    //State updating functions
    int arrived(uint8_t index, bool dir);
    int setInitialStations(uint16_t* train_positions, uint8_t train_len, bool dir);
    int remove(bool dir);

    int incrementCyclesAtEnd(bool dir); //returns updated number of cycles.
    int setTrainState(uint16_t circuitID, uint8_t train_dir);

    void updateLEDS();  //For fully stateful (use lists of waiting station indexes for each direction)
    void updateLEDS2(Adafruit_LEDBackpack &matrix); //Based on minimully stateful version (bit array reset every check with server)
    void updateLEDS3(Adafruit_NeoPixel &strip); //Based on minimally stateful version, but with WS2812 strip instead of HT1633k matrix

    void setEndLED(); //For minimally stateful version, set last station's led on if necessary
    void clearState(); //reset state after every API call.

    bool trainAtLED(uint8_t led); //If a train is at a station represented by the given led, return true.

    //Getters
    uint8_t* getStations(bool dir);
    String getState();
    String getState(bool dir);
    String printVariables();
    uint8_t getLen(bool dir);
    uint32_t getLEDColor();
    uint8_t getTotalNumStations();
    uint8_t getCyclesAtEnd(bool dir);
    uint16_t getOppCID(bool dir);
    uint16_t getLastCID(bool dir);
    int8_t at(uint8_t index, bool dir);
    //int8_t operator[](uint8_t index, bool dir); //Doesn't make sense with two lists

    int16_t getWaitingStationCircuit(uint8_t index, bool dir); //get circuit for station based on index in waiting stations
    int16_t getStationCircuit(uint8_t index, bool dir); //get circuitID of any given station

};//END SimpleList definition

//Add new station to waiting station list to represent new train. New train only added when train arrives at 1st station.
//Since 1st station (index 0) always waiting, insert new train into 2nd position and copy all other values over one.
//Returns new size of waiting stations list, or -1 if out-of-bounds (list is full)
int TrainLine::add(bool dir){
  uint8_t len = lens[dir];
  if( len < total_num_stations ){
    if(len == 1){
      waiting_stations[dir][len] = 1;
    }
    else{
      uint8_t* buf = new uint8_t[len-1];
      memcpy(buf, waiting_stations[dir]+1, (len-1)); /*Flawfinder: ignore */
      waiting_stations[dir][1] = 1;
      memcpy(waiting_stations[dir]+2, buf, (len-1)); /*Flawfinder: ignore */
      delete[] buf;
    }
    lens[dir]++;
    return lens[dir];
  }
  else{
    return -1; 
  } 
}//end add

//Insert a station into list of waiting stations using insertion sort. 
//Only called when initially setting line state, when order of inserted stations is unknown
//Not optimized for insertion sort b/c only run once per startup.
//Station_num == station index FOR CURRENT DIRECTION (always moves positively)
int TrainLine::insert(uint8_t station_num, bool dir){
  //Serial.println("In Insert");
  //Serial.printf("Current State: %s\tCurrent Len: %d\n", getState(), len);
  uint8_t len = lens[dir];
  //If list "empty" (only first station waiting), add station
  if(len == 1){
    waiting_stations[dir][1] = station_num;
    lens[dir]++;
    return 0;
  }

  //If station is larger than last (largest) station, append.
  else if(station_num > waiting_stations[dir][len-1] ){
    waiting_stations[dir][len] = station_num;
    lens[dir]++;
    return 0;
  }

  //Move all items larger than station up 1, and insert where 1st item larger than inserted station was
  //where i is index of first staion larger than isnerted station.
  for(int8_t i = 0; i < len; i++){
    if(station_num < waiting_stations[dir][i] ){
      //Serial.printf("Station %d is smaller than station %d. Inserting it at index %d\n", station_num, waiting_stations[dir][i], i);
      if(len-i > 0){
        uint8_t* buf = new uint8_t[len-i]; /*Flawfinder: ignore */
        memcpy(buf, waiting_stations[dir]+i, (len-i)); /*Flawfinder: ignore */
        memcpy(waiting_stations[dir]+i+1, buf, (len-i)); /*Flawfinder: ignore */
        delete[] buf;
      }//end shift
      waiting_stations[dir][i] = station_num;
      lens[dir]++;
      return 0;
    }//end if smaller value.
  }
  return -1;
}//end insert

//Remove a train after reaching end of line. Removes end of list (i.e. dequeue)
//Returns size of waiting stations list, 
//Returns -1 if last train not at end of line
int TrainLine::remove(bool dir){
  //can only remove if last train is at last station
  uint8_t len = lens[dir];
  if(waiting_stations[dir][len-1] == total_num_stations){
    //digitalWrite(leds[waiting_stations[dir][len-1]-1], 0); //URGENT: UPDATE LED HANDLING
    waiting_stations[dir][len-1] = 0;
    lens[dir]--;
    return lens[dir];
  }
  return -1;
}//end remove

//Constructor sets first station as waiting and initializes rest of list to 0.
//Set default values for testing northbound end of redline.
TrainLine::TrainLine(){
  total_num_stations = 10;
  lens[0] = 1;
  lens[1] = 1;

  //Set state arrays to number of stations in track.
  waiting_stations_0 = new uint8_t[total_num_stations];
  waiting_stations_1 = new uint8_t[total_num_stations];
  station_circuits_0 = new uint16_t[total_num_stations];
  station_circuits_1 = new uint16_t[total_num_stations];
  station_leds = new uint8_t[total_num_stations];

  waiting_stations[0] = waiting_stations_0;
  waiting_stations[1] = waiting_stations_1;

  station_circuits[0] = station_circuits_0;
  station_circuits[1] = station_circuits_1;

  memset(waiting_stations[0], 0, sizeof(*waiting_stations));
  memset(waiting_stations[1], 0, sizeof(*waiting_stations));

  memset(led_to_station_map, -1, sizeof(*led_to_station_map));


  uint16_t tmp_station_circuits_0[10] = {485, 496, 513, 527, 548, 571, 591, 611, 629, 652};
  uint16_t tmp_station_circuits_1[10] = {868, 846, 828, 809, 785, 757, 731, 717, 700, 686};

  uint8_t tmp_leds[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

  //first_led = 0;
  led_color = 0x00FF0000; //Red in hex-based RGB value

  

  memcpy(station_circuits_0, tmp_station_circuits_0, sizeof(uint16_t) * total_num_stations); /*FlawFinder: Ignore */
  memcpy(station_circuits_1, tmp_station_circuits_1, sizeof(uint16_t) * total_num_stations); /*FlawFinder: Ignore */

  memcpy(station_leds, tmp_leds, sizeof(uint8_t) * total_num_stations); /*FlawFinder: Ignore */


  //Given system-wide led number, map to that station's index in this line.
  for(uint8_t i=0; i<total_num_stations; i++){
    led_to_station_map[station_leds[i]] = i;
  }

  opp_dir_1st_cid[0] = station_circuits_1[0];
  opp_dir_1st_cid[1] = station_circuits_0[0];

  cycles_at_end[0] = 0;
  cycles_at_end[1] = 0;
  state = 0;

  last_station_waiting[0] = false;
  last_station_waiting[1] = false;

}
//END default constructor

//Overloaded constructor that takes list of track circuitIDs corresponding to each station on line
TrainLine::TrainLine(uint8_t num_stations, const uint16_t* circuit_list_0, const uint16_t* circuit_list_1, uint32_t hex_color, const uint8_t* led_list){
  
  //Serial.println("In constructor");
  //delay(500);

  total_num_stations = num_stations;
  //num_clusters = CID_clusters_num;
  lens[0] = 1;
  lens[1] = 1;

  //Set state arrays to number of stations in track.
  waiting_stations_0 = nullptr; //new uint8_t[total_num_stations];
  waiting_stations_1 = nullptr; //new uint8_t[total_num_stations];
  station_circuits_0 = new uint16_t[total_num_stations];
  station_circuits_1 = new uint16_t[total_num_stations];
  station_leds = new uint8_t[total_num_stations];

  //Point directional arrays to each list
  waiting_stations[0] = waiting_stations_0;
  waiting_stations[1] = waiting_stations_1;
  station_circuits[0] = station_circuits_0;
  station_circuits[1] = station_circuits_1;

  //Set both waiting stations list to 0s
  //memset(waiting_stations_0, 0, sizeof(uint8_t) * total_num_stations);
  //memset(waiting_stations_1, 0, sizeof(uint8_t) * total_num_stations);

  memset(led_to_station_map, -1, sizeof(uint8_t) * TOTAL_SYSTEM_STATIONS);

  //Serial.println("About to memcpy stations");
  //delay(500);

  //Set list of station circuitIDs to lists passed in as arguments
  memcpy(station_circuits_0, circuit_list_0, sizeof(uint16_t) * total_num_stations); /* FlawFinder: Ignore */
  memcpy(station_circuits_1, circuit_list_1, sizeof(uint16_t) * total_num_stations); /* FlawFinder: Ignore */

  memcpy(station_leds, led_list, sizeof(uint8_t) * total_num_stations); /*FlawFinder: Ignore */

  //Given system-wide led number, map to that station's index in this line.
  for(uint8_t i=0; i<total_num_stations; i++){
    led_to_station_map[station_leds[i]] = i;
  }


  //Serial.println("About to memcpy clusters");
  //delay(500);

  //Set list of circuitIDs that determine start / end of "clusters"
  //memcpy(cluster_edges_0, edges_0, sizeof(uint16_t) * num_clusters * 2); /* FlawFinder: Ignore */
  //memcpy(cluster_edges_1, edges_1, sizeof(uint16_t) * num_clusters * 2); /* FlawFinder: Ignore */

  //Serial.println("Done memcpy");
  //delay(500);

  /*
  if( (end_led-start_led)+1 != num_stations){
    Serial.println("Error! LEDs != number of stations");
    //return
  } 
  */

  //first_led = start_led;
  led_color = hex_color;


  //memcpy(leds, leds_list, sizeof(uint8_t) * total_num_stations); /* FlawFinder: Ignore */

  opp_dir_1st_cid[0] = station_circuits_1[0];
  opp_dir_1st_cid[1] = station_circuits_0[0];

  cycles_at_end[0] = 0;
  cycles_at_end[1] = 0;

  state = 0;

  last_station_waiting[0] = false;
  last_station_waiting[1] = false;

}
//END overloaded constructor

/*Indicate train has arrived at station by incrementing waiting station to next station
* Only updates if there is is not another train already at station.

* Returns new index of waiting station or size of station list if station removed.
* Returns -2 for collision with neighbor, -1 for list full (from add), and -1 for improper access
*/
int TrainLine::arrived(uint8_t index, bool dir){
  uint8_t len = lens[dir];
  if(index < len){

    //Update list of waiting trains and leds.
    //If next station is waiting for train, cannot move forward.
    if(waiting_stations[dir][index]+1 != waiting_stations[dir][index+1]){ //TODO: replace with robust circuitID "collision" check.

      //digitalWrite(leds[waiting_stations[dir][index]], 1); //turn led on to show arrival at station. //URGENT: UPDATE LED HANDLING

      //If train arrives at first station, add train to list of waiting trains.
      if(index == 0){
        add(dir);
      }
      //Otherwise, increment train's "position" (station waiting for it) and turn previous LED off.
      else {
        //digitalWrite(leds[waiting_stations[dir][index]-1], 0); //URGENT: UPDATE LED HANDLING
        waiting_stations[dir][index]++;
      }
      return waiting_stations[dir][index];
    }//end else for normal train arrival

    return -2; //return -2 for collision with neighbor
  }//end if
  return - 1;
}//end updates

//Set current state of waiting stations based on array of active train positions and station circuit numbers.
//TODO: Error handling. Make efficient (binary search?)
int TrainLine::setInitialStations(uint16_t *train_positions, uint8_t train_len, bool dir){
  
  uint8_t len = lens[dir];
  //Can only set initial state if no active trains on tracks
  if (len != 1){
    return -1;
  }

  //Iterate through each active train. If it's between arriving (w/i 2 circuits) at a given station and arriving at the next station,
  //update state to show it arriving at that station.
  for(uint8_t i=0; i<train_len; i++){
    for(uint8_t j=0; j<total_num_stations-1; j++){

      int8_t coefficient = (dir * -2) + 1; //turns 0 to 1 and 1 to -1
      int16_t tmp_train_pos = coefficient * train_positions[i];
      int16_t tmp_station_circuit = coefficient * station_circuits[dir][j];
      int16_t tmp_nxt_station = coefficient * station_circuits[dir][j+1];

      if(tmp_train_pos >= (tmp_station_circuit - 2) && tmp_train_pos < (tmp_nxt_station -2)){
        //Serial.println("Updating State");
        //digitalWrite(leds[j], 1); //update LED at current station  //URGENT: UPDATE LED HANDLING
        insert(j+1, dir); //Add next station to list of waiting stations
        break;
      }
    }//end loop through station circuit list
  }//end loop through active trains

  return 0;
}//end SetState

//Given a circuit, use it to update line's state.
int TrainLine::setTrainState(uint16_t circID, uint8_t train_dir){

  //If train in one of the line's positive direction cluster's, update state

  //If waiting for last station and train shows up at opposite direction's 1st station (but going in current direction)
  if(circID == getOppCID(train_dir) && (last_station_waiting[train_dir] == true) ){

    uint16_t last_station_idx;
    if(train_dir == 0) { last_station_idx = total_num_stations-1;}
    else{last_station_idx = 0;}

    //Serial.printf("CircuitID %d setting station %d\n", circID, last_station_idx);
    state |= (1 << last_station_idx);
    cycles_at_end[train_dir]++;
    last_station_waiting[train_dir] = false;
    return last_station_idx;
  }

  int8_t cf = (train_dir * -2) + 1; //turns 0 to 1 and 1 to -1 - allows for same comparison logic when train moves negatively

  //Otherwise, if not within direction's range, exit and return -1

  /*
  if(circID*cf < station_circuits[train_dir][0]*cf || circID*cf > station_circuits[train_dir][total_num_stations-1]*cf){
    return -1;
  }
  */

  //If arriving at last station, update state and note arrival to keep LED on momentarily
  if( (circID*cf >= (getLastCID(train_dir)*cf)-2) &&
   (circID*cf < (getLastCID(train_dir)*cf)+4) &&
   (last_station_waiting[train_dir] == true)) {

    uint16_t last_station_idx;
    if(train_dir == 0) { last_station_idx = total_num_stations-1;}
    else{last_station_idx = 0;}

    Serial.printf("CircuitID %d setting station %d\n", circID, last_station_idx);
    state |= (1 << last_station_idx);
    cycles_at_end[train_dir]++;
    last_station_waiting[train_dir] = false;
    return last_station_idx;
  }

  //Yellow line bridge from pentagon to L'enfont has very weird circuitIDs, Checking for circuits in-between
  //excludes bridge IDs and includes IDs of stations later in line. Check here and set appropriate station.
  if( (circID >= 1050 && circID <= 1054) || (circID >= 3105 && circID <= 3123) || (circID == 2229) ) {
    state |= 1 << 7;
    return 7;
  }

  if( (circID <= 2366 && circID >= 2362) || (circID <= 3145 && circID >= 3124)){
    state |= 1 << 8;
    return 8;
  }

  //Otherwise, loop through all station positions for direction and set state if in between two of them.
  for(uint8_t i=0; i<total_num_stations-1; i++){

    if( circID*cf >= ((station_circuits[train_dir][i]*cf)-2) && circID*cf < ((station_circuits[train_dir][i+1]*cf)-2) ){

      //Do not check in between yellow line bridge stations. Gets false positives and negatives.
      if( !(station_circuits[train_dir][i] == 1052 || station_circuits[train_dir][i] == 2364)){

        uint8_t station_idx;
        if(train_dir == 0){station_idx = i;}
        else if(train_dir == 1){station_idx = (total_num_stations-i)-1;} //e.g. for 10-station line, "2nd" station in reverse has i == 1, need to shift bit 8 times to reach.
        
        state |= 1 << station_idx;


        Serial.printf("CircuitID: %d, Station index: %d, LED number %d\n", circID, station_idx, station_leds[station_idx]);

        //If "at" 2nd to last station, set last station waiting to true.
        if(i == total_num_stations-2){
          Serial.println("Setting last station waiting");
          last_station_waiting[train_dir] = true;
        }

        return station_idx;
      }
    }//end check if train between stations

    //Occasionally, lines are broken into different circuitID segments, with higher ID numbers before lower numbers.
    //Check if moving to a new segment. If so, check if train's circuit is in one of edge cases 
    if(station_circuits[train_dir][i]*cf > station_circuits[train_dir][i+1]*cf){

      /* Specific stations affected and where circuitIDs jump

      N01(3238)->K05(2844) : 3280->2830 (SV->SV/OR)
      K05(3001)->N01(3377) : 2988->3419 (SV/OR->SV)

      K01(2911)->C05(1092) : 2927->1090 (SV/OR->SV/OR/BL)
      C05(1285)->K01(3061) : 1283->3076 (SV/OR.BL -> SV/OR)

      J02(2634)->C13(969)  : 2673->966  (BL->BL/YL)
      C13(1162)->J02(2709) : 1159->2752 (BL/YL->BL)

      F01(2246)->E01(1753) : 2246->1744 (Gallery Place->Mt. Vernon; GR/YL)
      F01(1899)->F02(2376) : 1899->2380

      N07(3627)->N06(3155) : 3630->3146 (Silver Line extension)
      N06(3290)->N07(3741) : 3281->3744

      */
     //Arrays of {<2 circuits before departing station>, <last circuit before jump>, <first circuit after jump>, <2 circuits before arriving station>}
      uint16_t N_to_K_0_circuits[4] = {3236, 3280, 2830, 2842};
      uint16_t K_to_C_0_circuits[4] = {2909, 2927, 1090, 1090};
      uint16_t J_to_C_0_circuits[4] = {2632, 2673, 966, 967};
      uint16_t F_to_E_0_circuits[4] = {2244, 2246, 1744, 1751};
      uint16_t N_to_N_0_circuits[4] = {3625, 3630, 3146, 3153};

      uint16_t K_to_N_1_circuits[4] = {3003, 2988, 3419, 3379};
      uint16_t C_to_K_1_circuits[4] = {1287, 1283, 3076, 3063};
      uint16_t C_to_J_1_circuits[4] = {1164, 1159, 2752, 2711};
      uint16_t E_to_F_1_circuits[4] = {1901, 1899, 2380, 2378};
      uint16_t N_to_N_1_circuits[4] = {3292, 3281, 3744, 3743};

      uint16_t* dir_0_jumps[5] = {N_to_K_0_circuits, K_to_C_0_circuits, J_to_C_0_circuits, F_to_E_0_circuits, N_to_N_0_circuits};
      uint16_t* dir_1_jumps[5] = {K_to_N_1_circuits, C_to_K_1_circuits, C_to_J_1_circuits, E_to_F_1_circuits, N_to_N_1_circuits};


      if(train_dir==0){
        for(uint8_t j =0; j<5; j++){
          //Find which jump is for current station by 
          if(station_circuits[train_dir][i] == dir_0_jumps[j][0]+2){

            //If circuit is on either of the two segments between CircuitID jumps,
            //return as current station
            if( (circID >= dir_0_jumps[j][0] && circID <= dir_0_jumps[j][1]) ||
            circID >= dir_0_jumps[j][2] && circID < dir_0_jumps[j][3]){

              state |= 1 << i; //set state to station's index
              return i;

            }//end if for successful check

          }//end for current station check
        }//end loop through all four jump stations
      }//end direction 0 check


      else if(train_dir==1){
        for(uint8_t j=0; j<5; j++){
          //Find which jump is for current station
          if(station_circuits[train_dir][i] == dir_1_jumps[j][0]-2){

            //If circuit is on either of the two segments between CircuitID jumps,
            //return as current station
            if( (circID <= dir_1_jumps[j][0] && circID >= dir_1_jumps[j][1]) ||
            circID <= dir_1_jumps[j][2] && circID > dir_1_jumps[j][3]){

              uint8_t station_idx = (total_num_stations-i)-1;
              state |= 1 << station_idx; //set state to station's index
              return station_idx;

            }//end if for train between stations check


          }//end current station check
        }//end loop through four jump stations
      }//end direction 1 check


    }//end check for train switching segments

  }//end loop through direction's stations

  //Old code for no given direction deleted, in matrix_led_TrainLine
  

  return -1; //If train not on line's valid stations
}//END setTrainState

//If a train is at a station represented by the given led, return true.

//TODO: Cut down on run-time with bounds check on each line's led range. Or, better, create O(1) lookup data structure.
bool TrainLine::trainAtLED(uint8_t led){

  //loop through every station on the line, see if station corresponds with given led.
  //If it does, see if train is at the station as stored in state bit array.

  if(led >= TOTAL_SYSTEM_STATIONS){
    Serial.println("Error: Invalid LED Number");
    return false;
  }

  int8_t station_index = led_to_station_map[led];

  if(station_index == -1){
    return false;
  }

  else{
    return ((1 << station_index) & state); //returns 1 if train at station, else 0.
  }

  return false;
}//END trainAtLED

uint32_t TrainLine::getLEDColor(){
  return led_color;
}

//Keep led on for last station in line for 3 cycles after arriving, then turn off. Call after looping through API data.
void TrainLine::setEndLED(){

  //If train was at end of line, increment through 3 cycles then turn LED off
  for(int8_t dir=0; dir<2; dir++){
    if(cycles_at_end[dir] > 0){
      if(dir == 0){state |= (1 << total_num_stations-1);} //turns led for last station on
      else if (dir ==1){state |= 1;}
      cycles_at_end[dir]++;
      if(cycles_at_end[dir] == 4){cycles_at_end[dir] = 0;} //After three cycles, reset to 0 so LED turns off in all future cycles.
    }
  }

}//END setEndLED

//Clear line's state. Call setting LEDs after each API call
void TrainLine::clearState(){
  state = 0;
}//end clearState

void TrainLine::updateLEDS2(Adafruit_LEDBackpack &matrix){
/*
  Serial.printf("State at start of LED update: %d\n", state);
  
  //If train was at end of line, increment through 3 cycles then turn LED off
  for(int8_t dir=0; dir<2; dir++){

    if(cycles_at_end[dir] > 0){
      if(dir == 0){state = state | (1 << total_num_stations-1);}
      else if (dir ==1){state = state | 1;}
      cycles_at_end[dir]++;
      if(cycles_at_end[dir] == 4){cycles_at_end[dir] = 0;} //After three cycles, reset to 0 so LED turns off in all future cycles.
    }
  }

  //Go through every station on line, turning LED on if train there and off if not
  for(uint16_t i=0; i<total_num_stations; i++){
    uint8_t led_position = i + first_led;
    uint8_t arr = floor(led_position/16);
    matrix.displaybuffer[arr] |= ((state & 1) << (led_position % 16)); // move bit representing station's state to it's appropriate LED position.
    //Serial.printf("Station %d train state: %d\n", i, (state & 1));
    //digitalWrite(leds[i], (state & 1) );
    state >>= 1;
  }

  state = 0;

  */
}//end updateLEDs (stateless)

void TrainLine::updateLEDS3(Adafruit_NeoPixel &strip){

  Serial.printf("State at start of LED update: %d\n", state);
  
  //If train was at end of line, increment through 3 cycles then turn LED off
  for(int8_t dir=0; dir<2; dir++){

    if(cycles_at_end[dir] > 0){
      if(dir == 0){state |= (1 << total_num_stations-1);} //turns led for last station on
      else if (dir ==1){state |= 1;}
      cycles_at_end[dir]++;
      if(cycles_at_end[dir] == 4){cycles_at_end[dir] = 0;} //After three cycles, reset to 0 so LED turns off in all future cycles.
    }
  }

  //Go through every station on line, turning LED on if train there and off if not
  for(uint16_t i=0; i<total_num_stations; i++){
    uint8_t led_position = station_leds[i]; //get led number from pre-set list of led numbers aligned to each station index.

    //If train at station (station's bit in state set to 1), turn led station's color.
    if(state & 1){
      strip.setPixelColor(led_position, led_color);
    }

    else{
      strip.setPixelColor(led_position, 0); //set to off
    }

    //strip.setPixelColor(led_position)

    //matrix.displaybuffer[arr] |= ((state & 1) << (led_position % 16)); // move bit representing station's state to it's appropriate LED position.
    //Serial.printf("Station %d train state: %d\n", i, (state & 1));
    //digitalWrite(leds[i], (state & 1) );
    state >>= 1; //shift state to next "station" on track
  }

  state = 0;

}//end updateLEDS3 (neopixel strip)


//Update LED display based on bi-directional current state
void TrainLine::updateLEDS(){
  uint16_t state = 0;
  
  //For each waiting station, train is "at" station before it. Set bit in state to reflect current station's index.
  for(int8_t i=1; i<lens[0]; i++){
    int8_t station = waiting_stations_0[i] - 1; //e.g. if waiting station is 3, current station is 2. 1 << 2 == 0x0100 (representing stations 3, 2, 1, and 0). 
    state = state | (1 << station); //add station to state using OR operator
  }

  //Put trains moving in opposite direction into state based on distance from end
  for(int8_t i=1; i<lens[1]; i++){
    int8_t station = (total_num_stations - waiting_stations_1[i]); //e.g. if waiting station(real) is 3(6), current station is 2(7). 1 << 7 0x0010000000 (stations 0(9), 1(8), 2(7), ...)
    state = state | (1 << station);
  }

  Serial.println(String(state));


  //If a train is at the end of either direction, set direction's last bit on
  if(cycles_at_end[0] > 0){
    uint16_t tmp = 1 << (total_num_stations - 1);
    state = state | tmp;
  }

  if(cycles_at_end[1] > 0){
    state = state | 1;
  }

  //Go through every station on line, turning LED on if train there and off if not
  for(uint16_t i=0; i<total_num_stations; i++){
    //Serial.printf("Station %d train state: %d\n", i, (state & 1));
    //digitalWrite(leds[i], (state & 1) );
    state = state >> 1;
  }

}//end updateLEDs

//Get cycles train has been at end of line.
uint8_t TrainLine::getCyclesAtEnd(bool dir){
  return cycles_at_end[dir];
}

//Increment, and reset if necessary, cycles train has been at end of line.
int TrainLine::incrementCyclesAtEnd(bool dir){
  cycles_at_end[dir]++;
  if(cycles_at_end[dir] >=4){cycles_at_end[dir] = 0;}

  return cycles_at_end[dir];
}

//Return pointer to list of waiting stations
uint8_t* TrainLine::getStations(bool dir){
  return waiting_stations[dir];
}

//Returns state of waiting stations as comma separated list of station indexes
String TrainLine::getState(){
  String state = "";
  for(uint8_t j=0; j<2; j++){
    uint8_t len = lens[j];
    state += "Direction " + String(j) + ": ";
    for(int i=0; i<len; i++){
      state += String(waiting_stations[j][i]);
      if (i != len-1){state += ", ";}
      else {state += "\n";}
    }
  }
  return state;
}//end getState

String TrainLine::getState(bool dir){
  String state = "";
  for(int i=0; i<lens[dir]; i++){
    state += String(waiting_stations[dir][i]);
    if (i != lens[dir]-1){state += ", ";}
  }
  return state;  
}

//Length getter
uint8_t TrainLine::getLen(bool dir){
  return lens[dir];
}

//total_num_stations getter
uint8_t TrainLine::getTotalNumStations(){
  return total_num_stations;
}

//return circuitID for the "1st" station in opposite direction (last station in cur direction)
uint16_t TrainLine::getOppCID(bool dir){
  return station_circuits[1-dir][0]; //Get 1st station circuit for direction opposite of one given
}

//returns circuitID of last station on track
uint16_t TrainLine::getLastCID(bool dir){
  return station_circuits[dir][total_num_stations-1];
}

//Return index of station at a given waiting_stations index
int8_t TrainLine::at(uint8_t index, bool dir){
  if(index >= lens[dir]){ //no negative bounds check b/c unsigned (negative int -> large unsigned)
    return -1;
  }
  return waiting_stations[dir][index];
}

//Index operator wraps at() function
/*
int8_t TrainLine::operator[](uint8_t index){
  return at(index);
}
*/

//get circuit for station based on index in waiting stations
int16_t TrainLine::getWaitingStationCircuit(uint8_t index, bool dir){

  if(index < lens[dir]){
    return station_circuits[dir][waiting_stations[dir][index]];
  }
return -1;
}//end getWaitingStationCircuit

//get circuitID of any given station
int16_t TrainLine::getStationCircuit(uint8_t index, bool dir){

  if(index < total_num_stations){
    return station_circuits[dir][index];
  }
return -1;
}

//get String representation of station circuitIDs and LED integers
//TODO: Update to print direction 1 station circuits
String TrainLine::printVariables(){
  String circuits = "";
  //String led_string = "";
  for (uint8_t i=0; i < total_num_stations; i++){
    circuits += String(station_circuits[0][i]) += ", ";
    //led_string += String(leds[i]) += ", ";
  }
  circuits += "\n";
  //led_string += "\n";
  return circuits; //+ led_string;
}

//Deconstructor to free dynamically allocated arrays
TrainLine::~TrainLine(){
  delete[] waiting_stations_0;
  delete[] waiting_stations_1;
  delete[] station_circuits_0;
  delete[] station_circuits_1;

  /*
  delete[] cluster_edges_0;
  delete[] cluster_edges_1;
  */
}

// END FUNCTION IMPLEMENTATION
