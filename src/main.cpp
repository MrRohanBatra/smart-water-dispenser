#define REMOTEXY_MODE__WIFI_POINT

#include <WiFi.h>
#include <RemoteXY.h>
#define touchpin T9
#define REMOTEXY_WIFI_SSID "RemoteXY"
#define REMOTEXY_WIFI_PASSWORD "12345678"
#define REMOTEXY_SERVER_PORT 6377

#pragma pack(push, 1)  
uint8_t RemoteXY_CONF[] = { 
  255,4,0,0,0,56,0,19,0,0,0,0,31,2,106,200,200,84,1,1,
  3,0,7,26,18,40,10,52,6,92,34,85,64,2,26,1,57,17,57,57,
  49,52,24,24,0,2,31,77,76,0,1,37,122,57,57,112,51,24,24,0,
  2,31,0 
};

struct {
  int16_t volume;
  uint8_t mlbased;
  uint8_t bypass;
  uint8_t connect_flag;
} RemoteXY;   
#pragma pack(pop)


#define ENA 13
#define IN1 12
#define IN2 14

const float FLOW_RATE = 11.05;

void analogWrite(int pin, int value) {
    ledcWrite(0, value);
}

void setup() {
    RemoteXY_Init();

    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(ENA, OUTPUT);
    pinMode(2,OUTPUT);
    ledcSetup(0, 5000, 8);
    ledcAttachPin(ENA, 0);

    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 0);

    Serial.begin(115200);
    Serial.println("Water Dispenser Ready!");
}

void loop() {
    RemoteXY_Handler();

    if (RemoteXY.bypass) {
        digitalWrite(IN1, HIGH);
        digitalWrite(IN2, LOW);
        analogWrite(ENA, 255);
    } else {
        digitalWrite(IN1, LOW);
        digitalWrite(IN2, LOW);
        analogWrite(ENA, 0);
    }

    if (RemoteXY.mlbased) {
        int volume = RemoteXY.volume;
        Serial.println(volume);
        if (volume > 0) {
            if (volume < 100) {
                volume += volume / 4;
            }

            int duration = volume * FLOW_RATE;

            Serial.print("Dispensing ");
            Serial.print(volume);
            Serial.println(" mL of water...");

            digitalWrite(IN1, HIGH);
            digitalWrite(IN2, LOW);
            analogWrite(ENA, 255);

            delay(duration);

            digitalWrite(IN1, LOW);
            digitalWrite(IN2, LOW);
            analogWrite(ENA, 0);

            Serial.println("Water Dispensed Successfully!");
        }
    }
    if(touchRead(touchpin)<=35){
        digitalWrite(2,HIGH);
        int duration = 250 * FLOW_RATE;

            Serial.print("Dispensing ");
            Serial.print(250);
            Serial.println(" mL of water...");

            digitalWrite(IN1, HIGH);
            digitalWrite(IN2, LOW);
            analogWrite(ENA, 255);

            RemoteXY_delay(duration*2.5);

            digitalWrite(IN1, LOW);
            digitalWrite(IN2, LOW);
            analogWrite(ENA, 0);
           digitalWrite(2,LOW);
            Serial.println("Water Dispensed Successfully!");
        }
        
    }