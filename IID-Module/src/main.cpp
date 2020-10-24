#include <Arduino.h>

#define RELAY_PIN 2
#define LED_PIN 15

void setup() {
  pinMode(RELAY_PIN, OUTPUT);   
  digitalWrite(RELAY_PIN, LOW);
  //digitalWrite(LED_PIN, LOW);
}

void loop() {
  // put your main code here, to run repeatedly:
}