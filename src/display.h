#ifndef _display_h_
#define _display_h_

#include "Arduino.h"
#include <LiquidCrystal_I2C.h>

// set the LCD address to 0x27 for a 20 chars and 4 line display
#define DISPLAY_I2C_ADDR 0x27
#define DISPLAY_COLS 20
#define DISPLAY_ROWS 4

extern LiquidCrystal_I2C lcd;

void setup_display();
void showStation();
void displayDateTime();
void displayMessage(uint8_t line, String msg);
void displayMessage2(uint8_t line, String msg);
void displayClear();
void clearLine(uint8_t line);

#endif