#pragma once

#include "Arduino.h"
#include <LiquidCrystal_I2C.h>

// set the LCD address to 0x27 for a 20 chars and 4 line display
#define DISPLAY_I2C_ADDR 0x27
#define DISPLAY_COLS 20
#define DISPLAY_ROWS 4

extern LiquidCrystal_I2C lcd;

void setup_display();
void showStation();
//void displayDateTime();
void displayMessage(uint8_t line, String msg);
//void displayMessage2(uint8_t line, String msg);
void displayScrollingMsgStart(uint8_t line, int timeId);
void displayScrollingMsgStop(uint8_t line, int timeId);
void displayScrollingMsgTick(uint8_t line);

void displayShowStatus(int8_t rssi, uint8_t vol, const char* datetime);
void displayWifiSignal(int8_t rssi);
void displayVolume(uint8_t vol);
void displayDateTime(const char* datetime);
void displayDateTime(const char* time, const char* date1, const char* date2);
void displayTimeSecBlink(boolean on);

void displayClear();
void displayClearLine(uint8_t line);
void displayBar(uint8_t line, uint8_t length);
