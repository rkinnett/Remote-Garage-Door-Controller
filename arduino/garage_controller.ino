#include "DHT.h"

#define STR_CMD_CLOSE "close"
#define STR_CMD_OPEN "open"
#define STR_CMD_TOGGLE "toggle"
#define STR_CMD_CHECK "check"

#define STR_STATE_CLOSED "closed"
#define STR_STATE_OPEN "open"

#define SIZE_BUFFER_TX 64
#define SIZE_BUFFER_RX 64

#define PIN_RELAY 5
#define PIN_RANGER_TRIGGER 10
#define PIN_RANGER_ECHO 9
#define PIN_DHT 8


#define INT_DHT_AVG_NSAMPLES 10

#define INT_CHECK_PERIOD_SEC 30 //sec

#define INT_RANGE_MIN 6 //inches
#define INT_RANGE_MAX_OPEN 20 //inches
#define INT_RANGE_MAX_CLOSED_CAR 60 //inches
#define INT_RANGE_MAX_CLOSED_NO_CAR 168 //inches
#define INT_RANGE_MAX 145 //inches

#define INT_RANGE_CHECK_COUNTER_MAX 20  //measurements
#define INT_RANGE_CHECK_MAX_DIFF 6 //inches

#define ONE_DAY_MILLIS (24 * 60 * 60 * 1000)

const char strEventTypeStateChanged[] = "state_changed";
const char strEventCarArrived[] = "car arrived";
const char strEventCarDeparted[] = "car departed";
const char strEventDoorOpened[] = "garage door opened";
const char strEventDoorClosed[] = "garage door closed";

char strDoorStateCurrent[16] = "unknown";
char strStateCommanded[16] = "";

char txBuffer[SIZE_BUFFER_TX] = "";
char strReceivedCommand[SIZE_BUFFER_RX] = "";

int intRangeInches = 0;
int intEchoPulseWidthMicros;

bool boolDoorIsOpen;
bool boolDoorWasOpen;

bool boolCarIsPresent = false;
bool boolCarWasPresent;
bool boolErrorOccurred = false;

unsigned long timeLastStateChange = millis();
unsigned long timeOpened;
int minutesSinceOpened;
unsigned long timeLastSync = millis();

DHT dht(PIN_DHT, DHT11);
int rawHumidity[16];
int rawTemperature[16];
int avgHumidity;
int avgTemperature;

struct Counter {
  unsigned uint4:4; // 4-bit counter
} ctr;



void setup() {
  // Configure hardware interfaces:
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_RANGER_TRIGGER, OUTPUT);
  pinMode(PIN_RANGER_ECHO, INPUT);
  digitalWrite(BUILTIN_LED, LOW);
  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_RANGER_TRIGGER, LOW);
  dht.begin();
  
  // Configure cloud interfaces:
  Particle.disconnect();
  Particle.function("command", receiveCommand);
  Particle.function("reseterror", resetErrorFlag);
  Particle.variable("lastcmd", strReceivedCommand);
  Particle.variable("state_str", strDoorStateCurrent);
  Particle.variable("doorisopen", boolDoorIsOpen);
  Particle.variable("minutesopen", minutesSinceOpened);
  Particle.variable("carispresent", boolCarIsPresent);
  Particle.variable("range", intRangeInches);
  //Particle.variable("error", boolErrorOccurred);
  Particle.variable("humidity", avgHumidity);
  Particle.variable("temperature", avgTemperature);
  Particle.connect();

  yield();
  delay(2000);
  
  // Initialize temperature and humidity buffers:
  rawHumidity[0] = dht.readHumidity();
  rawTemperature[0] = dht.readTemperature(true);
  for(int i=0; i<16; i++){
    rawHumidity[i] = rawHumidity[0];
    rawTemperature[i] = rawTemperature[0];
  }
}


// MAIN LOOP
void loop() {
  longWaitSeconds(INT_CHECK_PERIOD_SEC);
  checkIfDoorIsOpen();
}



int checkConnection(){
  // If Oak is connected to wifi, but not connected to Particle, attempt to reconnect to Particle.
  if( Oak.connected() ){
    if( !Particle.connected() ){
      Particle.connect();
      longWaitSeconds(INT_CHECK_PERIOD_SEC);
    }
  } else {
    // If wifi is not connected, wait, recheck, and if still not connected, reboot.
    longWaitSeconds(INT_CHECK_PERIOD_SEC);
    if( !Oak.connected() ){
      Oak.rebootToUser();
    }
  }
}


// longWaitSeconds( seconds )
// Pauses for specified number of seconds, broken into multiple delays with yields
// between delays to allow windows for http requests between main loop operations.
// This function tracks elapsed time to compensate for http interruptions.
int longWaitSeconds(int seconds_to_wait){
  unsigned long time_sec_now = millis()/1000;
  unsigned long time_sec_start = time_sec_now;
  // Note: in theory, typecasting to signed long in the 
  // following comparison makes this robust to millis rollover.
  // See: http://playground.arduino.cc/Code/TimingRollover
  while( (long)(time_sec_now - time_sec_start) < seconds_to_wait ){
    yield(); // window for http requests
    delay(2000);
    yield();
    readDhtSensor();
    heartBeat();
    time_sec_now = millis()/1000;
  }
}


int readDhtSensor(){
  ctr.uint4++;
  rawHumidity[ctr.uint4] = dht.readHumidity();
  rawTemperature[ctr.uint4] = dht.readTemperature(true);
  int sumH = 0;
  int sumT = 0;
  for(int i=0; i<16; i++){
    sumH += rawHumidity[i];
    sumT += rawTemperature[i];
  }
  avgHumidity = sumH/16;
  avgTemperature = sumT/16;
}



int receiveCommand(String strCommand ){
  // Acknowledge command receipt:
  strncpy(strReceivedCommand, strCommand.c_str(), SIZE_BUFFER_RX);
  Particle.publish("Received command", strReceivedCommand, SIZE_BUFFER_RX, PRIVATE);

  // Check initial state, throw error if unable to determine
  if( checkIfDoorIsOpen() == -1 ){
    Particle.publish("receiveCommand() error","unable to determine initial state");
    boolErrorOccurred = true;
    return -1;
  }

  // If received "OPEN" command:
  if (strcmp(strReceivedCommand, STR_CMD_OPEN)==0){
    if( boolDoorIsOpen ){
      Particle.publish("receiveCommand() warning","received OPEN command while door is already open");
      return -1;      
    }
    strcpy(strStateCommanded, STR_STATE_OPEN);
  } 
  
  // If received "CLOSE" command:
  else if(strcmp(strReceivedCommand, STR_CMD_CLOSE)==0){
    if( !boolDoorIsOpen  ){
      Particle.publish("receiveCommand() warning","received CLOSE command while door is already closed");
      return -1;      
    }    
    strcpy(strStateCommanded, STR_STATE_CLOSED);
  }
  
  // If received "TOGGLE" command:
  else if(strcmp(strReceivedCommand, STR_CMD_TOGGLE)==0){
    if( boolDoorIsOpen ){
      strcpy(strStateCommanded, STR_STATE_CLOSED);
    } else {
      strcpy(strStateCommanded, STR_STATE_OPEN);
    }
  }
  
  // If received "CHECK" command:
  else if (strcmp(strReceivedCommand, STR_CMD_CHECK)==0){
    return ( boolDoorIsOpen? 1 : ( boolCarIsPresent?3:0 ));
  }
  
  // Error if the command string is unrecognized:
  else {
    Particle.publish("Error, unrecognized command", strReceivedCommand, SIZE_BUFFER_RX, PRIVATE);
    boolErrorOccurred = true;
    return -1;
  }

  // Execute the command!
  Particle.publish("Executing command", strReceivedCommand, SIZE_BUFFER_RX, PRIVATE);
  return toggleGarageDoor();
}


int resetErrorFlag(String str){
  boolErrorOccurred = false;
}


// toggleGarageDoor() closes the garage button relay for half a second
int toggleGarageDoor(){
  //bool doorInitialState = checkIfDoorIsOpen();
  //bool doorCurrentState = doorInitialState;
  
  // Pulse the opener toggle relay:
  Particle.publish("toggleGarageDoor(): toggling", strReceivedCommand);
  digitalWrite(PIN_RELAY, HIGH);
  delay(500);
  digitalWrite(PIN_RELAY, LOW);

/*
  // Wait 10 sec, then check state once per sec
  delay(10000);
  for(int i=0; i<10; i++){
    doorCurrentState = checkIfDoorIsOpen();
    if( doorCurrentState != doorInitialState ){
      break;
    }
    delay(1000);
  }

  // Throw error if state never changed
  if( doorCurrentState != doorInitialState ){
    Particle.publish("toggleGarageDoor(): toggled to", strDoorStateCurrent);
    return doorCurrentState;
  } else {
    Particle.publish("toggleGarageDoor(): failed", "state did not change");
    boolErrorOccurred = true;
    return -1;
  }
  */
  return 1;
}



int getRange(){
  digitalWrite(PIN_RANGER_TRIGGER, LOW); // ensure trigger pin is low prior to pulse
  delay(100); 
  digitalWrite(PIN_RANGER_TRIGGER, HIGH); // start trigger pulse
  delay(10); 
  digitalWrite(PIN_RANGER_TRIGGER, LOW); // end trigger pulse

  // Measure width in microseconds of the return echo
  intEchoPulseWidthMicros = pulseIn(PIN_RANGER_ECHO, HIGH);  
 
  // Calculate distance to object. Divide by 2 because echo time is roundtrip
  intRangeInches = (intEchoPulseWidthMicros/2) / 74;

  // Check if result is in valid range:
  if( intRangeInches<INT_RANGE_MIN || intRangeInches>INT_RANGE_MAX ){
    sprintf(txBuffer, "%i inches", intRangeInches);
    Particle.publish("checkState() error, out of range", txBuffer);
    boolErrorOccurred = true;
    intRangeInches = -1;
  }

  //sprintf(txBuffer,"%i inches",intRangeInches);
  //Particle.publish("getRange() measured range",txBuffer);
  return intRangeInches;
}


int checkIfDoorIsOpen(){
  bool foundRange = false;
  int intRangeMeasurement[10];
  bool boolRangeMeasurementIsValid[10];
  int rangeCheckCounter = 0;
  int hoursSinceOpened = minutesSinceOpened/60;
  int prevHoursSinceOpened = hoursSinceOpened;

  boolDoorWasOpen = boolDoorIsOpen;
  boolCarWasPresent = boolCarIsPresent;

  // Measure distance:  
  // Try up to 10 times to get valid range
  // Need 2 consecutive valid measurements near eachother
  boolRangeMeasurementIsValid[0] = false;
  while( !foundRange && rangeCheckCounter < INT_RANGE_CHECK_COUNTER_MAX ){
    rangeCheckCounter++;
    intRangeMeasurement[rangeCheckCounter] = getRange();
    boolRangeMeasurementIsValid[rangeCheckCounter] = ( intRangeMeasurement[rangeCheckCounter]>0 && intRangeMeasurement[rangeCheckCounter]<INT_RANGE_MAX );
    if( boolRangeMeasurementIsValid[rangeCheckCounter] && boolRangeMeasurementIsValid[rangeCheckCounter-1] ){
      if( (intRangeMeasurement[rangeCheckCounter]-intRangeMeasurement[rangeCheckCounter-1])<INT_RANGE_CHECK_MAX_DIFF && (intRangeMeasurement[rangeCheckCounter]-intRangeMeasurement[rangeCheckCounter-1])>-INT_RANGE_CHECK_MAX_DIFF ){
        foundRange = true;
        intRangeInches = (intRangeMeasurement[rangeCheckCounter]+intRangeMeasurement[rangeCheckCounter-1])/2;
        break;
      }
    }
    yield();
    delay(100);
  }

  strcpy(txBuffer,"");
  for(int i=1; i<=rangeCheckCounter; i++){
    snprintf(txBuffer, SIZE_BUFFER_TX-1, "%s%i,", txBuffer, intRangeMeasurement[i]);
  }
  Particle.publish("checkIfDoorIsOpen() ranges",txBuffer);

  // If failed to acquire valid range measurement, then return last known state
  if( !foundRange ){
    Particle.publish("checkIfDoorIsOpen() error","unable to measure consistent range");
    return -1;
  }

  // Report result:
  sprintf(txBuffer, "%i inches (found in %i range checks)", intRangeInches, rangeCheckCounter);
  Particle.publish("found_range", txBuffer);

  // Update door state:
  boolDoorIsOpen = (intRangeInches>0 && intRangeInches<=INT_RANGE_MAX_OPEN);

  // Update car state, ONLY if door is closed
  if(!boolDoorIsOpen){
    boolCarIsPresent = (intRangeInches<=INT_RANGE_MAX_CLOSED_CAR);
  } 

  // If the garage door state changed, then update door state string and publish event:
  if( boolDoorIsOpen != boolDoorWasOpen || boolCarIsPresent != boolCarWasPresent){
    if(boolDoorIsOpen){
      strcpy(strDoorStateCurrent, STR_STATE_OPEN);
      timeOpened = millis();
    } else {
      strcpy(strDoorStateCurrent, STR_STATE_CLOSED);
      minutesSinceOpened = 0;
    }
    Particle.publish(strEventTypeStateChanged, strDoorStateCurrent);
  }

  // If door is open, then update the open counter:
  if( boolDoorIsOpen ){
    minutesSinceOpened = (int) (millis() - timeOpened)/1000/60;
    hoursSinceOpened = minutesSinceOpened/60;
    if( hoursSinceOpened>prevHoursSinceOpened ){
      sprintf(txBuffer, "%i", hoursSinceOpened);
      Particle.publish("hours_opened", txBuffer);
    }
  }


  // Publish event if car presence changed:
  if( boolCarIsPresent != boolCarWasPresent ){
    if( boolCarIsPresent ){
      Particle.publish(strEventTypeStateChanged, strEventCarArrived);
    } else {
      Particle.publish(strEventTypeStateChanged, strEventCarDeparted);
    }
  }
  
  return (boolDoorIsOpen)?1: ((boolCarIsPresent)?3:0);
}



void heartBeat(){
  digitalWrite(BUILTIN_LED, HIGH);
  delay(50);
  //digitalWrite(BUILTIN_LED, LOW);
  //delay(50);
  //digitalWrite(BUILTIN_LED, HIGH);
  //delay(50);
  digitalWrite(BUILTIN_LED, LOW);
}


