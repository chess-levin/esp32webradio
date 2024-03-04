#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Update.h>
#include <SPIFFS.h>

#include "commons.h"
#include "display.h"
#include "www.h"

#define PARAM_SSID  "ssid"
#define PARAM_PKEY  "pkey"
#define PARAM_BIN   "bin"

#define CTYPE_HTML  "text/html"
#define CTYPE_PLAIN "text/plain"

#define U_PART U_SPIFFS

AsyncWebServer server(80);
size_t contentLen;

void printProgressCB(size_t prg, size_t sz) {
    uint8_t p = (prg*100)/contentLen;
    log_i("Progress: %d%%", p);
}

void setupWww() {

    Update.onProgress(printProgressCB);

    server.onNotFound([](AsyncWebServerRequest* request) {
        log_d("onNotFound");
        request->send(404, CTYPE_PLAIN, "Not found");
    });

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        log_d("GET /");
        request->send(SPIFFS, "/index.html");
    });

    server.on("/ota", HTTP_GET, [](AsyncWebServerRequest* request) {
        log_d("GET /ota");
        request->send(SPIFFS, "/ota.html");
    });


    // https://lastminuteengineers.com/esp32-ota-web-updater-arduino-ide/
    // https://github.com/me-no-dev/ESPAsyncWebServer
    // https://github.com/lbernstone/asyncUpdate/blob/master/AsyncUpdate.ino
    server.on("/ota", HTTP_POST, 
        [](AsyncWebServerRequest *request) {
            log_d("POST /ota 1");
            AsyncWebServerResponse *response = request->beginResponse(200, CTYPE_PLAIN, (Update.hasError()) ? "FAIL" : "OK");
            response->addHeader("Connection", "close");
            request->send(SPIFFS, "/restart.html");
        },
        [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data,size_t len, bool final) {
            log_d("POST /ota 2");
            
            if (!index) {
                log_d("Update");
                displayMessage(0, "___Upload Firmware__");

                contentLen = request->contentLength();
                log_d("filename: %s , size %d", filename.c_str(), contentLen);
                // if filename includes spiffs, update the spiffs partition
                int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;

                if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
                    Update.printError(Serial);
                }
            }

            if (Update.write(data, len) != len) {
                Update.printError(Serial);
            }

            if (final) {
                log_i("final");
                AsyncWebServerResponse *response = request->beginResponse(302, CTYPE_PLAIN, "Please wait while the device reboots");
                response->addHeader("Refresh", "20");  
                response->addHeader("Location", "/");
                request->send(response);
                if (!Update.end(true)){
                    Update.printError(Serial);
                } else {
                    log_d("Update completed");
                    Serial.flush();
                    
                    restartInRunMode(RUN_MODE_RADIO);
                }
            }
    });

    server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest* request) {
        log_d("GET /wifi");
        request->send(SPIFFS, "/wifi.html");
    });

    server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest* request) {
        log_d("POST /wifi");
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
            log_d("Errors: %s\n", message.c_str());
            message = "/?error=" + message;
            request->redirect(message);
        } else {
            log_d("Received ssid='%s', pkey='%s'\n", ssid.c_str(), pkey.c_str());
            setWifiConfig(ssid, pkey);
            request->send(SPIFFS, "/restart.html");
        }
    });

    server.on("/restart", HTTP_GET, [](AsyncWebServerRequest* request) {
        restartInRunMode(RUN_MODE_STANDBY);
    });

}

void startWww() {
    server.begin();
}

void stop_www() {
    server.end();
}