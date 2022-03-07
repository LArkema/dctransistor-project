#line 2 "LastTrainTest.ino"

#include <AUnit.h>
#include "../../inc/miscFuncs.h" //miscFuncs includes TrainLine header


void setup(){
    Serial.begin(9600);
    randomSeed(0);
}

void loop(){
    aunit::TestRunner::run();
}

test(train_init){
    TrainLine line = TrainLine();
    assertEqual(line.getOppCID(0), 868);
}


test(set_train_state){
    TrainLine line = TrainLine();
    uint16_t train_positions[3] = {520, 596, 629};
    line.setInitialStations(train_positions, 3, 0);
    assertEqual(line.getState(), "0, 3, 7, 9");
}

test(arrive_last_only_train){
    TrainLine line = TrainLine();
    uint16_t train_positions[3] = {520, 596, 629};
    line.setInitialStations(train_positions, 3, 0);
    uint16_t arrival[1] = {652};
    checkEndOfLine(line, arrival, 1, 0);
    assertEqual(line.getCyclesAtEnd(0), 1);
    assertEqual(line.getState(), "0, 3, 7, 10");
    
    //2nd check
    checkEndOfLine(line, arrival, 1, 0);
    assertEqual(line.getCyclesAtEnd(0), 2);

    //3rd
    checkEndOfLine(line, arrival, 1, 0);
    assertEqual(line.getCyclesAtEnd(0), 3);

    //4th (removed)
    checkEndOfLine(line, arrival, 1, 0);
    assertEqual(line.getCyclesAtEnd(0), 0);
    assertEqual(line.getState(), "0, 3, 7");
}

test(approach_last_only_train){
    TrainLine line = TrainLine();
    uint16_t train_positions[3] = {520, 596, 629};
    line.setInitialStations(train_positions, 3, 0);
    uint16_t arrival[1] = {650};
    checkEndOfLine(line, arrival, 1, 0);
    assertEqual(line.getCyclesAtEnd(0), 1);
    assertEqual(line.getState(), "0, 3, 7, 10");

    //2nd check
    arrival[0] = 651;
    checkEndOfLine(line, arrival, 1, 0);
    assertEqual(line.getCyclesAtEnd(0), 2);

    //3rd
    arrival[0] = 652;
    checkEndOfLine(line, arrival, 1, 0);
    assertEqual(line.getCyclesAtEnd(0), 3);

    //4th (removed)
    checkEndOfLine(line, arrival, 1, 0);
    assertEqual(line.getCyclesAtEnd(0), 0);
    assertEqual(line.getState(), "0, 3, 7");
}

//When train arrives at first station on opposite platform
test(arrive_opp_only_train){
    TrainLine line = TrainLine();
    uint16_t train_positions[3] = {520, 596, 629};
    line.setInitialStations(train_positions, 3, 0);
    uint16_t arrival[1] = {868};
    checkEndOfLine(line, arrival, 1, 0);
    assertEqual(line.getCyclesAtEnd(0), 1);
    assertEqual(line.getState(), "0, 3, 7, 10");

    //2nd check
    checkEndOfLine(line, arrival, 1, 0);
    assertEqual(line.getCyclesAtEnd(0), 2);

    //3rd
    arrival[0] = 869;
    checkEndOfLine(line, arrival, 1, 0);
    assertEqual(line.getCyclesAtEnd(0), 3);

    //4th (removed)
    checkEndOfLine(line, arrival, 1, 0);
    assertEqual(line.getCyclesAtEnd(0), 0);
    assertEqual(line.getState(), "0, 3, 7");
}

/*
    BEGIN TESTS WITH MULTIPLE ACTIVE TRAINS
*/

//When train arrives suddenly registered at last circuit
test(arrive_last_many_trains){
    TrainLine line = TrainLine();
    uint16_t train_positions[3] = {520, 596, 629};
    line.setInitialStations(train_positions, 3, 0);
    uint16_t arrival[3] = {520, 569, 652};
    checkEndOfLine(line, arrival, 3, 0);
    assertEqual(line.getCyclesAtEnd(0), 1);
    assertEqual(line.getState(), "0, 3, 7, 10"); 

    //2nd check
    checkEndOfLine(line, arrival, 3, 0);
    assertEqual(line.getCyclesAtEnd(0), 2);

    //3rd
    checkEndOfLine(line, arrival, 3, 0);
    assertEqual(line.getCyclesAtEnd(0), 3);

    //4th (removed)
    checkEndOfLine(line, arrival, 3, 0);
    assertEqual(line.getCyclesAtEnd(0), 0);
    assertEqual(line.getState(), "0, 3, 7");   
}

//When train arrives by approaching last circuit
test(approach_last_many_trains){
    TrainLine line = TrainLine();
    uint16_t train_positions[3] = {520, 596, 629};
    line.setInitialStations(train_positions, 3, 0);
    uint16_t arrival[3] = {520, 569, 650};
    checkEndOfLine(line, arrival, 3, 0);
    assertEqual(line.getCyclesAtEnd(0), 1);
    assertEqual(line.getState(), "0, 3, 7, 10");

    //2nd check
    arrival[2] = 651;
    checkEndOfLine(line, arrival, 3, 0);
    assertEqual(line.getCyclesAtEnd(0), 2);

    //3rd
    arrival[2] = 652;
    checkEndOfLine(line, arrival, 3, 0);
    assertEqual(line.getCyclesAtEnd(0), 3);

    //4th (removed)
    checkEndOfLine(line, arrival, 3, 0);
    assertEqual(line.getCyclesAtEnd(0), 0);
    assertEqual(line.getState(), "0, 3, 7");    
}

test(arrive_opp_many_trains){
    TrainLine line = TrainLine();
    uint16_t train_positions[3] = {520, 596, 629};
    line.setInitialStations(train_positions, 3, 0);
    uint16_t arrival[1] = {868};
    checkEndOfLine(line, arrival, 3, 0);
    assertEqual(line.getCyclesAtEnd(0), 1);
    assertEqual(line.getState(), "0, 3, 7, 10");

    //2nd check
    checkEndOfLine(line, arrival, 3, 0);
    assertEqual(line.getCyclesAtEnd(0), 2);

    //3rd
    arrival[2] = 869;
    checkEndOfLine(line, arrival, 3, 0);
    assertEqual(line.getCyclesAtEnd(0), 3);

    //4th (removed)
    checkEndOfLine(line, arrival, 3, 0);
    assertEqual(line.getCyclesAtEnd(0), 0);
    assertEqual(line.getState(), "0, 3, 7");
}

/*
    BEGIN FALSE POSITIVE TESTS (SHOULD NOT INDICATE ARRIVAL)
*/

//Make sure that a train sitting at 1st circuit opposite direction doesn't skip real train
test(false_opp_train){
    TrainLine line = TrainLine();
    uint16_t train_positions[4] = {520, 868, 596, 625}; //train sitting at opp, one coming to 
    line.setInitialStations(train_positions, 4, 0);
    assertEqual(line.getState(), "0, 3, 7, 8");

    uint16_t arrival[1] = {628};
    line.arrived(3, 0);
    checkEndOfLine(line, arrival, 1, 0);
    assertEqual(line.getState(), "0, 3, 7, 9");

    arrival[0] = 629;
    checkEndOfLine(line, arrival, 1, 0);
    assertEqual(line.getState(), "0, 3, 7, 9");

    arrival[0] = 634;
    checkEndOfLine(line, arrival, 1, 0);
    assertEqual(line.getState(),"0, 3, 7, 9");

    arrival[0] = 647;
    checkEndOfLine(line, arrival, 1, 0);
    assertEqual(line.getState(), "0, 3, 7, 9");

    //Only indicate arrival when train actually arrives
    arrival[0] = 652;
    checkEndOfLine(line, arrival, 1, 0);
    assertEqual(line.getState(), "0, 3, 7, 10");
    assertEqual(line.getCyclesAtEnd(0), 1);
}