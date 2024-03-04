#include "display.h"


//library for LCD display
#include <LiquidCrystal_I2C.h>

#include "commons.h"

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

int scrollPtr = -1;
int scrollMsgLen = 0;

void setupDisplay() {
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
  }

} 

//show a one line message
void displayMessage(uint8_t line, String msg) {
  displayClearLine(line);
  lcd.setCursor(0,line);
  //massage is limited to 20 chars
  ex_print(msg.substring(0,20));
}


void displayScrollingMsgStart(uint8_t line, int timeId) {
  scrollPtr = 0;
  displayMessage(line, scrollingStreamTitleBuffer);

  strlcat(scrollingStreamTitleBuffer, "  ", MAXLEN_SCROLL_STREAMTITLE_BUFFER);

  scrollMsgLen = strlen(scrollingStreamTitleBuffer);

  // doubling string as buffer for scrolling
  uint8_t end = MIN(scrollMsgLen - 2, MAXLEN_SCROLL_STREAMTITLE_BUFFER/2);
  for (uint8_t i=0; i < end; i++) {
    scrollingStreamTitleBuffer[scrollMsgLen + i] = scrollingStreamTitleBuffer[i];
  }
  scrollingStreamTitleBuffer[scrollMsgLen + end] = '\0';
  timer.enable(timeId);
}

void displayScrollingMsgStop(uint8_t line, int timeId) {
  scrollPtr = -1;
  scrollMsgLen = 0;
  displayMessage(line, scrollingStreamTitleBuffer);
  timer.enable(timeId);
}

void displayScrollingMsgTick(uint8_t line) {
  char buf[DISPLAY_COLS+1]; // TODO: that's shit. better construct String from pointer and len. move to global?

  if (scrollPtr > scrollMsgLen) {
    scrollPtr = 0;
  }

  strncpy(buf, scrollingStreamTitleBuffer + scrollPtr, DISPLAY_COLS);
  buf[DISPLAY_COLS] = '\0';
  //log_d("buf '%s'  %d\n", buf, strlen(buf));
  
  scrollPtr++;
  lcd.setCursor(0, line);
  lcd.print(extraChar(buf));
  
  //lcd.print(doubleMsg.substring(scrollPtr, scrollPtr + DISPLAY_COLS));
  //scrollPtr = (scrollPtr < (msg.length()+4)) ? scrollPtr+1 : 0;
}

void displayBarSingle(uint8_t line, uint8_t percentage) {
  lcd.setCursor(percentage / (100 / DISPLAY_COLS), line);
  lcd.print("#");
}

void displayBar(uint8_t line, uint8_t length){
    if (length < DISPLAY_COLS) {
        char bar[] = "####################";
        for (uint8_t i = length+1; i < DISPLAY_COLS; i++) {
          bar[i] = ' ';
        }

        lcd.setCursor(0, line);
        lcd.print(bar); // lcd.printf("%.*s", length, bar);
    }
}

void displayAt(uint8_t line, uint8_t col, char c) {
  lcd.setCursor(col, line);
  lcd.print(c);
}

void displayMoveChar(uint8_t fromLine, uint8_t fromCol, uint8_t toLine, uint8_t toCol, char c) {
  displayAt(fromLine, fromCol, ' ');
  displayAt(toLine, toCol, c);
}


void displayClear() {
  lcd.clear();
}


void displayClearLine(uint8_t line) {
  lcd.setCursor(0, line);
  for (uint8_t i = 0; i<DISPLAY_COLS; i++) {
    lcd.print(" ");
  }
}


void displayShowStatus(int8_t rssi, uint8_t vol, const char* datetime) {
  displayWifiSignal(rssi);
  displayVolume(vol);
  displayDateTime(datetime);
}


void displayWifiSignal(int8_t rssi) {
  lcd.setCursor(0, 0);
  lcd.printf("%.2d", rssi);
}

void displayVolume(uint8_t vol) {
  lcd.setCursor(4, 0);
  lcd.write((byte) 0);
  lcd.printf("%02d", vol);
}

void displayDateTime(const char* datetime) {
  lcd.setCursor(8, 0);
  lcd.printf("%s", datetime);
}

void displayDateTime(const char* time, const char* date1, const char* date2) {
  lcd.setCursor((DISPLAY_COLS / 2) - (strlen(time) / 2), 1);
  lcd.print(time);
  lcd.setCursor((DISPLAY_COLS / 2) - (strlen(date1) / 2), 2);
  lcd.print(date1);
  lcd.setCursor((DISPLAY_COLS / 2) - (strlen(date2) / 2), 3);
  lcd.print(date2);
}

void displayTimeSecBlink(boolean on) {
  lcd.setCursor(17, 0);
  lcd.print((on)?":": " ");
}


/* https://stackoverflow.com/questions/293438/left-pad-printf-with-spaces
void print_with_indent(int indent, char * string)
{
    printf("%*s%s", indent, "", string);
}
*/