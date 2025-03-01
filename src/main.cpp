#include <SinricPro.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SinricProSwitch.h>
#include <FS.h>
#include <SPIFFS.h>

#define en 13
#define in1 12
#define in2 14
#define ledPin 2
#define tpin 32

const float flowrate = 11.05 * 2.5;
bool deviceState = false;
bool dispensing = false;
unsigned long dispenseStartTime = 0;
int dispenseDuration = 0;

WebServer server(80);
SinricProSwitch &device = SinricPro["67befdd1c8ff9665569cc54f"];

void startDispense(int ml) {
    Serial.printf("Starting to dispense %d mL...\n", ml);
    dispensing = true;
    dispenseStartTime = millis();
    dispenseDuration = flowrate * ml;

    digitalWrite(in1, HIGH);
    digitalWrite(ledPin, HIGH);
    ledcWrite(0, 255);
}

void stopDispense() {
    Serial.println("Stopping dispense...");
    digitalWrite(in1, LOW);
    digitalWrite(ledPin, LOW);
    ledcWrite(0, 0);

    device.sendPowerStateEvent(false, "Dispensing complete");
    deviceState = false;
    dispensing = false;
}

bool onPowerState(const String &deviceId, bool &state) {
    Serial.printf("Power state changed: %s\n", state ? "ON" : "OFF");
    deviceState = state;
    device.sendPowerStateEvent(state, "Command received");

    if (state) {
        startDispense(200);
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
    if (ml <= 100) {
        ml += ml / 4;
    }

    startDispense(ml);
    server.send(200, "application/json", "{\"state\":true}");
}

void handleState() {
    server.send(200, "application/json", String("{\"state\":") + (deviceState ? "true" : "false") + "}");
}

void reconnectWiFi() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Reconnecting to WiFi...");
        WiFi.disconnect();
        WiFi.reconnect();
        delay(5000);
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi connection failed! Restarting ESP32...");
            ESP.restart();
        }
    }
}

void setup() {
    Serial.begin(115200);
    pinMode(27, OUTPUT);
    digitalWrite(27, LOW);

    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed! Restarting ESP32...");
        ESP.restart();
    }

    WiFi.begin("Rohan", "vikki08494");
    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000) {
        Serial.print(".");
        delay(500);
    }
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection failed! Restarting ESP32...");
        ESP.restart();
    }
    Serial.printf("\nWiFi connected at: %s\n", WiFi.localIP().toString().c_str());

    if (!MDNS.begin("esp32")) {
        Serial.println("MDNS initialization failed! Restarting ESP32...");
        ESP.restart();
    }

    server.on("/dispense", HTTP_GET, handleDispense);
    server.on("/state", HTTP_GET, handleState);
    server.begin();

    SinricPro.begin("964a1c01-01b6-4ecf-b76d-bbcc243efb7a", "41f135cb-1954-497c-90dc-d49356b7b239-0ea77ef7-0bfa-4567-abe5-39a5df6de03c");
    device.onPowerState(onPowerState);

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
    reconnectWiFi();

    if (dispensing && (millis() - dispenseStartTime >= dispenseDuration)) {
        stopDispense();
    }

    static unsigned long lastTouchCheck = 0;
    if (millis() - lastTouchCheck > 300) {
        lastTouchCheck = millis();
        if (touchRead(tpin) <= 30) {
            Serial.println("Touch detected, dispensing...");
            device.sendPowerStateEvent(true, "Touch triggered");
            startDispense(200);
        }
    }
}
