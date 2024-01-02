#include "Arduino.h"
#include "globals.h"
#include "ota.h"
#include <ArduinoOTA.h>
#include "display.h"

uint32_t oldprc;

const char* otaHostname = "webradio";
const char* otaPassword = "radioupdate";

//prepare OTA
void setup_ota() {
  //set host name and passwort
  ArduinoOTA.setHostname(otaHostname);
  ArduinoOTA.setPassword(otaPassword);
  //register callback routines
  ArduinoOTA.onStart(ota_onStart);
  ArduinoOTA.onEnd(ota_onEnd);
  ArduinoOTA.onProgress(ota_onProgress);
  ArduinoOTA.onError(ota_onError);
  //start OTA handling
  ArduinoOTA.begin();
}

//on start decide between program or file upload
void ota_onStart() {
  displayClear();
  oldprc = 0;
  String type;
  if (ArduinoOTA.getCommand() == U_FLASH)
    type = "sketch";
  else // U_SPIFFS
    type = "filesystem";

  // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
  displayMessage(0,"Updating " + type);
  displayMessage(1,"Progress: 0%");
}
//on end no extra processing
void ota_onEnd() {
  displayMessage(2,"End");
}

//s
void ota_onProgress(unsigned int progress, unsigned int total) {
  uint32_t prc = progress / (total / 100);
  if (prc > oldprc) { 
    lcd.setCursor(10,1);
    lcd.print(prc);
    lcd.print("%");
    oldprc = prc;
  }
}

void ota_onError(ota_error_t error) {
  char err[20];
  sprintf(err,"Error[%u]: ", error);
  if (error == OTA_AUTH_ERROR) displayMessage(3,String(err)+"Auth Failed");
  else if (error == OTA_BEGIN_ERROR) displayMessage(3,String(err)+"Begin Failed");
  else if (error == OTA_CONNECT_ERROR) displayMessage(3,String(err)+"Connect Failed");
  else if (error == OTA_RECEIVE_ERROR) displayMessage(3,String(err)+"Receive Failed");
  else if (error == OTA_END_ERROR) displayMessage(3,String(err)+"End Failed");
}
