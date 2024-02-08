#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>

#include "commons.h"
#include "display.h"
#include "www.h"
#include "html/index.h"
#include "html/restart.h"
#include "html/wifi.h"
#include "html/ota.h"

#define PARAM_SSID  "ssid"
#define PARAM_PKEY  "pkey"
#define PARAM_BIN   "bin"

#define CTYPE_HTML  "text/html"
#define CTYPE_PLAIN "text/plain"

AsyncWebServer server(80);
size_t contentLen;

void printProgressCB(size_t prg, size_t sz) {
    uint8_t p = (prg*100)/contentLen;
    Serial.printf("Progress: %d%%\n", p);
    displayBarSingle(2, p);
}

void setup_www() {

    Update.onProgress(printProgressCB);

    server.onNotFound([](AsyncWebServerRequest* request) {
        Serial.println("onNotFound");
        request->send(404, CTYPE_PLAIN, "Not found");
    });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        Serial.println("GET /");
        request->send(200, CTYPE_HTML, INDEX_HTML);
    });

    server.on("/ota", HTTP_GET, [](AsyncWebServerRequest* request) {
        Serial.println("GET /ota");
        request->send(200, CTYPE_HTML, OTA_HTML);
    });


    // https://lastminuteengineers.com/esp32-ota-web-updater-arduino-ide/
    // https://github.com/me-no-dev/ESPAsyncWebServer
    // https://github.com/lbernstone/asyncUpdate/blob/master/AsyncUpdate.ino
    server.on("/ota", HTTP_POST, 
        [](AsyncWebServerRequest *request) {
            AsyncWebServerResponse *response = request->beginResponse(200, CTYPE_PLAIN, (Update.hasError()) ? "FAIL" : "OK");
            response->addHeader("Connection", "close");
            request->send(200, CTYPE_HTML, RESTART_HTML);
        },
        [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data,size_t len, bool final) {
            if (!index) {
                Serial.println("Update");
                displayMessage(0, "___Upload Firmware__");
                contentLen = request->contentLength();
                Serial.printf("filename: %s\n", filename.c_str());
                Serial.printf("content length: %d\n", contentLen);
                // if filename includes spiffs, update the spiffs partition
                //int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                }
            }

            if (Update.write(data, len) != len) {
                Update.printError(Serial);
            }

            if (final) {
                AsyncWebServerResponse *response = request->beginResponse(302, CTYPE_PLAIN, "Please wait while the device reboots");
                response->addHeader("Refresh", "20");  
                response->addHeader("Location", "/");
                request->send(response);
                if (!Update.end(true)){
                    Update.printError(Serial);
                } else {
                    Serial.println("Update completed");
                    Serial.flush();
                    
                    restartInRunMode(RUN_MODE_RADIO);
                }
            }
    });

    server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest* request) {
        Serial.println("GET /wifi");
        request->send(200, CTYPE_HTML, WIFI_HTML);
    });

    server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest* request) {
        Serial.println("POST /wifi");
        String message = "";
        boolean err = false;
        String ssid;
        String pkey;

        if (request->hasParam(PARAM_SSID, true)) {
            AsyncWebParameter* p = request->getParam(PARAM_SSID, true);
            ssid = p->value();
            if (ssid.length() == 0) {
                err = true;
                message += "empty_ssid,";
            }
        } else {
            message =+ "no_ssid,";
            err = true;
        }

        if (request->hasParam(PARAM_PKEY, true)) {
            AsyncWebParameter* p = request->getParam(PARAM_PKEY, true);
            pkey = p->value();
            if (pkey.length() == 0) {
                err = true;
                message += "empty_pkey,";
            } 
        } else {
            message += "no_pkey,";
            err = true;
        }

        if (err) {
            Serial.printf("Errors: %s\n", message.c_str());
            message = "/?error=" + message;
            request->redirect(message);
        } else {
            Serial.printf("Received ssid='%s', pkey='%s'\n", ssid.c_str(), pkey.c_str());
            setWifiConfig(ssid, pkey);
            request->send(200, CTYPE_HTML, RESTART_HTML);
        }
    });

    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest* request) {
        restartInRunMode(RUN_MODE_STANDBY);
    });

}

void start_www() {
    server.begin();
}

void stop_www() {
    server.end();
}