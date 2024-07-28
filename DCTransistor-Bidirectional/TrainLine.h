#include "auto_update.h"

/*
    Defines TrainLine class - stores information on train position state on a given line.
    State is set by having multiple circuit IDs (locations) and directions passed in (fetched from WMATA API)
    and converted to being "at" nearest station based on train's current circuitID and track layout.


    State is reset and built from scratch after every API call

    (c) Logan Arkema, 2023

*/

//Define TrainLine member variables and functions.
class TrainLine {

  private:
    // ----- VARIABLES -----

    //static line data
    uint8_t total_num_stations; //number of stations on the line
    uint32_t led_color; //Hex WWRRGGBB color to represent train's on line.
    const char* color; //String of line's color
    const uint8_t* station_leds[2]; //simple array to point to lists of LEDs for each direction
    const uint8_t* station_leds_0; //Array of global LED indexes for LEDs on current line and 'forward' direciton
    const uint8_t* station_leds_1; //Same, but for opposite direction
    const char** station_codes;
    uint16_t end_line_trks[2];

    //Version 1.0 varialbes and functions

    // const uint16_t* station_circuits[2]; //simple array to point to the lists of station circuit IDs for each direction
    // const uint16_t* station_circuits_0; //list for station circuitIDs, trains move positively along circuits
    // const uint16_t* station_circuits_1; //stores station circuits for other direction, trains move negatively along circuits
    // //int setLastTrain(uint8_t train_dir);
    // int setLastTrain(uint16_t circID, uint8_t train_dir);
    // int checkCircuitJumps(uint16_t circID, uint8_t train_dir, uint8_t stn_idx);
    // int checkAllStations(uint16_t circID, uint8_t train_dir);

    //Line state variables
    uint64_t state[2]; //simple binary array of whether or not a train is "at" a given station. One for each line.
    uint8_t num_trains; //Count of trains on the line in current iteration.

    //Arrays that hold specific end-of-line data for each direction
    uint8_t cycles_at_end[2]; //hold how many cycles a train has been at last station
    uint16_t opp_dir_1st_cid[2]; //hold opposite dir's 1st CircuitID. 
    bool last_station_waiting[2]; //check if last station is waiting for a train to arrive from 2nd to last

    // ----- FUNCTIONS -----
    //Private functions called by SetTrainState
    int handleExceptions(char* station_code);

  public:

    //Constructors and Destructor
    TrainLine(uint8_t num_stations, const char* codes[], const char* color_name, uint32_t hex_color, const uint8_t* led_list_0, const uint8_t* led_list_1, const uint16_t end_trk_id_0, const uint16_t end_trk_id_1);
    ~TrainLine();

    // Version 1.0 Functions

    //TrainLine(uint8_t num_stations, const uint16_t* circuit_list_0, const uint16_t* circuit_list_1, const char* color_name, uint32_t hex_color, const uint8_t* led_list_0, const uint8_t* led_list_1);
    // int setTrainState(uint16_t circuitID, uint8_t train_dir);
    // uint8_t* getStations(bool dir);
    // String printVariables(bool dir);
    // uint16_t getOppCID(bool dir);
    // uint16_t getLastCID(bool dir);
    // int16_t getStationCircuit(uint8_t index, bool dir); //get circuitID of any given station

    //Functions called by main loop
    int setTrainStateByCode(const char* trkID, uint8_t train_dir);
    void setEndLED(); //For minimally stateful version, set last station's led on if necessary
    bool trainAtLED(uint8_t led); //If a train is at a station represented by the given led, return true.
    void clearState(); //reset state after every API call.
    void defaultShiftDisplay(bool dir, bool train); //function to run state shift function if no live data

    //Getters
    uint32_t getLEDColor();
    const char* getColor();
    uint8_t getTotalNumStations();
    uint8_t getTrainCount();
    uint8_t getLEDForIndex(uint8_t index, uint8_t train_dir);

};//END TrainLine definitiong

// Version 2.0 construction that takes list of Station Codes and maps to them
TrainLine::TrainLine(uint8_t num_stations, const char* codes[], const char* color_name, uint32_t hex_color, const uint8_t* led_list_0, const uint8_t* led_list_1, const uint16_t end_trk_id_0, const uint16_t end_trk_id_1){
  
  //Set state arrays to constant arrays in config file
  total_num_stations = num_stations;

  station_codes = codes; //new uint16_t[total_num_stations];
  station_leds_0 = led_list_0; //new uint8_t[total_num_stations];
  station_leds_1 = led_list_1; //new uint8_t[total_num_stations];

  //Point directional arrays to each list
  station_leds[0] = station_leds_0;
  station_leds[1] = station_leds_1;

  //Set LED color to its own string
  led_color = hex_color;
  color = color_name;

  end_line_trks[0] = end_trk_id_0;
  end_line_trks[1] = end_trk_id_1;

  cycles_at_end[0] = 0;
  cycles_at_end[1] = 0;

  state[0] = 0;
  state[1] = 0;

  last_station_waiting[0] = false;
  last_station_waiting[1] = false;

  num_trains = 0;
}


// VERSION 1.0 CODE
/*

//Overloaded constructor that takes list of track circuitIDs corresponding to each station on line
TrainLine::TrainLine(uint8_t num_stations, const uint16_t* circuit_list_0, const uint16_t* circuit_list_1, const char* color_name, uint32_t hex_color, const uint8_t* led_list_0, const uint8_t* led_list_1){
  
  //Serial.println("In constructor");

  //Set state arrays to constant arrays in config file
  total_num_stations = num_stations;
  station_circuits_0 = circuit_list_0; //new uint16_t[total_num_stations];
  station_circuits_1 = circuit_list_1; //new uint16_t[total_num_stations];
  station_leds_0 = led_list_0; //new uint8_t[total_num_stations];
  station_leds_1 = led_list_1; //new uint8_t[total_num_stations];

  //Point directional arrays to each list
  station_circuits[0] = station_circuits_0;
  station_circuits[1] = station_circuits_1;
  station_leds[0] = station_leds_0;
  station_leds[1] = station_leds_1;

  //Set LED color to its own string
  led_color = hex_color;
  color = color_name;

  //Set opposite direction's 1st circuit and empty state data
  opp_dir_1st_cid[0] = station_circuits_1[0];
  opp_dir_1st_cid[1] = station_circuits_0[0];

  cycles_at_end[0] = 0;
  cycles_at_end[1] = 0;

  state[0] = 0;
  state[1] = 0;

  last_station_waiting[0] = false;
  last_station_waiting[1] = false;

  num_trains = 0;
}

//Call after checking that train arriving at last station has shown up
int TrainLine::setLastTrain(uint16_t circID, uint8_t train_dir){

    //int last_station_idx;
    //if(train_dir == 0) { last_station_idx = total_num_stations-1;}
    //else{last_station_idx = 0;}

    uint8_t last_station_idx = total_num_stations-1; // FOR SEPARATE STATE ARRAYS

    #ifdef PRINT
      Serial.printf("CircuitID %d setting station %d\n", circID, last_station_idx);
    #endif

    uint64_t one = 1; // 64-bit otherwise overflow with Silver line.

    state[train_dir] |= (one << last_station_idx);
    cycles_at_end[train_dir]++;
    last_station_waiting[train_dir] = false;
    return last_station_idx;
}//END setLastTrain

//Some Trains "jump" from consecutive increasing / decreasing CircuitIDs.
//After checking if two stations have a "jump" between them, check if CircuitID is within one of the jumps
int TrainLine::checkCircuitJumps(uint16_t circID, uint8_t train_dir, uint8_t stn_idx){

  //  Specific stations affected and where circuitIDs jump

  // N01(3238)->K05(2844) : 3280->2830 (SV->SV/OR)
  // K05(3001)->N01(3377) : 2988->3420 (SV/OR->SV)

  // K01(2911)->C05(1092) : 2927->1090 (SV/OR->SV/OR/BL)
  // C05(1285)->K01(3061) : 1283->3076 (SV/OR/BL -> SV/OR)

  // J02(2634)->C13(969)  : 2673->966  (BL->BL/YL)
  // C13(1162)->J02(2709) : 1159->2752 (BL/YL->BL)

  // F01(2246)->E01(1753) : 2246->1744 (Gallery Place->Mt. Vernon; GR/YL)
  // F01(1899)->F02(2376) : 1899->2380

  // N07(3627)->N06(3155) : 3630->3146 (Silver Line extension)
  // N06(3290)->N07(3741) : 3281->3744

  //

  //Check for positive moving trains
  if(train_dir==0){

    //Arrays of {<2 circuits before departing station>, <last circuit before jump>, <first circuit after jump>, <2 circuits before arriving station>}
    uint16_t N_to_K_0_circuits[4] = {3236, 3280, 2830, 2842};
    uint16_t K_to_C_0_circuits[4] = {2909, 2927, 1090, 1090};
    uint16_t J_to_C_0_circuits[4] = {2632, 2673, 966, 967};
    uint16_t F_to_E_0_circuits[4] = {2244, 2246, 1744, 1751};
    uint16_t N_to_N_0_circuits[4] = {3625, 3630, 3146, 3153};
    uint16_t* dir_0_jumps[5] = {N_to_K_0_circuits, K_to_C_0_circuits, J_to_C_0_circuits, F_to_E_0_circuits, N_to_N_0_circuits};

    for(uint8_t j =0; j<5; j++){
      //Find which jump is for current station by comparing station circuit to two circuits before station
      if(station_circuits[train_dir][stn_idx] == dir_0_jumps[j][0]+2){

        //If circuit is on either of the two segments between CircuitID jumps,
        //return as current station
        if( (circID >= dir_0_jumps[j][0] && circID <= dir_0_jumps[j][1]) ||
        circID >= dir_0_jumps[j][2] && circID < dir_0_jumps[j][3]){

          uint64_t one = 1;
          state[0] |= one << stn_idx; //set state to station's index
          return stn_idx;

        }//end if for successful check

      }//end for current station check
    }//end loop through all five jump stations
  }//end direction 0 check

  //Check for negative moving trains
  else if(train_dir==1){

    //Arrays of {<2 circuits before departing station>, <last circuit before jump>, <first circuit after jump>, <2 circuits before arriving station>}
    uint16_t K_to_N_1_circuits[4] = {3003, 2988, 3420, 3379};
    uint16_t C_to_K_1_circuits[4] = {1287, 1283, 3076, 3063};
    uint16_t C_to_J_1_circuits[4] = {1164, 1159, 2752, 2711};
    uint16_t E_to_F_1_circuits[4] = {1901, 1899, 2380, 2378};
    uint16_t N_to_N_1_circuits[4] = {3292, 3281, 3744, 3743};
    uint16_t* dir_1_jumps[5] = {K_to_N_1_circuits, C_to_K_1_circuits, C_to_J_1_circuits, E_to_F_1_circuits, N_to_N_1_circuits};

    for(uint8_t j=0; j<5; j++){
      //Find which jump is for current station by comparing station circuit to two circuits before station
      if(station_circuits[train_dir][stn_idx] == dir_1_jumps[j][0]-2){

        //If circuit is on either of the two segments between CircuitID jumps,
        //return as current station
        if( (circID <= dir_1_jumps[j][0] && circID >= dir_1_jumps[j][1]) ||
        circID <= dir_1_jumps[j][2] && circID > dir_1_jumps[j][3]){

          uint64_t one = 1;
          state[1] |= one << stn_idx; //set state for reverse direction.
          return stn_idx;

          //uint8_t station_idx = (total_num_stations-stn_idx)-1;
          //state |= 1 << station_idx; //set state to station's index
          //return station_idx;

        }//end if for train between stations check


      }//end current station check
    }//end loop through four jump stations
  }//end direction 1 check

  return -1;
}//END checkCircuitJumps

//If no special condition met, check where Train's CircuitID is between all stations on line
int TrainLine::checkAllStations(uint16_t circID, uint8_t train_dir){

  uint64_t station_idx = 0;
  int8_t cf = (train_dir * -2) + 1; //turns 0 to 1 and 1 to -1

  //Loop through all station positions for direction and set state if in between two of them.
  for(uint8_t i=0; i<total_num_stations-1; i++){

    //Check if circuit is in between two before current station and two before next station
    if( circID*cf >= ((station_circuits[train_dir][i]*cf)-CIRCS_BEFORE_STATION) && 
    circID*cf < ((station_circuits[train_dir][i+1]*cf)-CIRCS_BEFORE_STATION) ){

      //Do not check in between yellow line bridge stations. Gets false positives and negatives.
      if( !((station_circuits[train_dir][i] == 1052 || station_circuits[train_dir][i] == 2364) && station_circuits_0[0] == 944)){

        //Based on direction of train, get the unified index of the station and update train line's state.
        //if(train_dir == 0){station_idx = i;}
        //else if(train_dir == 1){station_idx = (total_num_stations-i)-1;} //e.g. for 10-station line, "2nd" station in reverse has i == 1, need to shift bit 8 times to reach.

        station_idx = i; //if not 64-bit integers, will overflow for silver line
        uint64_t one = 1;
        
        state[train_dir] |= one << station_idx; //For separate direction states, just i works. One direction not starting at end of array.
        //state |= 1 << station_idx;

        #ifdef PRINT
          Serial.printf("CircuitID: %d, Station index: %d, LED number %d\n", circID, station_idx, station_leds[train_dir][station_idx]);
        #endif

        //If "at" 2nd to last station, set last station waiting to true.
        if(i == total_num_stations-2){
          #ifdef PRINT
            Serial.println("Setting last station waiting");
          #endif
          last_station_waiting[train_dir] = true;
        }

        return station_idx;
      }
    }//end check if train between stations

    //Occasionally, lines are broken into different circuitID segments, with higher ID numbers before lower numbers.
    //Check if moving to a new segment. If so, check if train's circuit is in one of edge cases 
    if(station_circuits[train_dir][i]*cf > station_circuits[train_dir][i+1]*cf){
      if ((station_idx = checkCircuitJumps(circID, train_dir, i)) != -1){
        return station_idx;
      }
      
    }//end check for train switching segments

  }//end loop through direction's stations
  return -1;

}//END checkAllStations

//Given a circuit, use it to update line's state after checking number of conditions
int TrainLine::setTrainState(uint16_t circID, uint8_t train_dir){

  num_trains++; //Increment number of trains on line

  //If waiting for last station and train shows up at opposite direction's 1st station (but going in current direction)
  if(circID == getOppCID(train_dir) && (last_station_waiting[train_dir] == true) ){
    return setLastTrain(circID, train_dir);
  }

  //Set coefficient to multiple circuits by for uniform comparisons (whether trains move + or - along circuits)
  int8_t cf = (train_dir * -2) + 1; //turns 0 to 1 and 1 to -1

  //If arriving at last station, update state and note arrival to keep LED on momentarily
  if( (circID*cf >= (getLastCID(train_dir)*cf)-CIRCS_BEFORE_STATION) &&
   (circID*cf < (getLastCID(train_dir)*cf)+4) &&
   (last_station_waiting[train_dir] == true)) {

    return setLastTrain(circID, train_dir);
  }

  //Yellow line bridge from pentagon to L'enfont has very weird circuitIDs, Checking for circuits in-between
  //excludes bridge IDs and includes IDs of stations later in line. Check here and set appropriate station.
  if (station_circuits_0[0] == 944){
    if( (circID >= 1050 && circID <= 1054) || (circID >= 3105 && circID <= 3123) || (circID == 2229)) {
      
      state[0] |= 1 << 8;
      return 8;
      //state |= 1 << 8;
      //return 8;
    }

    if( (circID <= 2366 && circID >= 2362) || (circID <= 3145 && circID >= 3124))  {
      
      state[1] |= 1 << 4; //Added shaw as station so last station doesn't break other direction.
      return 4;
      //state |= 1 << 9;
      //return 9;
    }
  }

  //Otherwise, loop through all station positions for direction and set state if in between two of them (or return -1)
  return checkAllStations(circID, train_dir);
  

}//END setTrainStates

//Keep led on for last station in line for 3 cycles after arriving, then turn off. Call after looping through API data.
void TrainLine::setEndLED(){

  //If train was at end of line, increment through CYCLES_AT_END cycles then turn LED off
  for(int8_t dir=0; dir<2; dir++){

    if(cycles_at_end[dir] > 0){
      //turns led for last station on (direction determines "last")
      
      //if(dir == 0){state |= (1 << total_num_stations-1);}
      //else if (dir ==1){state |= 1;}
      uint64_t one = 1;
      state[dir] |= one << total_num_stations-1;
      
      //Increment cycles train has been at end of line, and set to 0 after 3.
      cycles_at_end[dir]++;
      if(cycles_at_end[dir] == 4){cycles_at_end[dir] = 0;} //After three cycles, reset to 0 so LED turns off in all future cycles.
    }
  }

// }//END setEndLED

//return circuitID for the "1st" station in opposite direction (last station in cur direction)
uint16_t TrainLine::getOppCID(bool dir){
  return station_circuits[1-dir][0]; //Get 1st station circuit for direction opposite of one given
}

//returns circuitID of last station on track
uint16_t TrainLine::getLastCID(bool dir){
  return station_circuits[dir][total_num_stations-1];
}

//get circuitID of any given station
int16_t TrainLine::getStationCircuit(uint8_t index, bool dir){

  if(index < total_num_stations){
    return station_circuits[dir][index];
  }

  return -1;
}

//get String representation of station circuitIDs
String TrainLine::printVariables(bool dir){
  String circuits = "";
  for (uint8_t i=0; i < total_num_stations; i++){
    circuits += String(station_circuits[dir][i]) += ", ";
  }
  circuits += "\n";
  return circuits;
}

*/

//Handle Track IDs that do not map to a station code.
int TrainLine::handleExceptions(char* station_code){

  //Red Line Exceptions
  if(!strcmp(color, "Red") && !strcmp(station_code, "B99")){
    return 18; // Index for B35 / NoMa
  }

  // Blue Line Exceptions + King St. (Both Blue and Yellow - same index)
  else if(!strcmp(color, "Blue") || !strcmp(color, "Yellow")){

    // Applies to both Blue and Yellow Lines
    if(!strcmp(station_code, "C97")){
      return 2; //Index for C13 / King St. *on both lines*
    }

    else if(!strcmp(station_code, "J01") || !strcmp(station_code, "C98")){
      return 1; //Index for J02 / Van Dorn
    }

    else if(!strcmp(station_code, "D98")){
      return 23; //Index for G01 / Benning Rd.
    }
  }


  else if(!strcmp(color, "Orange")){

    if(!strcmp(station_code, "K98")){
      return 2; //Index for K03 / West Falls Church
    }

    else if(!strcmp(station_code, "D98")){
      return 21; //Index for D09 / Minnesota Ave
    }

  }

  // Silver line exceptions
  else if(!strcmp(color, "Silver")){

    if(!strcmp(station_code, "N98A") || !strcmp(station_code, "N98B")){
      return 1; //Loudoun Gateway / N11
    }

    else if(!strcmp(station_code, "N97")){
      return 2; //Dulles / N10
    }

    else if(!strcmp(station_code, "N96")){
      return 3; //Innovation Center / N9
    }

    else if(!strcmp(station_code, "N95") || !strcmp(station_code, "N94") || !strcmp(station_code, "N93") ){
      return 6; //Wiehle Reston / N06
    }

    else if(!strcmp(station_code, "N05")){
      return 7; //Spring Hill / N04
    }

    else if(!strcmp(station_code, "N92") || !strcmp(station_code, "N91") || !strcmp(station_code, "K98") ){
      return 10; //Wiehle Reston / N06
    }

    else if(!strcmp(station_code, "D98")){
      return 29; //Index for G01 / Benning Rd.
    }

  }

  return -1;
}

//Given a Track Id from GIS API, parse out and set appropriate station code
int TrainLine::setTrainStateByCode(const char* trkID, uint8_t train_dir){

  //Convert full TrackID (e.g. A01-A2-132) to just station code - A01
  const char* dash_delim = "-";
  char* station_code = NULL;
  char* trk_id = NULL;
  char tmp_trkID_string[20] = {0};

  //Copy trkID into editable string then parse station code (before first dash) and track ID (after second)
  strncpy(tmp_trkID_string, trkID, strlen(trkID)+1);
  station_code = strtok(tmp_trkID_string, dash_delim);
  strtok(NULL, dash_delim);
  trk_id = strtok(NULL, dash_delim);

  int64_t station_idx = 0;
  uint64_t one = 1;

  // Check if code does not map neatly onto station and assign station if not.
  if((station_idx=handleExceptions(station_code)) != -1){

    state[train_dir] |= one << station_idx;
    num_trains++;
    return (int)station_idx;
  } 

  // Otherwise, loop through line's list of station codes and check for match
  for(uint8_t i=0; i<total_num_stations; i++){

    // On match, set state to reflect train's presence
    if(!strcmp(station_codes[i], station_code)){

      // Check if train is at the end of its line.
      // First, check if train is at track ID where trains linger, and remove if so
      if( (train_dir == 0 && i == total_num_stations-1) || (train_dir == 1 && i == 0)){

        // If train's track ID is track where trains sit and do nothing, remove.
        if(trk_id != NULL){
          int trk_id_int = atoi(trk_id);
          if(trk_id_int >= end_line_trks[train_dir]){
            return -1;
          }
        }

        // Otherwise, increment cycles for train at end of line
        cycles_at_end[train_dir]++;
      }

      // For all matches, update state and return station's index
      station_idx = i;
      state[train_dir] |= one << station_idx;

      num_trains++;
      return i;
    }

  }

  return -1;
}


//For a given board-wide LED, get that station's position on current train line's track (if any), and return
//state (train or no train) at that station.
bool TrainLine::trainAtLED(uint8_t led){

  if(led >= TOTAL_SYSTEM_STATIONS){
    Serial.printf("Error: Invalid LED Number: %d\n", led);
    return false;
  }

  //int8_t station_index = led_to_station_map_0[led];

  uint64_t one = 1; //must declare as 64-bit integer, otherwise will assume 32-bit and overflow for silver line 

  //If LED used in either direction, return that LED's state
  for(uint8_t i=0; i<total_num_stations; i++){
    if (station_leds_0[i] == led){
      //Serial.printf("LED: %d;  Station: %d;  State: %d\n", led, i, state[0]);
      return (one << i) & state[0];
    }
    else if (station_leds_1[i] == led){
      return (one << i) & state[1];
    }
  }

  //Return false if led / station not found in loop
  return false;

}//END trainAtLED

//Get current line's LED color (defined at construction time)
uint32_t TrainLine::getLEDColor(){
  return led_color;
}

//Get current line's line color as full string
const char* TrainLine::getColor(){
  return color;
}

// Get the LED Number for an index on the station's line
uint8_t TrainLine::getLEDForIndex(uint8_t index, uint8_t train_dir){
  return station_leds[train_dir][index];
}

// Function with 2.0 Refactor to turn stale end-of-line LEDS off and reset them
void TrainLine::setEndLED(){

  //If train was at end of line, increment through CYCLES_AT_END cycles then turn LED off
  for(int8_t dir=0; dir<2; dir++){

    uint64_t station_idx=0;
    uint64_t one = 1;
    if (dir == 0){station_idx = total_num_stations-1;}

    #ifdef PRINT
      Serial.printf("Line: %s; Dir: %d; Cycles: %d; Present:%d\n", 
        color, dir, cycles_at_end[dir], (bool)(state[dir] & (one << station_idx)) );
    #endif

    //If train not at station, but cycles are set, reset to 0
    if( (!((one << station_idx) & state[dir])) && (cycles_at_end[dir] > 0)){
      cycles_at_end[dir] = 0;
    }

    //If over cycles at end, remove train from State before showing LEDs
    if( cycles_at_end[dir] > CYCLES_AT_END ){
      state[dir] -= (one << station_idx);
    }

  }
}

// Shift the state for a given direction one if train "set" to arrive.
void TrainLine::defaultShiftDisplay(bool dir, bool train){
  state[dir] = state[dir] << 1;
  if(train){state[dir] |= 1;}
}

//Clear line's state. Call after setting LEDs after each API call
void TrainLine::clearState(){
  state[0] = 0;
  state[1] = 0;
  num_trains = 0;
}//end clearState

//total_num_stations getter
uint8_t TrainLine::getTotalNumStations(){
  return total_num_stations;
}

//number of current trains getter
uint8_t TrainLine::getTrainCount(){
  return num_trains;
}

//Deconstructor to free dynamically allocated arrays
TrainLine::~TrainLine(){
  delete[] station_leds_0;
  delete[] station_leds_1;
  //free(color);
}

// END FUNCTION IMPLEMENTATION
