#include <WiFi.h>
#include <ESPAsyncWebServer.h>

#include "commons.h"
#include "display.h"
#include "www.h"
#include "html/index.h"
#include "html/restart.h"

#define PARAM_SSID "ssid"
#define PARAM_PKEY "pkey"

#define CTYPE_HTML  "text/html"
#define CTYPE_PLAIN "text/plain"


AsyncWebServer server(80);

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void setup_www() {

    server.onNotFound(notFound);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
        Serial.println("ESP32 Web Server: New request received:");
        Serial.println("GET /");
        request->send(200, CTYPE_HTML, INDEX_HTML);
    });

    server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest* request) {
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