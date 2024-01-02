#include "rotary.h"
#include "globals.h"
#include "display.h"
#include "audio.h"

//library for rotary encoder
#include "AiEsp32RotaryEncoder.h"

//used pins for rotary encoder
#define ROTARY_ENCODER_A_PIN 32
#define ROTARY_ENCODER_B_PIN 33
#define ROTARY_ENCODER_BUTTON_PIN 35
#define ROTARY_ENCODER_VCC_PIN -1 /* 27 put -1 of Rotary encoder Vcc is connected directly to 3,3V; else you can use declared output pin for powering rotary encoder */
//depending on your encoder - try 1,2 or 4 to get expected behaviour
//#define ROTARY_ENCODER_STEPS 1
#define ROTARY_ENCODER_STEPS 2
//define ROTARY_ENCODER_STEPS 4
//instance for rotary encoder
AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

//handle events from rotary encoder
//called from main loop
void rotary_loop()
{
  //dont do anything unless value changed
  if (rotaryEncoder.encoderChanged())
  {
    uint16_t v = rotaryEncoder.readEncoder();
    Serial.printf("Station: %i\n",v);
    uint8_t cnt = 0; //overflow counter, prevents endless loop if no station is enabled
    while ((!stationlist[v].enabled) && (cnt < 2)){
      v++;
      if (v >= STATIONS_MAX){
        v=0;
        cnt++;
      }
    }
    //set new currtent station and show its name
    if (v < STATIONS_MAX) {
      curStation = v;
      showStation();
      lastchange = millis();
    }
  }
  //if no change happened within 10s set active station as current station
  if ((lastchange > 0) && ((millis()-lastchange) > 10000)){
    curStation = actStation;
    lastchange = 0;
    showStation();
  }
  //react on rotary encoder switch
  if (rotaryEncoder.isEncoderButtonClicked())
  {
    //set current station as active station and start streaming
    actStation = curStation;
    Serial.printf("Active station %s\n",stationlist[actStation].name);
    pref.putUShort("station",curStation);
    if (!startUrl(String(stationlist[actStation].url))) {
      //if start fails we switch back to station 0
      actStation = 0;
      startUrl(String(stationlist[actStation].url));
    }
    //call show station to display the speaker symbol
    showStation();
  }
}

//interrupt handling for rotary encoder
void IRAM_ATTR readEncoderISR()
{
  rotaryEncoder.readEncoder_ISR();
}

void setup_rotary() {
  //start rotary encoder instance
  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  rotaryEncoder.setBoundaries(0, STATIONS_MAX, true); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)
  rotaryEncoder.disableAcceleration();
  rotaryEncoder.setEncoderValue(actStation); //preset the value to current active station

}
