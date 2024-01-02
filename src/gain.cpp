#include "globals.h"
#include "gain.h"
#include "audio.h"
#include "display.h"

//library for rotary encoder
#include "AiEsp32RotaryEncoder.h"

//used pins for rotary encoder
#define ROTARY_ENCODER_A_PIN 13
#define ROTARY_ENCODER_B_PIN 14
#define ROTARY_ENCODER_BUTTON_PIN 12
#define ROTARY_ENCODER_VCC_PIN -1 /* 27 put -1 of Rotary encoder Vcc is connected directly to 3,3V; else you can use declared output pin for powering rotary encoder */
//depending on your encoder - try 1,2 or 4 to get expected behaviour
//#define ROTARY_ENCODER_STEPS 1
#define ROTARY_ENCODER_STEPS 2
//define ROTARY_ENCODER_STEPS 4
//instance for rotary encoder
AiEsp32RotaryEncoder gain = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

//handle events from rotary encoder
//called from main loop
void gain_loop()
{
  //dont do anything unless value changed
  if (gain.encoderChanged())
  {
    curGain = gain.readEncoder(); //get new value for gain
    pref.putUShort("gain",curGain);  //save the new value to preferences
    setGain();  //set the real loudness
    uint8_t g = curGain;
    if (g > 99) g = 99;  
    lcd.setCursor(18,0);
    lcd.print("  ");
    lcd.setCursor(18,0);
    lcd.print(g);
  }
}

//interrupt handling for rotary encoder
void IRAM_ATTR readGainISR()
{
  gain.readEncoder_ISR();
}

//prepare rotary encoder for gain control
void setup_gain() {
  //start rotary encoder instance
  gain.begin(); //satrencoder handling
  gain.setup(readGainISR); //register interrupt service routine
  gain.setBoundaries(0, 100, false); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)
  gain.setEncoderValue(curGain); //preset the value to current gain
}
