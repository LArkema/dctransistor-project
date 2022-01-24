#include <Arduino.h>
/*
    Defines SimpleList class. Data structure optimized to store which stations are waiting for a train to arrive
    and updates accordingly. Represents stations using index (position of station on given line) for simplicity.

    Designed to compile on Desktop (using EpoxyDuino) and Arduino for easy and integrated unit testing.
*/

const uint8_t MAX_NUM_STATIONS = 10; //Max number of stations per line

class SimpleList {
  private:
    uint8_t len;

    //Insert new station to waiting station list to represent new train. New train only added when train arrives at 1st station.
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

    //Remove a train after reaching end of line. Removes end of list (i.e. dequeue)
    //Returns size of waiting stations list, 
    //Returns -1 if last train not at end of line
    int remove(){
      //can only remove if last train is at last station
      if(stations[len-1]+1 == MAX_NUM_STATIONS){
        stations[len-1] = 0;
        len--;
        return len;
      }
      return -1;
    }//end remove

  //END PRIVATE
  
  public:
    uint8_t stations[MAX_NUM_STATIONS]; //list of integers representing station indexes /*Flawfinder: ignore */
    
    SimpleList(){
      len = 1;
      memset(stations, 0, MAX_NUM_STATIONS);
    }

    //Indicate train has arrived at station by incrementing waiting station to next station
    //Only updates if there is is not another train already at station.

    //Returns new index of waiting station or size of station list if station removed.
    //Returns -2 for collision with neighbor, -1 for list full (from add), and -1 for improper access
    int arrived(uint8_t index){
      if(index < len){
        //If arriving at last station, remove from list
        if(stations[index]+1 == MAX_NUM_STATIONS){
          return remove();
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