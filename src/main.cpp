#include <SinricPro.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SinricProSwitch.h>
#include <FS.h>
#include <SPIFFS.h>
#include "ESP32OTAHelper.h"
#include<SemVer.h>

#define FIRMWARE_VERSION "1.1.3"

#define en 13
#define in1 12
#define in2 14
#define ledPin 2 
#define tpin 32

float flowrate = 11.05*2.5;
bool deviceState = false;

WebServer server(80);
SinricProSwitch &device = SinricPro["67befdd1c8ff9665569cc54f"];

void dispenseWater(int ml) {
    Serial.printf("Dispensing %d mL...\n", ml);
    digitalWrite(in1, HIGH);
    digitalWrite(ledPin, HIGH);
    ledcWrite(0, 255);
    delay(flowrate * ml); 
    digitalWrite(in1, LOW);
    digitalWrite(ledPin, LOW);
    ledcWrite(0, 0);
    Serial.println("Dispensing complete.");
}

bool onPowerState(const String &deviceId, bool &state) {
    Serial.printf("Power state changed: %s\n", state ? "ON" : "OFF");
    deviceState = state;
    device.sendPowerStateEvent(state, "Command received");

    if (state) {
        dispenseWater(200);
        device.sendPowerStateEvent(false, "Dispensing complete");
        deviceState = false;
    }

    return true;
}

void handleDispense() {
    if (!server.hasArg("ml")) {
        server.send(400, "text/plain", "Missing 'ml' parameter");
        return;
    }

    int ml = server.arg("ml").toInt();
    if (ml < 1 || ml > 1000) {
        server.send(400, "text/plain", "Invalid amount (1-1000 mL allowed)");
        return;
    }
    if(ml<=100){
        ml+=ml/4;
    }
    dispenseWater(ml);
    device.sendPowerStateEvent(false, "Dispensing complete");
    deviceState = false;
    server.send(200, "application/json", "{\"state\":false}");
}

void handleState() {
    server.send(200, "application/json", String("{\"state\":") + (deviceState ? "true" : "false") + "}");
}
void handlefile(String filename) {
    File file = SPIFFS.open(("/" + filename).c_str(), "r");
    if (!file) {
        server.send(404, "text/plain", "File not found");
        return;
    }
    
    String contentType = "text/plain";
    if (filename.endsWith(".css")) contentType = "text/css";
    else if (filename.endsWith(".js")) contentType = "application/javascript";
    
    server.streamFile(file, contentType);
    file.close();
}
void handleRoot() {
    File file = SPIFFS.open("/index.html", "r");
    if (!file) {
        server.send(404, "text/plain", "File not found");
        return;
    }
    server.streamFile(file, "text/html");
    file.close();
}

bool handleOTAUpdate(const String& url, int major, int minor, int patch, bool forceUpdate) {
    Version currentVersion  = Version(FIRMWARE_VERSION);
    Version newVersion      = Version(String(major) + "." + String(minor) + "." + String(patch));
    bool updateAvailable    = newVersion > currentVersion;

    Serial.print("OTA URL: ");
    Serial.println(url.c_str());
    Serial.print("Current version: ");
    Serial.println(currentVersion.toString());
    Serial.print("New version: ");
    Serial.println(newVersion.toString());

    if (forceUpdate || updateAvailable) {
        if (updateAvailable) {
            Serial.println("Update available!");
        }

        String result = startOtaUpdate(url);
        if (!result.isEmpty()) {
            SinricPro.setResponseMessage(std::move(result));
            return false;
        } 
        return true;
    } else {
        Serial.println("Already up to date.");
        SinricPro.setResponseMessage("Current version is up to date.");
        return false;
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(27,OUTPUT);
    digitalWrite(27,LOW);
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed!");
    } else {
        Serial.println("SPIFFS initialized.");
    }

    WiFi.begin("Rohan", "vikki08494");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print("....");
        delay(100);
    }
    Serial.printf("\nWiFi connected at: %s\n", WiFi.localIP().toString().c_str());

    if (!MDNS.begin("esp32")) {
        Serial.println("MDNS initialization failed!");
    } else {
        Serial.println("MDNS started with hostname 'esp32'");
    }

    server.on("/", HTTP_GET, handleRoot);
    server.on("/style.css", HTTP_GET, []() { handlefile("style.css"); });
    server.on("/script.js", HTTP_GET, []() { handlefile("script.js"); });
    server.on("/dispense", HTTP_GET, handleDispense);
    server.on("/state", HTTP_GET, handleState);
    server.begin();

    SinricPro.begin("964a1c01-01b6-4ecf-b76d-bbcc243efb7a", "41f135cb-1954-497c-90dc-d49356b7b239-0ea77ef7-0bfa-4567-abe5-39a5df6de03c");
    device.onPowerState(onPowerState);
    SinricPro.onOTAUpdate(handleOTAUpdate);

    int pins[] = {en, in1, in2, ledPin};
    for (int pin : pins) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    ledcSetup(0, 1000, 8);
    ledcAttachPin(en, 0);
    device.sendPowerStateEvent(false, "default off");
    Serial.println("Setup complete.");
}

void loop() {
    SinricPro.handle();
    server.handleClient();
    
    if (touchRead(tpin) <= 30) {
        Serial.println("Touch detected, dispensing...");
        device.sendPowerStateEvent(true, "Touch triggered");
        dispenseWater(200);
        device.sendPowerStateEvent(false, "Dispensing complete");
        delay(400);
    }
}