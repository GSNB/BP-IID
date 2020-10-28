#include <Arduino.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <AsyncTCP.h>
#include <FS.h>
#include "SPIFFS.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <cJSON.h>

int scanTime = 5; //In seconds
BLEScan *pBLEScan;
BLEScanResults foundDevices;

#define MINIMUM_DEVICE_INFO_RESPONSE_SIZE 30
#define MAXIMUM_DEVICE_OBJECT_SIZE 72

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Serial.printf("A%s \n", advertisedDevice.getName().c_str());
  }
};

const char *ssid = "IID Module";
const char *password = "zaq1@WSX";

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define RELAY_PIN 2
#define LED_PIN 15

AsyncWebServer server(80);
AsyncEventSource events("/events");

const char *PARAM_MESSAGE = "message";

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

void setup()
{
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  //BT
  Serial.println("Starting BLE scan");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value

  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  WiFi.softAP(ssid, password);

  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Send a GET request to <IP>/get?message=<message>
  server.on("/api/getdeviceslist", HTTP_GET, [](AsyncWebServerRequest *request) {
    foundDevices = pBLEScan->start(scanTime, false);

    cJSON *devices = NULL;
    cJSON *device = NULL;
    cJSON *name = NULL;
    cJSON *address = NULL;

    cJSON *json = cJSON_CreateObject();
    if (json == NULL)
    {
      goto end;
    }

    devices = cJSON_CreateArray();
    if (devices == NULL)
    {
      goto end;
    }
    cJSON_AddItemToObject(json, "devices", devices);

    for (int i = 0; i < foundDevices.getCount(); i++)
    {

      device = cJSON_CreateObject();
      if (device == NULL)
      {
        goto end;
      }
      cJSON_AddItemToArray(devices, device);

      name = cJSON_CreateString(foundDevices.getDevice(i).getName().c_str());
      if (name == NULL)
      {
        goto end;
      }
      cJSON_AddItemToObject(device, "Name", name);

      address = cJSON_CreateString(foundDevices.getDevice(i).getAddress().toString().c_str());
      if (address == NULL)
      {
        goto end;
      }
      cJSON_AddItemToObject(device, "Address", address);
    }
    request->send(200, "application/json", cJSON_Print(json));
  end:
    cJSON_Delete(json);
  });

  // Send a POST request to <IP>/post with a form field message set to <message>
  server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request) {
    String message;
    if (request->hasParam(PARAM_MESSAGE, true))
    {
      message = request->getParam(PARAM_MESSAGE, true)->value();
    }
    else
    {
      message = "No message sent";
    }
    request->send(200, "text/plain", "Hello, POST: " + message);
  });

  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  events.onConnect([](AsyncEventSourceClient *client) {
    client->send("hello!", NULL, millis(), 1000);
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html");
  });

  server.serveStatic("/", SPIFFS, "/");

  server.begin();
}

void loop()
{
}