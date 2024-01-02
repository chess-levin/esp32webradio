#include "globals.h"
#include "display.h"
#include "audio.h"

//library for LCD display
#include <LiquidCrystal_I2C.h>

//Special character to show a speaker icon for current station
uint8_t font [8][9] {
     {0x03,0x05,0x19,0x11,0x19,0x05,0x03,0x00}, // 0 Speaker
     {0x11,0x04,0x0a,0x11,0x1f,0x11,0x11,0x00}, // 1 Ä
     {0x11,0x0e,0x11,0x11,0x11,0x11,0x0e,0x00}, // 2 Ö
     {0x11,0x00,0x11,0x11,0x11,0x11,0x0e,0x00}, // 3 Ü
     {0x0a,0x00,0x1e,0x01,0x0f,0x13,0x0d,0x00}, // 4 ä
     {0x0a,0x00,0x0e,0x11,0x11,0x11,0x0e,0x00}, // 5 ö
     {0x0a,0x00,0x12,0x12,0x12,0x12,0x0f,0x00}, // 6 ü
     {0x0c,0x12,0x12,0x14,0x12,0x12,0x14,0x00}  // 7 ß
};


//instance for LCD display
LiquidCrystal_I2C lcd(DISPLAY_I2C_ADDR, DISPLAY_COLS, DISPLAY_ROWS); 

void setup_display() {
  //init the LCD display
  lcd.init();
  lcd.backlight();
  for (uint8_t i = 0; i<8; i++) lcd.createChar(i, font[i]);
}

//translate german umlaut from UTF8 to extra character
String extraChar(String text){
  String res = "";
  uint8_t i = 0;
  char c;
  while (i<text.length()) {
    c=text[i];
    if (c==195) { //UTF8 Deutsche Umlaute
      i++;
      switch (text[i]) {
        case 164: c=4; break; //ä
        case 182: c=5; break; //ö
        case 188: c=6; break; //ü
        case 159: c=7; break; //ß
        case 132: c=1; break; //Ä
        case 150: c=2; break; //Ö
        case 156: c=3; break; //Ü
        default: c=0;
      }
    } else if (c > 128) { //other special Chracters 
      c=0;
    }
    if (c>0) res.concat(c);
    i++;
  }
  return res;
}

//print function which not interrupts audio stream
void ex_print(String output) {
  const char * o = output.c_str();
  for (uint16_t i = 0; i<strlen(o); i++) { 
    lcd.print(o[i]);
    if (decoder && (decoder->isRunning())) decoder->loop();
  }

}

//show name of current station on LCD display
//show the speaker symbol in front if current station = active station
void showStation() {
  clearLine(1);
  //show speaker if current station is active
  if (curStation == actStation) {
    lcd.setCursor(0,1);
    lcd.print(char(0));
  }
  //show station name on position 2
  lcd.setCursor(2,1);
  String name = String(stationlist[curStation].name);
  ex_print(name.substring(0,18)); //limit length to 18 chars
}

//show a two line message with line wrap
void displayMessage2(uint8_t line, String msg) {
  //first translate german umlauts
  msg = extraChar(msg);
  //delete both lines
  clearLine(line);
  clearLine(line+1);
  lcd.setCursor(0,line);
  if (msg.length() < 21)
    //message fits in one line
    ex_print(msg);
  else {
    //message has more then 20 chars, so split it on space
    uint8_t p = msg.lastIndexOf(" ",20); //if name does not fit, split line on space
    ex_print(msg.substring(0,p));
    lcd.setCursor(0,line+1);
    //limit the second line to 20 chars
    ex_print(msg.substring(p+1,p+20));
  }
}

//show date, time and loudness in the first line
void displayDateTime() {
  char sttime[21];
  clearLine(0);
  //limit gain on 99% since we have only two digits
  lcd.setCursor(0,0);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
  //get date and time as a string
    strftime(sttime, sizeof(sttime), "%d.%b %Y %H:%M 0", &timeinfo);
    ex_print(String(sttime));
  } else {
    //liefert die RTC keine Werte so wird ??.??? angezeigt
    ex_print("??.??? ????  ??:?? ");
  }
  //display loudness
  uint8_t g = curGain;
  if (g > 99) g = 99;  
  lcd.setCursor(18,0);
  lcd.print(g);
}

//show a one line message
void displayMessage(uint8_t line, String msg) {
  clearLine(line);
  lcd.setCursor(0,line);
  //massage is limited to 20 chars
  ex_print(msg.substring(0,20));
}

//clear the whole display
void displayClear() {
  lcd.clear();
}

//clear one line
void clearLine(uint8_t line) {
  lcd.setCursor(0,line);
  for (uint8_t i = 0; i<20; i++) {
    if (decoder && (decoder->isRunning())) decoder->loop();
    lcd.print(" ");
  }
}
