#include <Arduino.h>
#include "memory.h"
#include "MyBLE.h"
#include "MyHotspot.h"
#include "MyWebServer.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include <AsyncTCP.h>

#define RELAY_PIN 2
#define LED_PIN 15

#define defaultSSID "IID Module"
#define defaultPassword "zaq1@WSX"

std::string breathalyzerAddress;
std::string receivedValue;
MyBLE *bluetooth;
MyHotspot *hotspot;
MyWebServer *web;

bool blockFlag = true;
extern bool notifed;

bool blockConnection = false;

int scanTime = 1; //In seconds

void setup()
{
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  Serial.println("Start SPIFFS");
  startSPIFFS();

  breathalyzerAddress = readFromFile("/ble.conf").c_str();
  Serial.print("Loaded address from memory: ");
  Serial.println(breathalyzerAddress.c_str());

  Serial.println("Creating BLE Object");
  bluetooth = new MyBLE;

  Serial.println("Creating Hotspot Object");
  hotspot = new MyHotspot(defaultPassword, defaultSSID);
  hotspot->getMemConfig();
  hotspot->start();

  Serial.println("Creating WebServer Object");
  web = new MyWebServer();
  web->setupBTEndpoint(breathalyzerAddress, bluetooth);
  web->setupInfoEndpoint(bluetooth, hotspot, breathalyzerAddress, blockConnection, scanTime);
  web->setupWebpageEndpoint();
  web->setupWIFIEndpoint();
  web->start();
}

void loop()
{
  if (blockFlag)
  {
    if (!bluetooth->pClient->isConnected() && breathalyzerAddress.length() == 17 && !blockConnection)
    {
      Serial.println("Trying to connect");
      bluetooth->connectToServer(breathalyzerAddress.c_str());
    }
    if (bluetooth->pClient->isConnected() && notified)
    {
      notified = false;
      Serial.println("Got notification!");
      Serial.print("Read characteristic value is: ");
      Serial.println(bluetooth->pRemoteCharacteristic->readValue().c_str());
      Serial.println(bluetooth->pRemoteCharacteristic->toString().c_str());
      receivedValue = bluetooth->pRemoteCharacteristic->readValue().c_str();
      if (receivedValue == "UNLOCK")
      {
        blockFlag = false;
        digitalWrite(RELAY_PIN, LOW);
      }
    }
  }
  else
  {
    Serial.println("Going to sleep...");
    Serial.println("And I think it's gonna be a long, long time");
    esp_deep_sleep_start();
  }
}