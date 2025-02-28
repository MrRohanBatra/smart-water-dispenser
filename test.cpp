#include<Arduino.h>


// Motor Pins (ESP32)
#define ENA 13   // PWM speed control
#define IN1 12   // Motor direction
#define IN2 14   // Motor direction

// Flow rate calculation: 750 mL in 14.6 sec → 19.47 ms per mL
// const float FLOW_RATE = 20000.0 / 900; // Time (ms) required per mL
const float FLOW_RATE = 11.03; // Time (ms) required per mL

// void analogWrite(int pin, int value) {
//     ledcWrite(0, value);  // Simulating analogWrite() using ledcWrite()
// }
void setup(){
    int arr[]={2,ENA,IN1,IN2};
    for(auto it:arr){
        pinMode(it,OUTPUT);
    }
    ledcSetup(0,5000,8);
    ledcAttachPin(ENA,0);
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    analogWrite(ENA, 0); // Stop motor

    Serial.begin(115200);
    Serial.println("Water Dispenser Ready!");
    delay(10000);
    pinMode(IN1,HIGH);
    pinMode(IN2,LOW);
    analogWrite(ENA,255);
    int dela=FLOW_RATE*60;
    delay(dela);
    pinMode(IN1,LOW);
    pinMode(IN2,LOW);
    analogWrite(ENA,0);
    
}

void loop(){
    
}
