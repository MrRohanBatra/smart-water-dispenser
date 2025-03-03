#include <SinricPro.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SinricProSwitch.h>
#include <FS.h>
#include <SPIFFS.h>
#include <OTAUpdate.h>

#define EN 13
#define IN1 12
#define IN2 14
#define LED_PIN 2
#define TOUCH_PIN 32
#define TOUCH_THRESHOLD 30

const float FLOWRATE = 27.608;
bool deviceState = false;
bool dispensing = false;
unsigned long dispenseStartTime = 0;
int dispenseDuration = 0;

WebServer server(80);
SinricProSwitch &device = SinricPro["67befdd1c8ff9665569cc54f"];
OTAUpdate ota("http://192.168.29.126:80/smart-water-dispenser");
void startDispense(int ml)
{
    Serial.printf("Starting to dispense %d mL...\n", ml);
    dispensing = true;
    dispenseStartTime = millis();
    dispenseDuration = FLOWRATE * ml;

    digitalWrite(IN1, HIGH);
    digitalWrite(LED_PIN, HIGH);
    ledcWrite(0, 255);
}

void stopDispense()
{
    Serial.println("Stopping dispense...");
    digitalWrite(IN1, LOW);
    digitalWrite(LED_PIN, LOW);
    ledcWrite(0, 0);

    device.sendPowerStateEvent(false, "Dispensing complete");
    deviceState = false;
    dispensing = false;
}

bool onPowerState(const String &deviceId, bool &state)
{
    Serial.printf("Power state changed: %s\n", state ? "ON" : "OFF");
    deviceState = state;
    device.sendPowerStateEvent(state, "Command received");

    if (state)
    {
        startDispense(200);
    }

    return true;
}

void handleDispense()
{
    if (server.method() != HTTP_POST)
    {
        server.send(405, "text/plain", "Method Not Allowed");
        return;
    }

    if (!server.hasArg("plain"))
    {
        server.send(400, "text/plain", "Missing JSON body");
        return;
    }

    String body = server.arg("plain");
    DynamicJsonDocument doc(2048);  // Keep DynamicJsonDocument as it was
    DeserializationError error = deserializeJson(doc, body);
    
    

    if (error)
    {
        server.send(400, "text/plain", "Invalid JSON");
        return;
    }

    int ml = doc["ml"];
    if (ml < 1 || ml > 1000)
    {
        server.send(400, "text/plain", "Invalid amount (1-1000 mL allowed)");
        return;
    }

    if (ml <= 100)
    {
        ml += ml / 4; // Adjust for small values
    }

    startDispense(ml);
    server.send(200, "application/json", "{\"state\":true}");
}

void handleState()
{
    server.send(200, "application/json", String("{\"state\":") + (deviceState ? "true" : "false") + "}");
}

void reconnectWiFi()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Reconnecting to WiFi...");
        WiFi.disconnect();
        WiFi.reconnect();
        delay(5000);
        if (WiFi.status() != WL_CONNECTED)
        {
            Serial.println("WiFi connection failed! Restarting ESP32...");
            ESP.restart();
        }
    }
}

void handleRoot()
{
    if (!SPIFFS.begin(true))
    {
        SPIFFS.format();
        Serial.println("SPIFFS initialization failed!");
        server.send(500, "text/plain", "SPIFFS Error");
        return;
    }

    File f = SPIFFS.open("/index.html", "r");
    if (!f)
    {
        // server.send(404, "text/plain", "File not found");
        server.send(200, "text/html", "error in openong file");
        return;
    }

    String data = f.readString();
    f.close();
    server.send(200, "text/html", data);
}

void setup()
{
    ota.setFirmwareVersion(5, 0, 0);
    Serial.begin(115200);
    pinMode(27, OUTPUT);
    digitalWrite(27, LOW);

    if (!SPIFFS.begin(true))
    {
        Serial.println("SPIFFS initialization failed! Restarting ESP32...");
        ESP.restart();
    }

    Serial.println("Connecting to WiFi...");
    WiFi.begin("Rohan", "vikki08494");

    unsigned long startAttemptTime = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 10000)
    {
        Serial.print(".");
        delay(500);
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi connection failed! Restarting ESP32...");
        ESP.restart();
    }
    Serial.printf("\nWiFi connected at: %s\n", WiFi.localIP().toString().c_str());
    ota.begin();

    if (!MDNS.begin("esp32"))
    {
        Serial.println("MDNS initialization failed! Restarting ESP32...");
        ESP.restart();
    }
    server.on("/", HTTP_GET, handleRoot);
    server.on("/dispense", HTTP_POST, handleDispense);
    server.on("/state", HTTP_GET, handleState);
    ota.setupManualOTA(server);
    server.begin();

    SinricPro.begin("964a1c01-01b6-4ecf-b76d-bbcc243efb7a", "41f135cb-1954-497c-90dc-d49356b7b239-0ea77ef7-0bfa-4567-abe5-39a5df6de03c");
    device.onPowerState(onPowerState);

    int pins[] = {EN, IN1, IN2, LED_PIN};
    for (int pin : pins)
    {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }

    ledcSetup(0, 1000, 8);
    ledcAttachPin(EN, 0);
    Serial.println("Setup complete.");
}
unsigned long long int lastcheckedforupdate = millis();
void loop()
{
    SinricPro.handle();
    server.handleClient();
    if (WiFi.status() != WL_CONNECTED)
    {
        reconnectWiFi();
    }

    if (dispensing && (millis() - dispenseStartTime >= dispenseDuration))
    {
        stopDispense();
    }

    static unsigned long lastTouchCheck = 0;
    if (millis() - lastTouchCheck > 300)
    {
        lastTouchCheck = millis();
        if (touchRead(TOUCH_PIN) <= TOUCH_THRESHOLD)
        {
            Serial.println("Touch detected, dispensing...");
            device.sendPowerStateEvent(true, "Touch triggered");
            startDispense(200);
        }
    }
    if (millis() - lastcheckedforupdate == (1000 * 60 * 10))
    {
        ota.checkForUpdates();
    }
}
