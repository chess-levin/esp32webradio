#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <WiFiClient.h>
#include <Preferences.h>
#include <SPIFFS.h>

#include "ArduinoJson.h"
#include "Audio.h"
#include "AiEsp32RotaryEncoder.h"

#include "commons.h"
#include "stations_default.h"
#include "stations.h"
#include "display.h"
#include "www.h"


Preferences preferences;    // https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/
Audio audio;

boolean writePreferencesOnSetup = false; // read values from preferences or from code

String ssid =     "";
String password = "";
String hostname = "WEBRADIO_ESP32";

const char *stationsFilename = "/stations.json";

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
#define CIRCLE_VALUES true

#define MAX_WIFI_CONNECT_TRIES  20

#define START_MENU_CURSOR   '>'

#define SCROLL_MSG_SPEED_MS     900
#define MENU_TIMEOUT_MS         15330
#define DISPLAY_TIME_UPDATE_MS  1000
#define AUTOSAVE_PREFS_MS       29333

#define SHORT_BTN_PUSH_MS       50
#define LONG_BTN_PUSH_MS        1000

#define PKEY_LAST_VOL       "lastVol"
#define PKEY_LAST_FAV_IDX   "lastFav"
#define PKEY_LAST_RUN_MODE  "lastRMo"
#define PKEY_SSID           "ssid"
#define PKEY_WIFI_KEY       "wifiKey"


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

int timerDateTimeDisplayUpdate = -1;
int timerMenuDispTimeout = -1;
int timerScrollMsgTick = -1;
int timerSavePreferences = -1;

unsigned long shortPressAfterMiliseconds = SHORT_BTN_PUSH_MS;   //how long a short push shoud be. Do not set too low to avoid bouncing.
unsigned long longPressAfterMiliseconds = LONG_BTN_PUSH_MS;     //how long a long push shoud be.

boolean menuActive = false;
boolean prefsHaveChanged = false;
boolean fsMounted = false;

uint8_t runMode = RUN_MODE_RADIO;


bool mountFS() {
  if(SPIFFS.begin()) {
    return true;
  } else {
    log_e("Error while mounting SPIFFS");
    return false;
  }
}


void updateTimeDisplayCB();

void printEspChipInfo() {
  log_i("Chip model %s (rev %d), #cores %d, CPU Freq %d MHz", ESP.getChipModel(), ESP.getChipRevision(), ESP.getChipCores(), ESP.getCpuFreqMHz());
  log_i("FlashChipSize %d KB, FlashChipSpeed %d", ESP.getFlashChipSize()/1024, ESP.getFlashChipSpeed());
}


void onVolBtnShortClick() {
    log_d("Volume button SHORT press %ul milliseconds after restart", millis());

    // Switch Standby -> Radio
    if (runMode == RUN_MODE_STANDBY) {
        runMode = RUN_MODE_RADIO;
        log_v("Switching on. runMode %d", runMode);
        prefsHaveChanged = true;
        displayClearLine(1);        // TODO: clear()?
        displayClearLine(2);
        displayClearLine(3);
        displayVolume(audio.getVolume());
        displayLastUpdatedMinute = -1;
        updateTimeDisplayCB();
        audio.connecttohost(getStationInfo(currentFavorite).url);
        displayMessage(2, getStationInfo(currentFavorite).name);
        timer.enable(timerDateTimeDisplayUpdate);
    }

    if (runMode == RUN_MODE_START_MENU) {
        long i = rotVolume.readEncoder();
        log_d("Selected value %d", i);
        if (i == 0)
            restartInRunMode(RUN_MODE_STANDBY);
        else
            restartInRunMode(RUN_MODE_OTA);
    }
}

void onVolBtnLongClick() {
    log_d("Volume button LONG press %ul milliseconds after restart", millis());
    
    // Switch Radio -> Standby
    if (runMode == RUN_MODE_RADIO) {
        timer.disable(timerScrollMsgTick);
        timer.disable(timerMenuDispTimeout);
        timer.disable(timerDateTimeDisplayUpdate);
        audio.stopSong();
        displayClear();
        displayLastUpdatedMinute = -1;
        runMode = RUN_MODE_STANDBY;
        updateTimeDisplayCB();
        prefsHaveChanged = true;
        timer.enable(timerDateTimeDisplayUpdate);
        log_v("Switching off. runMode %d", runMode);
    } else if (runMode == RUN_MODE_OTA) {
        log_v("Return from OTA ( runMode %d) to standby", runMode);
        timer.disable(timerScrollMsgTick);
        timer.disable(timerMenuDispTimeout);
        timer.disable(timerDateTimeDisplayUpdate);
        displayClear();
        restartInRunMode(RUN_MODE_START_MENU);
    }
}


void onStatBtnShortClick() {
    log_d("Station button SHORT press %ul milliseconds after restart", millis());

    // select and connect to new radio station
    if (runMode == RUN_MODE_RADIO) {

        if (menuActive) {
            uint8_t newFavorite = rotStation.readEncoder();
            
            displayClearLine(1);

            if (newFavorite != currentFavorite) {
                currentFavorite = newFavorite;
                audio.connecttohost(getStationInfo(currentFavorite).url);
                timer.disable(timerScrollMsgTick);
                displayMessage(2, getStationInfo(currentFavorite).name);
                displayClearLine(3);
                prefsHaveChanged = true;
            }
            menuActive = false;
            
            timer.disable(timerMenuDispTimeout);
        }

    }
}

void onStatBtnLongClick() {
    log_d("Station button LONG press %ul milliseconds after restart", millis());

    restartInRunMode(RUN_MODE_START_MENU);
}


void handleRotaryVolBtn() {
    static unsigned long lastTimeButtonDown = 0;
    static bool wasButtonDown = false;

    bool isEncoderButtonDown = rotVolume.isEncoderButtonDown();
    //isEncoderButtonDown = !isEncoderButtonDown; //uncomment this line if your button is reversed

    if (isEncoderButtonDown) {
    if (!wasButtonDown) {
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

void handleRotaryStatBtn() {
    static unsigned long lastTimeButtonDown = 0;
    static bool wasButtonDown = false;

    bool isEncoderButtonDown = rotStation.isEncoderButtonDown();
    //isEncoderButtonDown = !isEncoderButtonDown; //uncomment this line if your button is reversed

    if (isEncoderButtonDown) {
    if (!wasButtonDown) {
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
      onStatBtnLongClick();
    } else if (millis() - lastTimeButtonDown >= shortPressAfterMiliseconds) {
      onStatBtnShortClick();
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

            newVol = MIN(newVol, VOL_MAX_STEPS);
            newVol = MAX(0, newVol);
            audio.setVolume(newVol);
            displayVolume(newVol);
            prefsHaveChanged = true;
        }

        if (runMode == RUN_MODE_START_MENU) {
            uint8_t newMenuPos = rotVolume.readEncoder();
            // TODO: this is only a quick&dirty solution for 2 menu items
            displayMoveChar(2 + abs(newMenuPos-1), 0, 2 + newMenuPos, 0, START_MENU_CURSOR); 
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
            stationInfo_t stationInfo = getStationInfo(selectedFavorite);
            log_d("STATION Value: %ld, %s", selectedFavorite, stationInfo.name);
            displayMessage(1, stationInfo.name);
        }
    }

    handleRotaryVolBtn();
    handleRotaryStatBtn();
}


void printLocalTime(){
    if(!getLocalTime(&timeinfo)){
        log_e("Failed to obtain time");
        return;
    }

    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
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


/* Callbacks */

void scrollMsgTickCB(){
    displayScrollingMsgTick(3);
}

void menuDispTimeoutCB() {
    log_v("menuDispTimeoutCB");
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
        preferences.putString(PKEY_SSID, ssid);
        preferences.putString(PKEY_WIFI_KEY, password);
        prefsHaveChanged = false;
        log_v("Saved preferences");
    }
}

void updateTimeDisplayCB() {
    if(!getLocalTime(&timeinfo)){
        log_e("Failed to obtain time");        // TODO: Show errormsg on LCD
        displayClearLine(0);
        return;
    }

    displayWifiSignal(WiFi.RSSI());
    
    // https://www.geeksforgeeks.org/strftime-function-in-c/
    if (runMode == RUN_MODE_RADIO) {
        if (timeinfo.tm_min != displayLastUpdatedMinute) {
            char datetime[13];
            strftime(datetime, 13, "%d.%m. %H:%M" , &timeinfo); // "%Y-%m-%d %H:%M", "%A, %B %d %H:%M:%S"
            displayDateTime(datetime);
            Serial.print("Update Time Display: "); Serial.println(&timeinfo, "'%d.%m. %H:%M'");    
            displayLastUpdatedMinute = timeinfo.tm_min;
        } else {
            displayTimeSecBlink (timeinfo.tm_sec % 2);
        } 
    } else if (runMode == RUN_MODE_STANDBY) {
        char time[9];
        char date1[13]; 
        char date2[21];
        strftime(time, 9, "%H:%M:%S" , &timeinfo); // "%Y-%m-%d %H:%M", "%A, %B %d %H:%M:%S"
        strftime(date1, 13, "%A," , &timeinfo);         // name of weekday Monday, Tuesday, Wednesday, Thursday, Friday, Saturday, Sunday
        strftime(date2, 21, "%B %d" , &timeinfo);       // full name of month
        displayDateTime(time, date1, date2);
    }
}

void setupRotLeftMenu() {
    rotVolume.begin();
    rotVolume.setup(readVolEncoderISR);
    rotVolume.setBoundaries(0, 1, CIRCLE_VALUES); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)
    rotVolume.setEncoderValue(0);
    rotVolume.disableAcceleration(); //acceleration is now enabled by default - disable if you dont need it
}

int setupRotVolume() {
    rotVolume.begin();
    rotVolume.setup(readVolEncoderISR);
    rotVolume.setBoundaries(0, VOL_MAX_STEPS-1, NO_CIRCLE_VALUES); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)
    rotVolume.setEncoderValue(VOL_START);
    rotVolume.disableAcceleration(); //acceleration is now enabled by default - disable if you dont need it
    //rotVolume.setAcceleration(20); 

    long lastVol = preferences.getInt(PKEY_LAST_VOL, VOL_START);

    log_d("lastVol = %lu (from prefs), min = %d, max = %d ", lastVol, 0, VOL_MAX_STEPS);

    // input sanitation
    lastVol = MIN(lastVol, VOL_MAX_STEPS-1);
    lastVol = MAX(0, lastVol);

    rotVolume.setEncoderValue(lastVol);

    log_d("After Sanitation: lastVol = %d", lastVol);
    return lastVol;
}

int setupRotStation() {
    rotStation.begin();
    rotStation.setup(readStaEncoderISR);
    rotStation.setBoundaries(0, getStationInfoSize() - 1, NO_CIRCLE_VALUES); //minValue, maxValue, circleValues true|false (when max go to min and vice versa)
    rotStation.setEncoderValue(0);
    //rotStation.setAcceleration(0); //or set the value - larger number = more accelearation; 0 or 1 means disabled acceleration
    rotStation.disableAcceleration();
    
    log_v("%s contains %d elements.", stationsFilename, getStationInfoSize());

    currentFavorite = preferences.getInt(PKEY_LAST_FAV_IDX, currentFavorite);

    // input sanitation
    if ((currentFavorite > getStationInfoSize()-1) || (currentFavorite < 0)) {
        log_w("Favindex from prefs is out of range for current favorite list");
    }
    currentFavorite = MIN(currentFavorite, getStationInfoSize() - 1);
    currentFavorite = MAX(0, currentFavorite);

    rotStation.setEncoderValue(currentFavorite);

    log_d("currentFavorite = %d %s", currentFavorite, getStationInfo(currentFavorite).name);
    return currentFavorite;
}

void showStartupMenu() {
    log_d("showStartupMenu");
    displayClear();
    displayMessage(0, "__Select Startmode__");
    displayMessage(2, "> Standby");
    displayMessage(3, "  Config/Update");
}

void setupTimers() {
    timerDateTimeDisplayUpdate = timer.setInterval(DISPLAY_TIME_UPDATE_MS, updateTimeDisplayCB);
    timer.disable(timerDateTimeDisplayUpdate);
    timerMenuDispTimeout = timer.setInterval(MENU_TIMEOUT_MS, menuDispTimeoutCB);
    timer.disable(timerMenuDispTimeout);
    timerScrollMsgTick = timer.setInterval(SCROLL_MSG_SPEED_MS, scrollMsgTickCB);
    timer.disable(timerScrollMsgTick);
    timerSavePreferences = timer.setInterval(AUTOSAVE_PREFS_MS, savePreferencesCB);
    log_v("Timers successfully configured");
}

void setup() {
    preferences.begin("webradio", false); 
    
    Serial.begin(115200);
    delay(2500);
    log_v("<<<<<<<<<<<< setup started >>>>>>>>>>>");

    int lastRunMode = preferences.getInt(PKEY_LAST_RUN_MODE, RUN_MODE_RADIO);
    log_v("lastRunMode %d", lastRunMode);

    setupDisplay();

    printEspChipInfo();

    setupTimers();

    fsMounted = mountFS();

    log_v("Mounting %s Filessystem: %s", "SPIFFS", (fsMounted)?"Succes":"Error");   

    if (lastRunMode == RUN_MODE_START_MENU) {
        runMode = lastRunMode;
        showStartupMenu();
        setupRotLeftMenu();
    }

    if (lastRunMode == RUN_MODE_OTA) {
        runMode = lastRunMode;

        log_v("Starting OTA setup");
        displayClear();
        displayMessage(0, "Starting OTA setup");

        WiFi.disconnect();
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_SSID, AP_PWD);
        IPAddress accessPtIp = WiFi.softAPIP();
   
        setupWww();
        startWww();

        log_d("AccessPoint: IP '%s', KEY '%s' ", accessPtIp.toString().c_str(), AP_PWD);
        displayMessage(0, "____ Wifi setup ____");
        char msgBuf[21];
        snprintf(msgBuf, 21, "SSID: %s", AP_SSID);
        displayMessage(1, msgBuf);
        displayMessage(2, AP_PWD);
        
        snprintf(scrollingStreamTitleBuffer, MAXLEN_SCROLL_STREAMTITLE_BUFFER / 2, "http://%s", accessPtIp.toString().c_str());
        if (strlen(scrollingStreamTitleBuffer) > DISPLAY_COLS) {
            displayScrollingMsgStart(3, timerScrollMsgTick);
            timer.enable(timerScrollMsgTick);
        } else {
            displayMessage(3, scrollingStreamTitleBuffer);
        }
      
    } 
    
    if ((lastRunMode == RUN_MODE_RADIO) || (lastRunMode == RUN_MODE_STANDBY)) 
    {
        if (loadStations(stationsFilename) > 0) {
            log_d("Loaded %d stations", getStationInfoSize());
        } else {
            displayClear();
            stationInfo_t defStation = setStationInfoToDefault();
            displayMessage(0,"______Warning_______");
            displayMessage(1,"Couldn't load preset");
            displayMessage(2,"Upload station.json");
            displayMessage(3,"in config mode.");
            log_d("No preset stations loaded. Using one default station %s (%s)", defStation.name, defStation.url);
            delay(10000);
            displayClear();
        }

        if (writePreferencesOnSetup) {
            preferences.putString(PKEY_SSID, ssid);
            preferences.putString(PKEY_WIFI_KEY, password);
            log_d("Wrote defaults to prefs");
            delay(500);
        }

        ssid = preferences.getString(PKEY_SSID, ssid);
        password = preferences.getString(PKEY_WIFI_KEY, password);

        if (ssid.isEmpty() ||ssid.isEmpty()) {
            log_d("Can't connect to wifi, no credentials found.");        
            displayMessage(0, "Cant connect to wifi");
            displayMessage(1, "no credentials found");
            delay(7000);
            restartInRunMode(RUN_MODE_START_MENU);
        } else {
            log_d("Connecting to wifi ");
            WiFi.disconnect();
            WiFi.mode(WIFI_STA);
            
            WiFi.setHostname(hostname.c_str()); //TODO: no effect?
            
            displayMessage(0, "Connecting to wifi");
            displayMessage(1, ssid.c_str());

            WiFi.begin(ssid.c_str(), password.c_str());
            uint8_t t = 0;
            while ((WiFi.status() != WL_CONNECTED) && (t < MAX_WIFI_CONNECT_TRIES)) {
                t++;
                log_d("Connecting to wifi attempt #%d", t);
                displayBar(2, t);
                delay(1500);
            }
        }
        if (WiFi.status() != WL_CONNECTED) {
            log_d("Could not connect to Wifi. Check Config. Restarting in 10s");
            displayMessage(0, "Couldn't connect to");
            displayMessage(2, "Check Wifi Config");
            restartInRunMode(RUN_MODE_START_MENU);
        }

        log_i("Wifi connected. IP %s, Signal-Strength (RRSI): %d", WiFi.localIP().toString().c_str(), WiFi.RSSI());
            
        displayMessage(0, "Wifi connected");

        configTzTime(tzBerlin, ntpServer, ntpServer, ntpServer);
        log_d("Time Configured: %s, %s", tzBerlin, ntpServer);
        printLocalTime();

        delay(2000);
        displayClearLine(0);
        displayClearLine(1);
        delay(500);
        
        timer.enable(timerDateTimeDisplayUpdate);

        audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT, -1);
        audio.setVolumeSteps(VOL_MAX_STEPS+1);
        audio.setVolume(setupRotVolume());
        audio.setConnectionTimeout(500, 2700);

        setupRotStation();

        displayWifiSignal(WiFi.RSSI());

        if (lastRunMode == RUN_MODE_RADIO) {
            displayVolume(audio.getVolume());
            audio.connecttohost(getStationInfo(currentFavorite).url);
            displayMessage(2, getStationInfo(currentFavorite).name);

        } else if (lastRunMode == RUN_MODE_STANDBY) {
            displayClear();
            
        } 

        runMode = lastRunMode;

        log_d("Start in runMode = %d", runMode);
    }
    log_v("<<<<<<<<<<<< setup finished >>>>>>>>>>>");
}

void loop(){
    if (runMode == RUN_MODE_STANDBY) {
    }
    if (runMode == RUN_MODE_RADIO) {
        audio.loop();
    }
    if (runMode == RUN_MODE_START_MENU) {
    }

    rotaryLoop();
    timer.run();
}

// optional callbacks from audio lib
// TODO: show important error info in display
void audio_info(const char *info){
    log_i("info        %s", info);
}
void audio_id3data(const char *info){  //id3 metadata
    log_i("id3data     %s", info);
}
void audio_eof_mp3(const char *info){  //end of file
    log_i("eof_mp3     %s", info);
}
void audio_showstation(const char *info){
    log_i("station     %s", info);
}
void audio_showstreamtitle(const char *info){
    strlcpy(scrollingStreamTitleBuffer, info, MAXLEN_SCROLL_STREAMTITLE_BUFFER / 2);
    log_i("currentStreamTitle '%s'  %d\n", scrollingStreamTitleBuffer, strlen(scrollingStreamTitleBuffer));

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
    log_i("bitrate     %s", info);
}
void audio_commercial(const char *info){  //duration in sec
    log_i("commercial  %s", info);
}
void audio_icyurl(const char *info){  //homepage
    log_i("icyurl      %s", info);
}
void audio_lasthost(const char *info){  //stream URL played
    log_i("lasthost    %s", info);
}
void audio_eof_speech(const char *info){
    log_i("eof_speech  %s", info);
}

void restartInRunMode(uint8_t newRunMode) {
    preferences.putInt(PKEY_LAST_RUN_MODE, newRunMode);
    log_i(">>> Restarting to newRunMode %d", newRunMode);
    displayClear();
    displayMessage(1, "    restarting...");
    delay(500);
    ESP.restart();
}

void setWifiConfig(const String newSsid, const String newPkey) {
    preferences.putString(PKEY_SSID, newSsid);
    preferences.putString(PKEY_WIFI_KEY, newPkey);
    log_d("Saved new Wifi config");
}