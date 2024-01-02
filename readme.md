# ESP32 Webradio

This project is a port of a project published by Gerald Lechner in an [article](https://www.az-delivery.de/blogs/azdelivery-blog-fur-arduino-und-raspberry-pi/internetradio-mit-esp32-und-max-98357a?comment=134821511435&page=1) at the AZ Delivery blog.

I've ported the original code from an Arduino IDE compatible structure to a PlatformIO version. I've also collected information (e.g. optionals displays, amplifiers, used protocols and libraries) on the subject of buildung an internet radio based on an ESP32 microcontroller - see documentation below.


## ESP32 Connections

Used GPIOs pins.

Pin      | Function | Application     | Arduino | Comment
--       |   --     | --              | --      | --
| GPIO21 | SDA      | Disp I2C        | D2 |
| GPIO22 | SCL      | Disp I2C        | D1 |
| GPIO33 | DigIn    | Rotary 1 CLK (A)| D6 |
| GPIO32 | DigIn    | Rotary 1 DT (B) | D5 |
| GPIO35 | DigIn    | Rotary 1 SW     | D7 |  10K Pullup Resistor
| GPIO14 | DigIn    | Rotary 2 CLK (A)|    |
| GPIO13 | DigIn    | Rotary 2 DT (B) |    |
|        |          | Rotary 2 SW     |    | not used
| GPIO25 | LRC      | Amp Left I2S    |    | 2. [Amp MAX98357a](https://www.az-delivery.de/products/i2s-3w-class-d-amplifier-breakout-max98357a) parallel (+5V over 470k $\Omega$)
| GPIO26 | BCLK     | Amp Left I2S    |    |
| GPIO27 | DIN      | Amp Left I2S    |    |



## Libs

### Audio

* [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio)

### Display/Graphics

* [LiquidCrystal_I2C_ESP32](https://registry.platformio.org/libraries/iakop/LiquidCrystal_I2C_ESP32)
* [Adafruit GFX Graphics Library](https://learn.adafruit.com/adafruit-gfx-graphics-library?view=all)

### Rotary Enocder
* [ai-esp32-rotary-encoder](https://github.com/igorantolic/ai-esp32-rotary-encoder/blob/master/examples/Esp32RotaryEncoderBasics/Esp32RotaryEncoderBasics.ino)

## Protocols

### SPI

* [SPI Pin Test](https://randomnerdtutorials.com/esp32-spi-communication-arduino/#esp32-spi-peripherals)

#### Default VSPI Pins:

Function | GPIO
-- | --
MOSI (Master Out slave in) | 23
MISO (Master in Slave out) | 19
SCK (Serial Clock)         | 18 
DC                         | 21 | lcdgfx
RST                        | 22 | lcdgfx
CS/SS (Chip Select)        | 5   


### I2C

Default I2C pins:

Function | GPIO
 -- | --
SDA | 21
SCL | 22

* Use [I2C Scanner Code](https://randomnerdtutorials.com/esp32-esp8266-i2c-lcd-arduino-ide/) to test if device is detected and to get its address


### Inter-IC Sound (I2S)

* [Espressif Docs](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/i2s.html)
* ESP32 Audio Tutorial with lots of examples (Andreas Spiess): [Video](https://www.youtube.com/watch?v=a936wNgtcRA)
* DroneBot Workshop ESP32 Sound - Working with I2S [Blog](https://dronebotworkshop.com/esp32-i2s/), [Video](https://www.youtube.com/watch?v=m-MPBjScNRk&t=2377s)

## Various ESP32 Boards

[ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/)


### ESP32 DEV KIT C V4

* [Pinout](https://live.staticflickr.com/4764/40089095211_ec1fee0087_b.jpg)
* [Supplier AZ Delivery](https://www.az-delivery.de/products/esp32-dev-kit-c-v4-unverlotet)

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

#### MAX98375a

* [Datasheet](https://eckstein-shop.de/AdafruitI2S3WClassDAmplifierBreakout-MAX98357A)

#### PCM5102 DAC

* [Datasheet](https://www.ti.com/lit/ds/symlink/pcm5102a.pdf)
* [Demo Project](https://www.hackster.io/esikora/esp32-audio-project-part-i-internet-radio-with-i-s-dac-a5515c)


## Other ESP-based Internet Radio Projects

### ESP32 I2S amplifier (I2S -> MAX98357a)

* https://www.az-delivery.de/blogs/azdelivery-blog-fur-arduino-und-raspberry-pi/internetradio-mit-esp32-und-max-98357a

* https://www.elektormagazine.de/labs/esp32-internet-radio-1

### ESP32 analog amplifier (ESP DAC -> PAM8403)

* [Akkubetrieb, USB-Laderegler](https://www.az-delivery.de/blogs/azdelivery-blog-fur-arduino-und-raspberry-pi/internet-radio-mit-dem-esp32)
* [Simple Demo](https://elektro.turanis.de/html/prj466/index.html)


### ESP32 Internet Radio on TTGO T-Display board

* OLED Display on Board, 8-Bit Audio? see [Video](https://www.youtube.com/watch?v=lThawtfSQYA&t=27s)

