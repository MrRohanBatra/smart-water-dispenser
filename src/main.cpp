#include <SinricPro.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <SinricProSwitch.h>
#include <FS.h>
#include <SPIFFS.h>
#include <OTAUpdate.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "time.h"
#include <Fonts/FreeSans9pt7b.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(128, 64, &Wire, -1);
#define EN 13
#define IN1 12
#define IN2 14
#define LED_PIN 2
#define TOUCH_PIN 32

#define TOUCH_THRESHOLD 30

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 19800;
const int daylightOffset_sec = 0;
int dispenseml;
void setupTime()
{
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        Serial.println("Time synced from NTP!");
    }
    else
    {
        Serial.println("Failed to get time!");
    }
}

String getCurrentTime()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return "Err";
    }
    char timeStr[16];
    strftime(timeStr, sizeof(timeStr), "%H:%M", &timeinfo);
    return String(timeStr);
}
const float FLOWRATE = 27.608;
bool deviceState = false;
bool dispensing = false;
unsigned long dispenseStartTime = 0;
int dispenseDuration = 0;

WebServer server(80);
SinricProSwitch &device = SinricPro["67befdd1c8ff9665569cc54f"];
OTAUpdate ota("http://192.168.29.126:80/smart-water-dispenser");
void displayLog(const String &message)
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(message);
    display.display();
}
void displayLog(const String &message, int size)
{
    display.clearDisplay();
    display.setTextSize(size);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(message);
    display.display();
}
const char *weatherAPIKey = "cb6b65b6dc8c47cb9bf124135230111";
const char *city = "Delhi,India";

String getWeather()
{
    String url = "http://api.weatherapi.com/v1/current.json?key=" + String(weatherAPIKey) + "&q=" + String(city) + "&aqi=no";

    HTTPClient http;
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode != 200)
    {
        Serial.println("Failed to fetch weather!");
        http.end();
        return "N/A";
    }

    String payload = http.getString();
    http.end();

    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (error)
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return "N/A";
    }

    float current_temp_c = doc["current"]["temp_c"];
    String condition = doc["current"]["condition"]["text"].as<String>();

    String weatherString = String(current_temp_c, 1);
    weatherString += (char)248;
    weatherString += "C " + condition;

    return weatherString;
}

void startDispense(int ml)
{
    dispenseml = ml;
    delay(10);
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
    DynamicJsonDocument doc(2048);
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
        ml += ml / 4;
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
        server.send(200, "text/html", "error in openong file");
        return;
    }

    String data = f.readString();
    f.close();
    server.send(200, "text/html", data);
}























void updateDisplayUI()
{
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    // if (ota.updateAvailable())
    // {
    //     display.clearDisplay();
    //     display.print("Update availabe");
    // }
    
    
        display.setTextSize(1);

        display.setCursor(0, 5);
        display.print("Time: ");
        display.print(getCurrentTime());

        display.setCursor(0, 20);
        display.print("Weather: ");
        display.print(getWeather());

        display.setCursor(0, 35);
        display.print("Pump: ");
        if (dispensing)
        {
            display.print("ON ");
            display.print(dispenseml);
            display.print(" ml");
        }
        else
        {
            display.print("OFF");
        }

        display.setCursor(0, 50);
        display.print("WiFi: ");
        display.print(WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString().c_str() : "Disconnected");

        display.display();
    }

void setup()
{
    Wire.begin(16, 15);
    ota.setupdisplay(display);
    ota.setFirmwareVersion(5, 0, 0);
    Serial.begin(115200);
    pinMode(27, OUTPUT);
    digitalWrite(27, LOW);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
    {
        Serial.println("SSD1306 initialization failed!");
        ESP.restart();
    }

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

    server.onNotFound([]()
                      { Serial.println(server.uri()); });
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
    setupTime();
    updateDisplayUI();
}
unsigned long long int lastcheckedforupdate = millis();
static unsigned long lastUIUpdate = 0;
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
            Serial.print(touchRead(TOUCH_PIN));
            Serial.println("Touch detected, dispensing...");
            device.sendPowerStateEvent(true, "Touch triggered");
            startDispense(200);
        }
    }
    if (millis() - lastcheckedforupdate >= (1000 * 60 * 10))
    {
        ota.checkForUpdates();
        lastcheckedforupdate = millis();
    }
    if (millis() - lastUIUpdate > 100)
    {
        lastUIUpdate = millis();
        updateDisplayUI();
    }
}