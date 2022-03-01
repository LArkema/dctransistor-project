#include <Arduino.h>
/*
    Defines SimpleList class. Data structure optimized to store which stations are waiting for a train to arrive
    and updates accordingly. Represents stations using index (position of station on given line) for simplicity.

    Designed to compile on Desktop (using EpoxyDuino) and Arduino for easy and integrated unit testing.
*/

// TODO: eventually make class variable.
const uint8_t MAX_NUM_STATIONS = 10; //Max number of stations per line

class SimpleList {
  private:
    uint8_t len;
    uint8_t cycles_at_end;

    //Add new station to waiting station list to represent new train. New train only added when train arrives at 1st station.
    //Since 1st station (index 0) always waiting, insert new train into 2nd position and copy all other values over one.
    //Returns new size of waiting stations list, or -1 if out-of-bounds (list is full)
    int add(){
      if( len < MAX_NUM_STATIONS ){
        if(len == 1){
          stations[len] = 1;
        }
        else{
          uint8_t* buf = new uint8_t[len-1];
          memcpy(buf, stations+1, (len-1)); /*Flawfinder: ignore */
          stations[1] = 1;
          memcpy(stations+2, buf, (len-1)); /*Flawfinder: ignore */
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
    //TODO: Optimize - insertion sort?
    int insert(uint8_t station_num){
      Serial.println("In Insert");
      Serial.printf("Current State: %s\tCurrent Len: %d\n", getState(), len);
      if(len == 1){
        stations[1] = station_num;
        len++;
        return 0;
      }

      else if(station_num > stations[len-1]){
        stations[len] = station_num;
        len++;
        return 0;
      }

      for(int8_t i = 0; i < len; i++){
        if(station_num < stations[i]){
          Serial.printf("Station %d is smaller than station %d. Inserting it at index %d\n", station_num, stations[i], i);
          if(len-i > 0){
            uint8_t* buf = new uint8_t[len-i]; /*Flawfinder: ignore */
            memcpy(buf, stations+i, (len-i)); /*Flawfinder: ignore */
            memcpy(stations+i+1, buf, (len-i)); /*Flawfinder: ignore */
            delete[] buf;
          }//end shift
          stations[i] = station_num;
          len++;
          return 0;
        }//end if smaller value.
      }
      return -1;
    }//end insert

    //Remove a train after reaching end of line. Removes end of list (i.e. dequeue)
    //Returns size of waiting stations list, 
    //Returns -1 if last train not at end of line
    int remove(){
      //can only remove if last train is at last station
      if(stations[len-1] == MAX_NUM_STATIONS-1){
        stations[len-1] = 0;
        len--;
        return len;
      }
      return -1;
    }//end remove

  //END PRIVATE
  
  public:
    uint8_t stations[MAX_NUM_STATIONS]; //list of integers representing station indexes /*Flawfinder: ignore */

    //Constructor sets first station as waiting and initializes rest of list to 0.
    SimpleList(){
      len = 1;
      memset(stations, 0, MAX_NUM_STATIONS);
      cycles_at_end = 0;
    }

    /*Indicate train has arrived at station by incrementing waiting station to next station
    * Only updates if there is is not another train already at station.

    * Returns new index of waiting station or size of station list if station removed.
    * Returns -2 for collision with neighbor, -1 for list full (from add), and -1 for improper access
    */
    int arrived(uint8_t index){
      if(index < len){
        //If "arriving" at last station, remove it from active state
        if(stations[index] == MAX_NUM_STATIONS-1){
          remove();
        }
        //If next station is waiting for train, cannot move forward
        else if(stations[index]+1 != stations[index+1]){
          //If train arrives at first station, add train to "track" (i.e. stations waiting for train)
          if(index == 0){
            add();
          }
          else {
            stations[index]++;
          }
          return index+1;
        }//end else for increment
        return -2; //return -2 for collision with neighbor
      }//end if
      return - 1;
    }//end updates

    //Set current state of waiting stations based on array of active train positions and station circuit numbers.
    //TODO: Error handling. Make efficient (binary search?)
    int setInitialStations(uint16_t *train_positions, uint8_t train_len, const uint16_t *station_circuits, uint8_t station_len){
      
      //Can only set initial state if no active trains on tracks
      if (len != 1){
        return -1;
      }

      //Iterate through each active train. If it's between arriving (w/i 2 circuits) at a given station and arriving at the next station,
      //update state to show it arriving at that station.
      for(uint8_t i=0; i<train_len; i++){
        for(uint8_t j=0; j<station_len-1; j++){
          if(train_positions[i] >= (station_circuits[j] - 2) && train_positions[i] < (station_circuits[j+1] -2)){
            Serial.println("Updating State");
            insert(j+1); //Adding to list of waiting stations
            break;
          }
        }//end loop through station circuit list
      }//end loop through active trains

      return 0;
    }//end SetState

    uint8_t* getStations(){
      return stations;
    }

    //Returns state of waiting stations as comma separated list of station indexes
    String getState(){
      String state = "";
      for(int i=0; i<len; i++){
        state += String(stations[i]);
        if (i != len-1){state += ", ";}
      }
      return state;
    }//end getState

    //Length getter
    uint8_t getLen(){
      return len;
    }


};//END SimpleList