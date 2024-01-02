#ifndef _ota_h_
#define _ota_h_

#include "Arduino.h"
#include <ArduinoOTA.h>

void setup_ota();
void ota_onStart();
void ota_onEnd();
void ota_onProgress(unsigned int progress, unsigned int total);
void ota_onError(ota_error_t error);

#endif