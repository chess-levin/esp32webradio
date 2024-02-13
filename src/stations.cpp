#include <ArduinoJson.h>
#include <SPIFFS.h>

#include "stations.h"
#include "commons.h"

stationInfo_t * stationInfo;
int countElements = -1;

stationInfo_t stationInfoDefault = {"Radio Bremen 2","http://icecast.radiobremen.de/rb/bremenzwei/live/mp3/128/stream.mp3"};

stationInfo_t setStationInfoToDefault() {
  stationInfo = &stationInfoDefault;
  countElements = 1;
  return stationInfo[0];
}

uint8_t getStationInfoSize() {
  return countElements;
}

stationInfo_t getStationInfo(uint8_t i) {
  if (i >= getStationInfoSize()) 
    log_e("index %d out of bounds in getStationInfo()", i);

  return stationInfo[i];
}

int loadStations(const char *filename) {
  log_d("Reading stations from: %s", filename); 
  
  File file = SPIFFS.open(filename);

  if(!file) {
    log_e("Failed to open: %s", filename);
    return countElements;
  } 

  log_d("Successfully opened: %s (%d bytes)", filename, file.size());
  
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, file);

  if (error) {
    log_e("deserializeJson() failed: %s", error.c_str());
    return countElements;
  }

  JsonArray arr = doc.as<JsonArray>();
  countElements = arr.size();
  log_d("Successfully deserialized %d elements", countElements);
  if (countElements > MAX_COUNT_STATIONS) {
    log_w("Number of elements exceeds maximum %d", MAX_COUNT_STATIONS);
  }

  stationInfo = new stationInfo_t[countElements];

  int i = 0;
  for (JsonObject item : doc.as<JsonArray>()) {
    const char *data = item["name"];
    stationInfo[i].name = strndup(data, 20);

    if (strlen(data) > MAXLEN_STATION_NAME) {
      log_w("shortened station.name '%s' to %d chars", data, MAXLEN_STATION_NAME);
    }

    if (stationInfo[i].name == NULL) {
      countElements = i;
      log_e("could not copy station.name '%s'", data);
      return countElements;
    }

    data = item["url"];
    stationInfo[i].url = strndup(data, 128);

    if (strlen(data) > MAXLEN_STATION_URL) {
      log_w("shortened station.url '%s' to %d chars", data, MAXLEN_STATION_URL);
    }

    if (stationInfo[i].url == NULL) {
      countElements = i;
      log_e("could not copy station.url '%s'", data);
      return countElements;
    }

    log_v("%d\t %s\t %s", i, stationInfo[i].name, stationInfo[i].url);
    i++;
  }

  file.close();
  return countElements;
}