#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>

#include "Audio.h"
#include "AiEsp32RotaryEncoder.h"

#include "ArduinoLog.h"

#include "commons.h"
#include "stations.h"
#include "display.h"

Preferences preferences;    // https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/
Audio audio;

String ssid =     "***";
String password = "***";
String hostname = "WEBRADIO_ESP32";

int displayLastUpdatedMinute = -1;

// ESP32 GPIO to PCM5102a header
// -> LCK
#define I2S_LRC     25
// -> DIN
#define I2S_DOUT    27
// -> BCK
#define I2S_BCLK    26

// ESP32 GPIO to Rotary Encoder Volume
#define ROT_VOL_CLK_A   32
#define ROT_VOL_DT_B    33
#define ROT_VOL_SW_BTN  35
#define ROT_VOL_STEPS   2
#define ROT_VOL_VCC     -1

// ESP32 GPIO to Rotary Encoder Station
#define ROT_STA_CLK_A   17
#define ROT_STA_DT_B    14
#define ROT_STA_SW_BTN  16
#define ROT_STA_STEPS   2
#define ROT_STA_VCC     -1

// Range [0 - 255]
#define VOL_MAX_STEPS   15
#define VOL_START       5

#define NO_CIRCLE_VALUES false

#define MAX_WIFI_CONNECT_TRIES  20

#define SCROLL_MSG_SPEED_MS     900
#define MENU_TIMEOUT_MS         9900
#define DISPLAY_TIME_UPDATE_MS  1000
#define AUTOSAVE_PREFS_MS       29333

#define SHORT_BTN_PUSH_MS       50
#define LONG_BTN_PUSH_MS        1000

#define PKEY_LAST_VOL       "lastVol"
#define PKEY_LAST_FAV_IDX   "lastFav"
#define PKEY_LAST_RUN_MODE  "lastRMo"

#define RUN_MODE_STANDBY    0
#define RUN_MODE_STARTING   1
#define RUN_MODE_RESTARTING 2
#define RUN_MODE_RADIO      3
#define RUN_MODE_OTA        4


AiEsp32RotaryEncoder rotVolume = AiEsp32RotaryEncoder(ROT_VOL_CLK_A, ROT_VOL_DT_B, ROT_VOL_SW_BTN, ROT_VOL_VCC, ROT_VOL_STEPS);
AiEsp32RotaryEncoder rotStation = AiEsp32RotaryEncoder(ROT_STA_CLK_A, ROT_STA_DT_B, ROT_STA_SW_BTN, ROT_STA_VCC, ROT_STA_STEPS);

SimpleTimer timer;

uint8_t currentFavorite = 0;
char scrollingStreamTitleBuffer[MAXLEN_SCROLL_STREAMTITLE_BUFFER];
struct tm timeinfo;

const char* ntpServer = "pool.ntp.org";
const char* tzBerlin = "CET-1CEST,M3.5.0,M10.5.0/3";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 3600;

int timerTimeDisplayUpdate = -1;
int timerMenuDispTimeout = -1;
int timerScrollMsgTick = -1;
int timerSavePreferences = -1;

unsigned long shortPressAfterMiliseconds = SHORT_BTN_PUSH_MS;   //how long a short push shoud be. Do not set too low to avoid bouncing.
unsigned long longPressAfterMiliseconds = LONG_BTN_PUSH_MS;     //how long a long push shoud be.

boolean menuActive = false;
boolean prefsHaveChanged = false;

uint8_t runMode = RUN_MODE_RADIO;

void updateTimeDisplayCB();

void printEspChipInfo() {
  Log.infoln("Chip model %s (rev %d), #cores %d, CPU Freq %d MHz", ESP.getChipModel(), ESP.getChipRevision(), ESP.getChipCores(), ESP.getCpuFreqMHz());
  Log.infoln("FlashChipSize %d KB, FlashChipSpeed %d", ESP.getFlashChipSize()/1024, ESP.getFlashChipSpeed());
}


void onStatButtonClick()
{
    static unsigned long lastTimeStatBtnPressed = 0; // Soft debouncing
   
    if (millis() - lastTimeStatBtnPressed < 500)
    {
        return;
    }

    lastTimeStatBtnPressed = millis();

    Serial.printf("STATION button pressed for %lu ms. ", millis() - lastTimeStatBtnPressed);

    if (menuActive) {
        uint8_t newFavorite = rotStation.readEncoder();
        
        displayClearLine(1);

        if (newFavorite != currentFavorite) {
            currentFavorite = newFavorite;
            audio.connecttohost(favArr[currentFavorite].url);
            timer.disable(timerScrollMsgTick);
            displayMessage(2, favArr[currentFavorite].name);
            displayClearLine(3);
            prefsHaveChanged = true;
        }
        menuActive = false;
        
        timer.disable(timerMenuDispTimeout);
    }

}

void onVolBtnShortClick() {
    Serial.print("Volbutton SHORT press ");
    Serial.print(millis());
    Serial.println(" milliseconds after restart");

    // Switch Standby -> Radio
    if (runMode == RUN_MODE_STANDBY) {
        runMode = RUN_MODE_RADIO;
        Serial.printf("Switching on. runMode %d\n", runMode);
        prefsHaveChanged = true;
        displayClearLine(1);        // TODO: clear()?
        displayClearLine(2);
        displayClearLine(3);
        displayVolume(audio.getVolume());
        displayLastUpdatedMinute = -1;
        updateTimeDisplayCB();
        audio.connecttohost(favArr[currentFavorite].url);
        displayMessage(2, favArr[currentFavorite].name);
        timer.enable(timerTimeDisplayUpdate);
    }
}

void onVolBtnLongClick() {
    Serial.print("Volbutton LONG press ");
    Serial.print(millis());
    Serial.println(" milliseconds after restart");
    
    // Switch Radio -> Standby
    if (runMode == RUN_MODE_RADIO) {
        timer.disable(timerScrollMsgTick);
        timer.disable(timerMenuDispTimeout);
        timer.disable(timerTimeDisplayUpdate);
        audio.stopSong();
        displayClear();
        displayLastUpdatedMinute = -1;
        runMode = RUN_MODE_STANDBY;
        updateTimeDisplayCB();
        prefsHaveChanged = true;
        timer.enable(timerTimeDisplayUpdate);
        Serial.printf("Switching off. runMode %d\n", runMode);
    }
}

void handleRotaryVolBton() {
    static unsigned long lastTimeButtonDown = 0;
    static bool wasButtonDown = false;

    bool isEncoderButtonDown = rotVolume.isEncoderButtonDown();
    //isEncoderButtonDown = !isEncoderButtonDown; //uncomment this line if your button is reversed

    if (isEncoderButtonDown) {
    if (!wasButtonDown) {
        //start measuring
        lastTimeButtonDown = millis();
    }
    //else we wait since button is still down
    wasButtonDown = true;
    return;
  }

  //button is up
  if (wasButtonDown) {
    //click happened, lets see if it was short click, long click or just too short
    if (millis() - lastTimeButtonDown >= longPressAfterMiliseconds) {
      onVolBtnLongClick();
    } else if (millis() - lastTimeButtonDown >= shortPressAfterMiliseconds) {
      onVolBtnShortClick();
    }
  }
  wasButtonDown = false;
}


void rotaryLoop()
{
    if (rotVolume.encoderChanged())
    {
        if (runMode == RUN_MODE_RADIO) {
            uint8_t newVol = MIN(rotVolume.readEncoder(), VOL_MAX_STEPS);
            //Serial.print("VOLUME Encoder Value: "); Serial.println(newVol);

            newVol = MIN(newVol, VOL_MAX_STEPS);
            newVol = MAX(0, newVol);
            audio.setVolume(newVol);
            displayVolume(newVol);
            prefsHaveChanged = true;
        }
    }

    if (rotStation.encoderChanged())
    {
        if (runMode == RUN_MODE_RADIO) {
            if (!menuActive) {
                menuActive = true;
                timer.enable(timerMenuDispTimeout);
            } else {
                timer.restartTimer(timerMenuDispTimeout);
            }

            long selectedFavorite = rotStation.readEncoder();
            Serial.print("STATION Value: ");
            Serial.print(selectedFavorite);
            Serial.print(", ");
            StationInfo stationInfo = favArr[selectedFavorite];
            Serial.println(stationInfo.name);        
            displayMessage(1, stationInfo.name);
        }
    }
    if (rotStation.isEncoderButtonClicked())
    {
            onStatButtonClick();
    }

    handleRotaryVolBton();
}


/* ISRs */

void IRAM_ATTR readVolEncoderISR()
{
    rotVolume.readEncoder_ISR();
}

void IRAM_ATTR readStaEncoderISR()
{
    rotStation.readEncoder_ISR();
}


void printLocalTime(){
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");
        return;
    }

    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

/* Callbacks */

void scrollMsgTickCB(){
    //Log.infoln("scrollMsgTickCB");    
    displayScrollingMsgTick(3);
}

void menuDispTimeoutCB() {
    Log.infoln("menuDispTimeoutCB");
    displayClearLine(1);
    rotStation.setEncoderValue(currentFavorite);
    menuActive = false;
    timer.disable(timerMenuDispTimeout);
}

void savePreferencesCB() {
    if (prefsHaveChanged) {
        preferences.putInt(PKEY_LAST_VOL, audio.getVolume());
        preferences.putInt(PKEY_LAST_FAV_IDX, currentFavorite);
        preferences.putInt(PKEY_LAST_RUN_MODE, runMode);
        prefsHaveChanged = false;
        Serial.println("Saved preferences");
    }
}

void updateTimeDisplayCB() {
    if(!getLocalTime(&timeinfo)){
        Serial.println("Failed to obtain time");        // TODO: Show errormsg on LCD
        displayClearLine(0);
        return;
    }

    displayWifiSignal(WiFi.RSSI());

    if (runMode == RUN_MODE_RADIO) {
        if (timeinfo.tm_min != displayLastUpdatedMinute) {
            char datetime[13];
            //char line[22];
            strftime(datetime, 13, "%d.%m. %H:%M" , &timeinfo); // "%Y-%m-%d %H:%M", "%A, %B %d %H:%M:%S"
            //sprintf(line, "%3d%17s", WiFi.RSSI(), datetime);
            displayDateTime(datetime);
            Serial.print("Update Time Display: "); Serial.println(&timeinfo, "'%d.%m. %H:%M'");    
            displayLastUpdatedMinute = timeinfo.tm_min;
        } else {
            displayTimeSecBlink (timeinfo.tm_sec % 2);
        } 
    } else if (runMode == RUN_MODE_STANDBY) {
        char time[9];
        char date1[12]; 
        char date2[21];
        strftime(time, 9, "%H:%M:%S" , &timeinfo); // "%Y-%m-%d %H:%M", "%A, %B %d %H:%M:%S"
        strftime(date1, 10, "%A," , &timeinfo); 
        strftime(date2, 21, "%B %d" , &timeinfo);
        displayDateTime(time, date1, date2);
    }
}

int setupRotVolume() {
    rotVolume.begin();
    rotVolume.setup(readVolEncoderISR);
    rotVolume.setBoundaries(0, VOL_MAX_STEPS-1, NO_CIRCLE_VALUES); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)
    rotVolume.setEncoderValue(VOL_START);
    rotVolume.disableAcceleration(); //acceleration is now enabled by default - disable if you dont need it
    //rotVolume.setAcceleration(20); 

    long lastVol = preferences.getInt(PKEY_LAST_VOL, VOL_START);

    Serial.printf("lastVol = %lu (from prefs), min = %d, max = %d ", lastVol, 0, VOL_MAX_STEPS);

    // input sanitation
    lastVol = MIN(lastVol, VOL_MAX_STEPS-1);
    lastVol = MAX(0, lastVol);

    rotVolume.setEncoderValue(lastVol);

    Serial.print("After Sanitation: lastVol = "); Serial.println(lastVol);
    return lastVol;
}

int setupRotStation() {
    rotStation.begin();
    rotStation.setup(readStaEncoderISR);
    rotStation.setBoundaries(0, ARRAY_LEN(favArr) - 1, NO_CIRCLE_VALUES); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)
    rotStation.setEncoderValue(0);
    //rotStation.setAcceleration(0); //or set the value - larger number = more accelearation; 0 or 1 means disabled acceleration
    rotStation.disableAcceleration();
    
    Serial.print("Favoritelist contains elements: "); Serial.println(ARRAY_LEN(favArr));

    currentFavorite = preferences.getInt(PKEY_LAST_FAV_IDX, currentFavorite);

    // input sanitation
    if ((currentFavorite > ARRAY_LEN(favArr) - 1) || (currentFavorite < 0)) {
        Serial.println("Favindex from prefs is out of range for current favorite list");
    }
    currentFavorite = MIN(currentFavorite, ARRAY_LEN(favArr) - 1);
    currentFavorite = MAX(0, currentFavorite);

    rotStation.setEncoderValue(currentFavorite);

    Serial.print("currentFavorite = "); Serial.print(currentFavorite); Serial.print(" "); Serial.println(favArr[currentFavorite].name);
    return currentFavorite;
}


void setup() {
    preferences.begin("webradio", false); 
    
    Serial.begin(115200);
    delay(5000);

    int lastRunMode = preferences.getInt(PKEY_LAST_RUN_MODE, RUN_MODE_RADIO);
    Serial.printf("lastRunMode %d\n", lastRunMode);

    setup_display();

    if (lastRunMode == RUN_MODE_RESTARTING) {
        // TODO: Goto OTA mode
        Serial.println("Recovering from an automatic restart. Switching on.");
        runMode = RUN_MODE_RADIO;
        displayClear();
    }

    Log.begin(LOG_LEVEL_TRACE, &Serial, true);
    printEspChipInfo();

    timerTimeDisplayUpdate = timer.setInterval(DISPLAY_TIME_UPDATE_MS, updateTimeDisplayCB);
    timerMenuDispTimeout = timer.setInterval(MENU_TIMEOUT_MS, menuDispTimeoutCB);
    timer.disable(timerMenuDispTimeout);
    timerScrollMsgTick = timer.setInterval(SCROLL_MSG_SPEED_MS, scrollMsgTickCB);
    timer.disable(timerScrollMsgTick);
    timerSavePreferences = timer.setInterval(AUTOSAVE_PREFS_MS, savePreferencesCB);

    Serial.print("Connecting to wifi ");
    WiFi.disconnect();
    WiFi.mode(WIFI_STA);
    
    WiFi.setHostname(hostname.c_str()); //TODO: no effect?
    
    displayMessage(0, "Connecting to wifi");
    displayMessage(1, ssid.c_str());

    WiFi.begin(ssid.c_str(), password.c_str());
    uint8_t t = 0;
    while ((WiFi.status() != WL_CONNECTED) && (t < MAX_WIFI_CONNECT_TRIES)) {
        t++;
        Serial.print('.');
        displayBar(2, t);
        delay(1500);
    }

    if (WiFi.status() != WL_CONNECTED) {
        runMode = RUN_MODE_RESTARTING;
        prefsHaveChanged = true;
        savePreferencesCB();
        Serial.println("Could not connect to Wifi. Check Config. Restarting in 10s");
        displayMessage(0, "Couldn't connect to");
        displayMessage(2, "Check Wifi Config");
        displayMessage(3, "Restarting in 10s");
        delay(10000);
        ESP.restart();
    }

    Serial.println(" success");
    Serial.printf("IP %s, Signal (RRSI): %d\n", WiFi.localIP().toString().c_str(), WiFi.RSSI());
        
    displayMessage(0, "Wifi connected");

    configTzTime(tzBerlin, ntpServer, ntpServer, ntpServer);
    Serial.printf("Time Configured: %s, %s\n", tzBerlin, ntpServer);
    printLocalTime();

    delay(2000);
    displayClearLine(0);
    displayClearLine(1);
    delay(500);
    
    timer.enable(timerTimeDisplayUpdate);

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT, -1);
    audio.setVolumeSteps(VOL_MAX_STEPS+1);
    audio.setVolume(setupRotVolume());
    audio.setConnectionTimeout(500, 2700);

    setupRotStation();

    displayWifiSignal(WiFi.RSSI());

    if (lastRunMode == RUN_MODE_RADIO) {
        displayVolume(audio.getVolume());
        audio.connecttohost(favArr[currentFavorite].url);
        displayMessage(2, favArr[currentFavorite].name);
        runMode = lastRunMode;
    } else if (lastRunMode == RUN_MODE_STANDBY) {
        displayClear();
        runMode = lastRunMode;
    } 
    
    Serial.printf("Start in runMode = %d\n", runMode);
    Serial.println("\n<<<<<<<<<<<< setup finished >>>>>>>>>>>\n");
}

void loop(){
    audio.loop();
    rotaryLoop();
    timer.run();
}

// optional
void audio_info(const char *info){
    Serial.print("info        "); Serial.println(info);
}
void audio_id3data(const char *info){  //id3 metadata
    Serial.print("id3data     ");Serial.println(info);
}
void audio_eof_mp3(const char *info){  //end of file
    Serial.print("eof_mp3     ");Serial.println(info);
}
void audio_showstation(const char *info){
    Serial.print("station     ");Serial.println(info);
   // displayMessage(2, info);
}
void audio_showstreamtitle(const char *info){
    strlcpy(scrollingStreamTitleBuffer, info, MAXLEN_SCROLL_STREAMTITLE_BUFFER / 2);
    Serial.printf("currentStreamTitle '%s'  %d\n", scrollingStreamTitleBuffer, strlen(scrollingStreamTitleBuffer));

    if (strlen(info) > DISPLAY_COLS) {
        displayScrollingMsgStart(3, timerScrollMsgTick);
        timer.enable(timerScrollMsgTick);
    } else {
        displayScrollingMsgStop(3, timerScrollMsgTick);
        timer.disable(timerScrollMsgTick);
        displayMessage(3, info);
    }
}
void audio_bitrate(const char *info){
    Serial.print("bitrate     ");Serial.println(info);
}
void audio_commercial(const char *info){  //duration in sec
    Serial.print("commercial  ");Serial.println(info);
}
void audio_icyurl(const char *info){  //homepage
    Serial.print("icyurl      ");Serial.println(info);
}
void audio_lasthost(const char *info){  //stream URL played
    Serial.print("lasthost    ");Serial.println(info);
}
void audio_eof_speech(const char *info){
    Serial.print("eof_speech  ");Serial.println(info);
}