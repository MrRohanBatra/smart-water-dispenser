//////////////////////////////////////////////
//        RemoteXY include library          //
//////////////////////////////////////////////

#define REMOTEXY_MODE__WIFI_POINT

#include <WiFi.h>
#include <RemoteXY.h>

// RemoteXY WiFi credentials
#define REMOTEXY_WIFI_SSID "RemoteXY"
#define REMOTEXY_WIFI_PASSWORD "12345678"
#define REMOTEXY_SERVER_PORT 6377

// RemoteXY GUI configuration  
#pragma pack(push, 1)  
uint8_t RemoteXY_CONF[] = { 
  255,4,0,0,0,56,0,19,0,0,0,0,31,2,106,200,200,84,1,1,
  3,0,7,26,18,40,10,52,6,92,34,85,64,2,26,1,57,17,57,57,
  49,52,24,24,0,2,31,77,76,0,1,37,122,57,57,112,51,24,24,0,
  2,31,0 
};

// RemoteXY variables 
struct {
  int16_t volume;   // -32768 .. +32767
  uint8_t mlbased;  // 1 = button pressed, 0 = not pressed
  uint8_t bypass;   // 1 = button pressed, 0 = not pressed
  uint8_t connect_flag;  // 1 = connected, 0 = not connected
} RemoteXY;   
#pragma pack(pop)

/////////////////////////////////////////////
//        Motor Control Configuration       //
/////////////////////////////////////////////

// Motor Pins (ESP32)
#define ENA 13   // PWM speed control
#define IN1 12   // Motor direction
#define IN2 14   // Motor direction

// Flow rate calculation: 750 mL in 14.6 sec → 19.47 ms per mL
// const float FLOW_RATE = 20000.0 / 900; // Time (ms) required per mL
const float FLOW_RATE = 11.03; // Time (ms) required per mL

void analogWrite(int pin, int value) {
    ledcWrite(0, value);  // Simulating analogWrite() using ledcWrite()
}

void setup() {
    // Initialize RemoteXY
    RemoteXY_Init();

    // Set up Motor Pins
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(ENA, OUTPUT);

    // PWM Setup (ESP32 does not have analogWrite)
    ledcSetup(0, 5000, 8);
    ledcAttachPin(ENA, 0);

    // Ensure Motor is OFF Initially
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 0); // Stop motor

    Serial.begin(115200);
    Serial.println("Water Dispenser Ready!");
}

void loop() {
    RemoteXY_Handler(); // Handle RemoteXY events

    // Bypass Mode: Keep motor running at full speed
    if (RemoteXY.bypass) {
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        analogWrite(ENA, 255);
    } else {
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        analogWrite(ENA, 0);
    }

    // If mlbased button is pressed, dispense water based on RemoteXY volume input
    if (RemoteXY.mlbased) {
        int volume = RemoteXY.volume; // Get volume from RemoteXY
        Serial.println(volume);
        if (volume > 0) {
            if (volume < 100) {
                volume += volume / 4;
            }

            int duration = volume * FLOW_RATE; // Calculate motor run time

            Serial.print("Dispensing ");
            Serial.print(volume);
            Serial.println(" mL of water...");

            // Start Motor
            digitalWrite(IN1, HIGH);
            digitalWrite(IN2, LOW);
            analogWrite(ENA, 255); // Full speed

            delay(duration); // Run motor for calculated time

            // Stop Motor
            digitalWrite(IN1, LOW);
            digitalWrite(IN2, LOW);
            analogWrite(ENA, 0);

            Serial.println("Water Dispensed Successfully!");
        }
    }
}