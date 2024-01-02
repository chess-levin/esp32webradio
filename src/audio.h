#include "Arduino.h"
#include "AudioGeneratorMP3.h"        //decoder

#ifndef _audio_h_
#define _audio_h_

#define LRCLK 25
#define BCLK 26
#define DOUT 27

extern AudioGenerator *decoder;         //MP3 decoder

void setup_audio();
void audio_loop();
void stopPlaying();
bool startUrl(String url);
void setGain();

#endif