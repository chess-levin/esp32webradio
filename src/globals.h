#include "Arduino.h"

//esp32 library to save preferences in flash
#include <Preferences.h>

#ifndef _globals_h_
#define _globals_h_

#define ELEMENTS(x) (sizeof(x) / sizeof(x[0]))
#define TITLE_LEN 64
#define STATIONS_MAX 40 // max number of stations in the list

//instance of preferences
extern Preferences pref;
extern Preferences sender;

//structure for station list
typedef struct {
  char url[150];  //stream url
  char name[32];  //stations name
  uint8_t enabled;//flag to activate the station
} Station;

extern Station stationlist[];

extern String ssid;
extern String pkey;
extern String ntp;
extern boolean connected;
extern uint8_t curStation;     //index for current selected station in stationlist
extern uint8_t curGain;        //current loudness
extern uint8_t actStation;     //index for current station in station list used for streaming 
extern uint32_t lastchange;    //time of last selection change
extern char title[];             //character array to hold meta data message
extern bool newTitle;
extern uint32_t tick;          //last tick-counter value to trigger timed event
extern uint32_t discon;        //tick-counter value to calculate disconnected time

#endif