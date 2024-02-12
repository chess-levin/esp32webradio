#pragma once

#include "ArduinoJson.h"

#define MAX_COUNT_STATIONS  40
#define MAXLEN_STATION_NAME 20
#define MAXLEN_STATION_URL  128

typedef struct stationInfo_s {
  const char* name;
  const char* url;
} stationInfo_t;


int loadStations(const char *filename);
stationInfo_t getStationInfo(uint8_t i);
uint8_t getStationInfoSize();
stationInfo_t setStationInfoToDefault();
