#include <Arduino.h>
/*
    Defines TrainLine class - an evolution of SimpleList. 
    Data structure optimized to store which stations on a line are waiting for a train to arrive.
    and updates accordingly. Represents stations using index (position of station on given line) for simplicity.

    Designed to compile on Desktop (using EpoxyDuino) and Arduino for easy and integrated unit testing.
*/

//Define TrainLine member variables and functions.
class TrainLine {

  private:
    //variables
    uint8_t total_num_stations; //number of stations on the line
    uint8_t* waiting_stations; //list of integers representing station indexes /*Flawfinder: ignore */
    uint16_t* station_circuits; //list of track circuits corresponding to each station
    uint8_t len;
    uint8_t cycles_at_end;
    uint16_t opp_dir_1st_cid;

    uint8_t* leds;

    //state updating functions
    int add();
    int insert(uint8_t index);
  
  public:

    //Constructors and Destructor
    TrainLine();
    TrainLine(uint8_t num_stations, uint16_t* circuit_list, uint8_t* leds_list, uint16_t opp_cid);
    ~TrainLine();

    //State updating functions
    int arrived(uint8_t index);
    int setInitialStations(uint16_t* train_positions, uint8_t train_len);
    int remove();

    int incrementCyclesAtEnd(); //returns updated number of cycles.

    //Getters
    uint8_t* getStations();
    String getState();
    String printVariables();
    uint8_t getLen();
    uint8_t getTotalNumStations();
    uint8_t getCyclesAtEnd();
    uint16_t getOppCID();
    uint16_t getLastCID();
    int8_t at(uint8_t index);
    int8_t operator[](uint8_t index);

    int16_t getWaitingStationCircuit(uint8_t index); //get circuit for station based on index in waiting stations
    int16_t getStationCircuit(uint8_t index); //get circuitID of any given station

};//END SimpleList definition

//Add new station to waiting station list to represent new train. New train only added when train arrives at 1st station.
//Since 1st station (index 0) always waiting, insert new train into 2nd position and copy all other values over one.
//Returns new size of waiting stations list, or -1 if out-of-bounds (list is full)
int TrainLine::add(){
  if( len < total_num_stations ){
    if(len == 1){
      waiting_stations[len] = 1;
    }
    else{
      uint8_t* buf = new uint8_t[len-1];
      memcpy(buf, waiting_stations+1, (len-1)); /*Flawfinder: ignore */
      waiting_stations[1] = 1;
      memcpy(waiting_stations+2, buf, (len-1)); /*Flawfinder: ignore */
      delete[] buf;
    }
    len++;
    return len;
  }
  else{
    return -1; 
  } 
}//end add

//Insert a station into list of waiting stations using insertion sort. 
//Only called when initially setting line state, when order of inserted stations is unknown
//Not optimized for insertion sort b/c only run once per startup.
int TrainLine::insert(uint8_t station_num){
  //Serial.println("In Insert");
  //Serial.printf("Current State: %s\tCurrent Len: %d\n", getState(), len);
  if(len == 1){
    waiting_stations[1] = station_num;
    len++;
    return 0;
  }

  else if(station_num > waiting_stations[len-1]){
    waiting_stations[len] = station_num;
    len++;
    return 0;
  }

  for(int8_t i = 0; i < len; i++){
    if(station_num < waiting_stations[i]){
      //Serial.printf("Station %d is smaller than station %d. Inserting it at index %d\n", station_num, waiting_stations[i], i);
      if(len-i > 0){
        uint8_t* buf = new uint8_t[len-i]; /*Flawfinder: ignore */
        memcpy(buf, waiting_stations+i, (len-i)); /*Flawfinder: ignore */
        memcpy(waiting_stations+i+1, buf, (len-i)); /*Flawfinder: ignore */
        delete[] buf;
      }//end shift
      waiting_stations[i] = station_num;
      len++;
      return 0;
    }//end if smaller value.
  }
  return -1;
}//end insert

//Remove a train after reaching end of line. Removes end of list (i.e. dequeue)
//Returns size of waiting stations list, 
//Returns -1 if last train not at end of line
int TrainLine::remove(){
  //can only remove if last train is at last station
  if(waiting_stations[len-1] == total_num_stations){
    digitalWrite(leds[waiting_stations[len-1]-1], 0);
    waiting_stations[len-1] = 0;
    len--;
    return len;
  }
  return -1;
}//end remove

//Constructor sets first station as waiting and initializes rest of list to 0.
//Set default values for testing northbound end of redline.
TrainLine::TrainLine(){
  total_num_stations = 10;
  len = 1;
  opp_dir_1st_cid = 868;

  //Set state arrays to number of stations in track.
  waiting_stations = new uint8_t[total_num_stations];
  station_circuits = new uint16_t[total_num_stations];
  leds = new uint8_t[total_num_stations];

  memset(waiting_stations, 0, sizeof(*waiting_stations));
  uint16_t tmp_station_circuits[10] = {485, 496, 513, 527, 548, 571, 591, 611, 629, 652};
  uint8_t tmp_leds[10] = {15, 13, 12, 14, 2, 0, 4, 5, 16, 10};

  memcpy(station_circuits, tmp_station_circuits, sizeof(uint16_t) * total_num_stations);
  memcpy(leds, tmp_leds, sizeof(uint8_t) * total_num_stations);

  cycles_at_end = 0;

  for(uint8_t i=0; i<total_num_stations; i++){
    pinMode(leds[i], OUTPUT);
  }

}

//Overloaded constructor that takes list of track circuitIDs corresponding to each station on line
TrainLine::TrainLine(uint8_t num_stations, uint16_t* circuit_list, uint8_t* leds_list, uint16_t opp_cid){
  total_num_stations = num_stations;
  len = 1;
  opp_dir_1st_cid = opp_cid;

  //Set state arrays to number of stations in track.
  waiting_stations = new uint8_t[total_num_stations];
  station_circuits = new uint16_t[total_num_stations];
  leds = new uint8_t[total_num_stations];

  memset(waiting_stations, 0, sizeof(uint8_t) * total_num_stations);
  memcpy(station_circuits, circuit_list, sizeof(uint16_t) * total_num_stations);
  memcpy(leds, leds_list, sizeof(uint8_t) * total_num_stations);

  cycles_at_end = 0;

  for(uint8_t i=0; i<total_num_stations; i++){
    pinMode(leds[i], OUTPUT);
  }

}

/*Indicate train has arrived at station by incrementing waiting station to next station
* Only updates if there is is not another train already at station.

* Returns new index of waiting station or size of station list if station removed.
* Returns -2 for collision with neighbor, -1 for list full (from add), and -1 for improper access
*/
int TrainLine::arrived(uint8_t index){
  if(index < len){

    //Update list of waiting trains and leds.
    //If next station is waiting for train, cannot move forward.
    if(waiting_stations[index]+1 != waiting_stations[index+1]){ //TODO: replace with robust circuitID "collision" check.

      digitalWrite(leds[waiting_stations[index]], 1); //turn led on to show arrival at station.

      //If train arrives at first station, add train to list of waiting trains.
      if(index == 0){
        add();
      }
      //Otherwise, increment train's "position" (station waiting for it) and turn previous LED off.
      else {
        digitalWrite(leds[waiting_stations[index]-1], 0);
        waiting_stations[index]++;
      }
      return waiting_stations[index];
    }//end else for normal train arrival

    return -2; //return -2 for collision with neighbor
  }//end if
  return - 1;
}//end updates

//Set current state of waiting stations based on array of active train positions and station circuit numbers.
//TODO: Error handling. Make efficient (binary search?)
int TrainLine::setInitialStations(uint16_t *train_positions, uint8_t train_len){
  
  //Can only set initial state if no active trains on tracks
  if (len != 1){
    return -1;
  }

  //Iterate through each active train. If it's between arriving (w/i 2 circuits) at a given station and arriving at the next station,
  //update state to show it arriving at that station.
  for(uint8_t i=0; i<train_len; i++){
    for(uint8_t j=0; j<total_num_stations-1; j++){
      if(train_positions[i] >= (station_circuits[j] - 2) && train_positions[i] < (station_circuits[j+1] -2)){
        //Serial.println("Updating State");
        digitalWrite(leds[j], 1); //update LED at current station
        insert(j+1); //Add next station to list of waiting stations
        break;
      }
    }//end loop through station circuit list
  }//end loop through active trains

  return 0;
}//end SetState

//Get cycles train has been at end of line.
uint8_t TrainLine::getCyclesAtEnd(){
  return cycles_at_end;
}

//Increment, and reset if necessary, cycles train has been at end of line.
int TrainLine::incrementCyclesAtEnd(){
  cycles_at_end++;
  if(cycles_at_end >=4){cycles_at_end = 0;}

  return cycles_at_end;
}

//Return pointer to list of waiting stations
uint8_t* TrainLine::getStations(){
  return waiting_stations;
}

//Returns state of waiting stations as comma separated list of station indexes
String TrainLine::getState(){
  String state = "";
  for(int i=0; i<len; i++){
    state += String(waiting_stations[i]);
    if (i != len-1){state += ", ";}
  }
  return state;
}//end getState

//Length getter
uint8_t TrainLine::getLen(){
  return len;
}

//total_num_stations getter
uint8_t TrainLine::getTotalNumStations(){
  return total_num_stations;
}

//return circuitID for the "1st" station in opposite direction (last station in cur direction)
uint16_t TrainLine::getOppCID(){
  return opp_dir_1st_cid;
}

//returns circuitID of last station on track
uint16_t TrainLine::getLastCID(){
  return station_circuits[total_num_stations-1];
}

//Return index of station at a given waiting_stations index
int8_t TrainLine::at(uint8_t index){
  if(index >= len){ //no negative bounds check b/c unsigned (negative int -> large unsigned)
    return -1;
  }
  return waiting_stations[index];
}

//Index operator wraps at() function
int8_t TrainLine::operator[](uint8_t index){
  return at(index);
}

//get circuit for station based on index in waiting stations
int16_t TrainLine::getWaitingStationCircuit(uint8_t index){

  if(index < len){
    return station_circuits[waiting_stations[index]];
  }
return -1;
}//end getWaitingStationCircuit

//get circuitID of any given station
int16_t TrainLine::getStationCircuit(uint8_t index){

  if(index < total_num_stations){
    return station_circuits[index];
  }
return -1;
}

//get String representation of station circuitIDs and LED integers
String TrainLine::printVariables(){
  String circuits = "";
  String led_string = "";
  for (uint8_t i=0; i < total_num_stations; i++){
    circuits += String(station_circuits[i]) += ", ";
    led_string += String(leds[i]) += ", ";
  }
  circuits += "\n";
  led_string += "\n";
  return circuits + led_string;
}

//Deconstructor to free dynamically allocated arrays
TrainLine::~TrainLine(){
  delete[] waiting_stations;
  delete[] station_circuits;
  delete[] leds;
}

// END FUNCTION IMPLEMENTATION
