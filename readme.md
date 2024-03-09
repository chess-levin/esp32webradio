# ESP32 Webradio
## Introduction

![picture of my webradio](/docs/radio.jpg "picture of my webradio")


This project is inspired by several other internet radio projects, but all projects lacked direct output via Bluetooth speakers. The first idea came from a project published by Gerald Lechner in an [article](https://www.az-delivery.de/blogs/azdelivery-blog-fur-arduino-und-raspberry-pi/internetradio-mit-esp32-und-max-98357a?comment=134821511435&page=1) at the AZ Delivery blog. I've ported the original code from an Arduino IDE compatible structure to a PlatformIO version. 

But this version suffers from a permanently interrupted tcp audio stream. So I looked for a solution to this problem and found these projects [ESP32-MiniWebRadio](https://github.com/schreibfaul1/ESP32-MiniWebRadio) and 
[ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S/tree/master/examples/Better_WiFi_throughput). Especially the last one contained the solution on how to optimize the tcp settings to get better streaming performance with the arduino framework - thank you [Wolle](https://github.com/schreibfaul1) for your support.

And then there was the Bluetooth challenge...

**First Try:** As I wanted to connect bluetooth speakers to my webradio I searched for similar projects that had done this before. I found ... nothing, but [pschatzmann](https://github.com/pschatzmann) and his great projects/libs [ESP32-A2DP](https://github.com/pschatzmann/ESP32-A2DP) and [arduino-audio-tools](https://github.com/pschatzmann/arduino-audio-tools). Finally I found his post *"The ESP32 only supports either Bluetooth or WIFI, but not both at the same time. So if you use A2DP, you will not be able to use any functionality which depends on WIFI (e.g. FreeRTOS queues)"* in his [project wiki](https://github.com/pschatzmann/ESP32-A2DP/wiki/WIFI-and-A2DP-Coexistence) - dead end.

**Second try:** I stumbled across the [KCX_BT_EMITTER](https://www.youtube.com/watch?v=ZQ5MWcis8rA) in [Ralph S Bacon's VLOG](https://www.youtube.com/watch?v=ZQ5MWcis8rA). I got it working, but this device couldn't pair with my Marshall Emberton II BT Speaker. It's a nice little device and it's fun to play with serial interface and these old AT+commands.

**Third try:** I bought this little gadget [ORIA Bluetooth Aux Adapter, 2 in 1 Bluetooth 5.0](https://www.amazon.de/dp/B0BNKJHGTL) to connect my BT speakers. The next picture shows how I integrated the ORIA in my case. It came with a rechargeable battery and I had to get rid of it (for details see [post](https://www.mikrocontroller.net/topic/565060)). The two holes at the side of the case are for the status LEDs and between the holes is the grey BT-connect press-button.

![ORIA BT Adapter Integration](/docs/kcx_bt_emitter_integrated.jpg "BT adapter integration")

**Fourth try:** I didn't like all these solutions I've tried so far, because the audio data is converted mulitple times (D->A->D) - that is superfluous. So I continued my search. I've found & ordered the [TSA5001 module](https://www.tinysineaudio.com/products/tsa5001-bluetooth-5-3-audio-transmitter-board-i2s-digital-input) that utilizes the I2S data as input. It works as a drop-in replacement of my PCM5102a, but I forgot, that different radio stations using different sampling rates. The TSA5001 specs

* Sampling Rate: 48KHz
* Bit per Sample: 16 bit, 24bit, 32bit.
* aptX, aptX Low Latency, aptX HD, SBC and AAC
* Bluetooth protocol: A2DP

are saying, it supports only one fixed sampling rate. This works perfectly for radio stations with a sample rate of @48KHz. Streams with other sample rates (e.g. 44,1Khz or 24KHz) are played back with regular interruptions or at a wrong pitch.

**@ALL: Is there a (easy) way to resample streams to 48KHz?**

To remember my research results I wrote down many of the information, e.g.

* [various ESP32 models](#various-esp32-boards)
* [hardware communication protocols](#protocols)
* [information on variuos displays](#various-displays)
* [analouge and I2C amplifiers](#variuos-amplifiers)
* [useful libraries ](#libs)


## Features/Requirements

* Reliable operation
* Good sound quality
* Connectivity for Bluetooth speakers
* Analog output-jack for active speakers
* Function as clock when radio is off
* Simple user interface in the style of old car radios
* Visualization of information about current song and station
* Store stations as JSON-file in mcu filesystem
* Webserver-based wifi setup
* OTA update for firmware and radio station JSON

## Components and Connections 

Components used for this project

* ESP32 DevKit V4 with WROOM32 U (and external antenna)
* LCD blue 4x20 Zeichen, HD44780, I2C
* PCM5102 DAC, I2S
* Rotary Encoder KY-040
* [ORIA Bluetooth Aux Adapter, 2 in 1 Bluetooth 5.0](https://www.amazon.de/dp/B0BNKJHGTL) 
* [I2S to bluetooth transmitter](https://www.tinysineaudio.com/products/tsa5001-bluetooth-5-3-audio-transmitter-board-i2s-digital-input) (in test phase)

### Connected GPIO pins

Pin      | Function | Application     | Arduino | Comment
| --     |   --     | --              | --      | --
| GPIO21 | SDA      | Disp I2C        | D2      |
| GPIO22 | SCL      | Disp I2C        | D1      |
| GPIO32 | DigIn    | Rotary 1 CLK (A)| D6      | select Volume
| GPIO33 | DigIn    | Rotary 1 DT (B) | D5      |
| GPIO35 | DigIn    | Rotary 1 SW     | D7      | add 10K Pullup Resistor
| GPIO17 | DigIn    | Rotary 2 CLK (A)|         | select Station 
| GPIO14 | DigIn    | Rotary 2 DT (B) |         |
| GPIO16 | DigIn    | Rotary 2 SW     |         | add 10K Pullup Resistor
| GPIO25 | LRC      | PCM 5102 (I2S)  |         | I2S Amplifier
| GPIO26 | BCLK     | PCM 5102 (I2S)  |         | I2S Amplifier
| GPIO27 | DIN      | PCM 5102 (I2S)  |         | I2S Amplifier

## Implementation

### State Transition Model

My implementation works with different states. Events like pressing a button or a successful firmware upload triggers the transition to another state.

![state transition](/docs/state_transition_diagram.svg "state transition diagram")


## TODOs

### Fixes

* show upload state on display e.g. progress bar
* fix display error when title contains "special chars"
* handle wifi connection loss while in standby

### Features

* use F-macro
* use mini CSS lib for html
* Change button long-press behaviour: When button is pressed long enough, the action happens without releasing the pressed button. So you don't have to count to know when it's time to release the button
* personalized favlist


## Libs

### Audio

* [ESP32-audioI2S](https://github.com/schreibfaul1/ESP32-audioI2S.git)
* [ESP32-vs1053_ext](https://github.com/schreibfaul1/ESP32-vs1053_ext)
* [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio)

### Display/Graphics

* [LiquidCrystal_I2C_ESP32](https://registry.platformio.org/libraries/iakop/LiquidCrystal_I2C_ESP32)
* [Adafruit GFX Graphics Library](https://learn.adafruit.com/adafruit-gfx-graphics-library?view=all)

### Rotary Enocder
* [ai-esp32-rotary-encoder](https://github.com/igorantolic/ai-esp32-rotary-encoder/blob/master/examples/Esp32RotaryEncoderBasics/Esp32RotaryEncoderBasics.ino)


### Preferences
* [Tutorial](https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/)

### Webserver
* [ESPAsyncWebServer](https://github.com/me-no-dev/ESPAsyncWebServer)

## Protocols

### SPI

#### Default VSPI Pins:

Function | GPIO
-- | --
MOSI (Master Out slave in) | 23
MISO (Master in Slave out) | 19
SCK (Serial Clock)         | 18 
DC                         | 21 | lcdgfx
RST                        | 22 | lcdgfx
CS/SS (Chip Select)        | 5   


Use [SPI Pin Test](https://randomnerdtutorials.com/esp32-spi-communication-arduino/#esp32-spi-peripherals) to ask MCU for actual pins.


### I2C

Default I2C pins on ESP32:

Function | GPIO
 -- | --
SDA | 21
SCL | 22

Use [I2C Scanner Code](https://randomnerdtutorials.com/esp32-esp8266-i2c-lcd-arduino-ide/) to test if device is detected and to get its address


### Inter-IC Sound (I2S)
 
Some [I2S basics](https://en.wikipedia.org/wiki/I%C2%B2S) from Wikipedia:

Label   | Synonyms  | Function
 --     | --        | --
SCK     | BCLK      | Bit clock line: Officially "continuous serial clock (SCK)".[1] Typically written "bit clock (BCLK)"
WS      | LRCLK, FS | Officially "word select (WS)". Typically called "left-right clock (LRCLK)" or "frame sync (FS)".
SD      | SDIN, SDATA | Officially "serial data (SD)",  but can be called SDATA, SDIN, SDOUT, DACDAT, ADCDAT, etc.[3]
MC        |           | Master clock: This is not part of the I2S standard,but is commonly included for synchronizing the internal operation of the analog/digital converters.


I2S pins on ESP32:

Function    | GPIO
 --         | --
SCK         | 14
WS          | 15
SD          | 22


* [Espressif Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)
* [ALL YOU NEED TO KNOW ABOUT I2S](https://hackaday.com/2019/04/18/all-you-need-to-know-about-i2s/)
* ESP32 Audio Tutorial with lots of examples (Andreas Spiess): [Video](https://www.youtube.com/watch?v=a936wNgtcRA)
* DroneBot Workshop ESP32 Sound - Working with I2S [Blog](https://dronebotworkshop.com/esp32-i2s/), [Video](https://www.youtube.com/watch?v=m-MPBjScNRk&t=2377s)
* [Blog post explains I2S](https://forum.arduino.cc/t/i-s-audio/670650/4)
* https://forum.arduino.cc/t/giving-two-outputs-in-one-i2s-port/988109

## Various ESP32 Boards

* [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)
* ESP API Guide [Partion Tables](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/partition-tables.html)
* [Video: How do OTA Updates work, including an explanation of partion tables](https://www.youtube.com/watch?v=zXqhv_iy_lI&list=PLYwlctyeRCy3EseqeheBXB8ndZs-U8dbD&index=2&t=231s)
* [Partion Table Calculator Sheet](https://docs.google.com/spreadsheets/d/1M6_qx00ZR82jyWa2slCMcvLtnXR9Q9dnis5mR5YChW8/edit#gid=0)
* [More Memory for the ESP32 without Soldering (How-to adapt partition sizes)](https://www.youtube.com/watch?app=desktop&v=Qu-1RK4Fk7g)



### ESP32 DEV KIT C V4

* [Pinout](https://live.staticflickr.com/4764/40089095211_ec1fee0087_b.jpg)
* [Supplier AZ Delivery](https://www.az-delivery.de/products/esp32-dev-kit-c-v4-unverlotet)
* ESP32 with ext. antenna [esp32-devkitc-v4-wroom32u](https://f3-innovator.de/esp32-devkitc-v4-wroom32u)

### ESP32 WROOM D1-MINI

PLATFORM: Espressif 32 (6.4.0) > Espressif ESP32 Dev Module
HARDWARE: ESP32 240MHz, 320KB RAM, 4MB Flash

Chip is ESP32-D0WDQ6 (revision v1.0)
Features: WiFi, BT, Dual Core, 240MHz, VRef calibration in efuse, Coding Scheme None
Crystal is 40MHz
MAC: 84:cc:a8:5e:c3:ac

#### Pinout
* https://www.fambach.net/d1-mini-esp8266-modul-2-2/


## Various Displays

### LCD blue 4x20 Zeichen, HD44780, I2C

* [Technische Daten](https://www.az-delivery.de/products/hd44780-2004-lcd-display-bundle-4x20-zeichen-mit-i2c-schnittstelle)
* I2C device found at address 0x27
* [Scrolling Text Example](https://randomnerdtutorials.com/esp32-esp8266-i2c-lcd-arduino-ide/)

The display 
* is extremely dependent on the viewing angle
* is extremely slow via I2C
* brightness is not changeable via I2C

### OLED Color 128x128, SPI, SSD1327 
* https://www.waveshare.com/w/upload/8/80/1.5inch_OLED_Module_User_Manual_EN.pdf
* https://eckstein-shop.de/Waveshare128x128General15inchOLEDdisplayModuleEN
* [SPI Connection Display ESP32](https://esphome.io/components/display/ssd1327.html#over-spi)
* Display Lib SSD1327 [lcdgfx](https://github.com/lexus2k/lcdgfx/)
* Display Lib SSD1327 [hexaguin](https://github.com/hexaguin/SSD1327)
* NEW: https://robpo.github.io/Paperino/exampleGFXdemo/, https://learn.adafruit.com/096-mini-color-oled


### OLED 128x64, monochrome, I2C, SSD1306

Eigenschaft         | Wert
------------------- | --
Betriebsspannung    | 3.3V bis 5V
Bus                 | I2C
Address             | I2C device found at address 0x3C

* [AZDevlivery](https://www.az-delivery.de/products/0-96zolldisplay)
* https://randomnerdtutorials.com/esp32-esp8266-i2c-lcd-arduino-ide/

## Variuos Amplifiers

#### HiLetgo PCM5102 I2S IIS Lossless Digital Audio DAC Decoder

Input via I2S

* [PSchatzmann Wiki](https://github.com/pschatzmann/arduino-audio-tools/wiki/External-DAC#hiletgo-pcm5102-i2s-iis-lossless-digital-audio-dac-decoder)
* [Datasheet](https://www.ti.com/lit/ds/symlink/pcm5102a.pdf)
* [Demo Project](https://www.hackster.io/esikora/esp32-audio-project-part-i-internet-radio-with-i-s-dac-a5515c)


#### MAX98375a

Input via I2S

* [Datasheet](https://eckstein-shop.de/AdafruitI2S3WClassDAmplifierBreakout-MAX98357A)
* [PSchatzmann Wiki](https://github.com/pschatzmann/arduino-audio-tools/wiki/External-DAC#i2s-3w-class-d-amplifier-breakout---max98357a)


## Various Rotary Encoders

* [KY-040](https://www.reichelt.de/entwicklerboards-drehwinkel-encoder-ky-040-debo-encoder-p282545.html)
* Rotary encoder with [RGB LED Push Button](https://eckstein-shop.de/SparkfunRotaryEncoder-IlluminatedRGBEN)
* Debouncing Circuit [Video by Ralph S Bacon](https://www.youtube.com/watch?v=b2uUYiGrS5Y)

## Other ESP32-based Internet Radio Projects

### ESP32 I2S amplifier (I2S -> MAX98357a)

* https://www.az-delivery.de/blogs/azdelivery-blog-fur-arduino-und-raspberry-pi/internetradio-mit-esp32-und-max-98357a
* https://www.elektormagazine.de/labs/esp32-internet-radio-1

### ESP32 analog amplifier (ESP DAC -> PAM8403)

* [Akkubetrieb, USB-Laderegler](https://www.az-delivery.de/blogs/azdelivery-blog-fur-arduino-und-raspberry-pi/internet-radio-mit-dem-esp32)
* [Simple Demo](https://elektro.turanis.de/html/prj466/index.html)

### M5StickC plus (ESP32 Pico), external I²S DAC (PCM5102A)

* [hackster.io Blog Post](https://www.hackster.io/esikora/esp32-audio-project-part-i-internet-radio-with-i-s-dac-a5515c)

### ESP32 Internet Radio on TTGO T-Display board

* OLED Display on Board, 8-Bit Audio? see [Video](https://www.youtube.com/watch?v=lThawtfSQYA&t=27s)

### by Wolle
* [ESP32-MiniWebRadio](https://github.com/schreibfaul1/ESP32-MiniWebRadio)

### by Andreas Spiess

* [#195 DIY Internet Radio using an ESP32 ](https://www.youtube.com/watch?v=hz65vfvbXMs)

### ESP32, VS1053 (MP3 Deoder), PAM8403 (digital Amp), 3,5" LCD 

https://www.instructables.com/Internet-Radio-Using-an-ESP32/

### ESP32 VS1053, TFT ILI9341 Touchdisplay by Ralph S Bacon

Ralph shows how he built and improved his webradio in several VLOG videos:

* [#204 TFT Touch Screen ILI9341 SPI for ESP32 (Internet Radio Research)](https://www.youtube.com/watch?v=wMJFkhmp2UE&pp=ygUTUmFscGggUyBCYWNvbiByYWRpbw%3D%3D)
* [#205 ESP32 Internet Radio with VS1053 MP3 decoder and ILI9341 TFT](https://www.youtube.com/watch?v=xrR8EZh2bMI&pp=ygUWUmFscGggUyBCYWNvbiB3ZWJyYWRpbw%3D%3D)
* [#206 ESP32 Circular Buffer for Internet Radio - and ESP32 WiFi Woes](https://www.youtube.com/watch?v=6BK4fzRaFGY&t=2s&pp=ygUWUmFscGggUyBCYWNvbiB3ZWJyYWRpbw%3D%3D)
* [#208 Using an ESP32 Task for my Web Radio - using the Arduino IDE](https://www.youtube.com/watch?v=aMS4XwEr8s0&pp=ygUWUmFscGggUyBCYWNvbiB3ZWJyYWRpbw%3D%3D)
* [#235 Bluetooth Audio🔊Transmitter (KCX_BT_EMITTER) - with AT commands](https://www.youtube.com/watch?v=ZQ5MWcis8rA&t=8s&pp=ygUTUmFscGggUyBCYWNvbiByYWRpbw%3D%3D)


## Lost + Found

* Changing partition table? Use [Hex Calculator](https://www.calculator.net/hex-calculator.htm) 
* [The evils of arduino strings](https://hackingmajenkoblog.wordpress.com/2016/02/04/the-evils-of-arduino-strings/)
* [ESP32 Logging](https://thingpulse.com/esp32-logging/)
* https://www.pschatzmann.ch/home/2021/04/30/back-to-basics-sampling-audio-with-the-esp32/#comment-1663

