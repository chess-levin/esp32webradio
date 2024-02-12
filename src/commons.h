#pragma once

#include "SimpleTimer.h"

// Get the number of elements in a C-style array 
#define ARRAY_LEN(array) (sizeof(array)/sizeof(array[0]))

#define MAX(a,b) ((a) > (b) ? a : b)
#define MIN(a,b) ((a) < (b) ? a : b)

// streamtitle can max use half of buffer size, because title is doubled for scrolling
#define MAXLEN_SCROLL_STREAMTITLE_BUFFER  128

#define RUN_MODE_STANDBY    0
#define RUN_MODE_STARTING   1
#define RUN_MODE_RESTART_SETUP 2
#define RUN_MODE_RADIO      3
#define RUN_MODE_OTA        4

// max 20 char long
#define AP_SSID "webradio"
#define AP_PWD  "12345678"

extern SimpleTimer timer;
extern char scrollingStreamTitleBuffer[];

void setWifiConfig(const String newSsid, const String newPkey);
void restartInRunMode(uint8_t newRunMode);
