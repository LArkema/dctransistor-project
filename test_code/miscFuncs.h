/*
* When 
*
*/

#include "TrainLine.h"

//Check if a train has reached the end of a line and, if so, whether or not to remove.
//TODO: APPLY OPPOSITE DIRECTION (Negative Circuit traversal) LOGIC
int checkEndOfLine(TrainLine &line, uint16_t* train_pos, uint8_t train_len, bool dir){

  int8_t coefficient = (dir * -2) + 1; //turns 0 to 1 and 1 to -1

  //Only enter if waiting for train at last station or train sitting at last station
  if(line.at(line.getLen(dir)-1, dir) == line.getTotalNumStations()-1 || line.getCyclesAtEnd(dir) != 0){

    Serial.println("Checking last station");

    //If train has reached end of line, cycles_at_end is non-zero.
    if(line.getCyclesAtEnd(dir) > 0){
      //Serial.println("Incrementing cycle");
      if(line.incrementCyclesAtEnd(dir) == 0){ //increment rolls over to 0 when max reached
        line.remove(dir);
      }
      return 0;
    }

    uint16_t* possible_trains = new uint16_t[train_len];
    uint8_t possible_trains_len = 0;//index for possible_trains array

    //If train not at end of line, check if is (or isn't)
    for(int i=0; i<train_len; i++){

      //Only look at trains arriving at or past 2nd to last station
      //Multiply both train position and circuitID 2 circuits before 2nd to last station by coefficient
      if(train_pos[i]*coefficient >= (line.getStationCircuit(line.getTotalNumStations()-2, dir)*coefficient)-2) {
        

        //If a train is between 2nd to last and last station, has not arrived yet. Return.
        if(train_pos[i]*coefficient < (line.getLastCID(dir)*coefficient)-2){
          //Serial.printf("Train %d en route to last. Not arrived yet.\n", train_pos[i]);

          delete[] possible_trains;
          return -1;
        }

        possible_trains[possible_trains_len] = train_pos[i];
        possible_trains_len++;

      }//end if for at or past 2nd to last station
      
    }//end loop through active trains. Check all trains before checking 1st circuit in opp direction.

    //Loop through all possible trains, check if it has arrived at last station circuit yet.
    for(uint8_t i=0; i < possible_trains_len; i++){
      //Serial.printf("Possible train at last station for direction %d: %d\n", dir, possible_trains[i]);

      //Arrived if train between 3 circuits before last station and last station (1st two code lines),
      // or at 1st station of other direction (last code line).
      if( ( (possible_trains[i]*coefficient >= (line.getLastCID(dir)*coefficient)-3) 
       && (possible_trains[i]*coefficient <= line.getLastCID(dir)*coefficient) ) ||
       possible_trains[i] == line.getOppCID(dir) ){

          Serial.println("Last station reached");
          line.arrived(line.getLen(dir)-1, dir);
          line.incrementCyclesAtEnd(dir);

        delete[] possible_trains;
        return 0;
      }
    }

    delete[] possible_trains;
  }
  return -1;
}