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
    Serial.printf("ERROR: index %d out of bounds in getStationInfo()", i);

  return stationInfo[i];
}

int loadStations(const char *filename) {
  Serial.printf("Reading stations from: %s\n", filename); 
  
  File file = SPIFFS.open(filename);

  if(!file) {
    Serial.printf("ERROR: Failed to open: %s\n", filename);
    return countElements;
  } 

  Serial.printf("Successfully opened: %s (%d bytes)\n", filename, file.size());
  
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, file);

  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return countElements;
  }

  JsonArray arr = doc.as<JsonArray>();
  countElements = arr.size();
  Serial.printf("Successfully deserialized %d elements\n", countElements);
  if (countElements > MAX_COUNT_STATIONS) {
    Serial.printf("Number of elements exceeds maximum %d\n", MAX_COUNT_STATIONS);
  }

  stationInfo = new stationInfo_t[countElements];

  int i = 0;
  for (JsonObject item : doc.as<JsonArray>()) {
    const char *data = item["name"];
    stationInfo[i].name = strndup(data, 20);

    if (strlen(data) > MAXLEN_STATION_NAME) {
      Serial.printf("WARN: shortened station.name '%s' to %d chars\n", data, MAXLEN_STATION_NAME);
    }

    if (stationInfo[i].name == NULL) {
      countElements = i;
      Serial.printf("ERROR: could not copy station.name '%s'\n", data);
      return countElements;
    }

    data = item["url"];
    stationInfo[i].url = strndup(data, 128);

    if (strlen(data) > MAXLEN_STATION_URL) {
      Serial.printf("WARN: shortened station.url '%s' to %d chars\n", data, MAXLEN_STATION_URL);
    }

    if (stationInfo[i].url == NULL) {
      countElements = i;
      Serial.printf("ERROR: could not copy station.url '%s'\n", data);
      return countElements;
    }

    Serial.printf("%d\t %s\t %s\n", i, stationInfo[i].name, stationInfo[i].url);
    i++;
  }

  file.close();
  return countElements;
}