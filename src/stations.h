#include "Arduino.h"

#ifndef _stations_h_
#define _stations_h_

void setup_senderList();
void saveList();
void restore();
void reorder(uint8_t oldpos, uint8_t newpos);

#endif