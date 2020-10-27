#include <Arduino.h>
#include "BluetoothSerial.h"
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <AsyncTCP.h>
#include <FS.h>
#include "SPIFFS.h"

const char *ssid = "IID Module";
const char *password = "zaq1@WSX";

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

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
  Serial.begin(115200);
  delay(1000);
  SerialBT.begin("IID Module");
  Serial.println("The device started, now you can pair it with bluetooth!");

  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  WiFi.softAP(ssid, password);

  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hello, world");
  });

  // Send a GET request to <IP>/get?message=<message>
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    String message;
    if (request->hasParam(PARAM_MESSAGE))
    {
      message = request->getParam(PARAM_MESSAGE)->value();
    }
    else
    {
      message = "No message sent";
    }
    request->send(200, "text/plain", "Hello, GET: " + message);
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

  SPIFFS.begin();

  events.onConnect([](AsyncEventSourceClient *client) {
    client->send("hello!", NULL, millis(), 1000);
  });

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");

  server.begin();
}

void loop()
{
  if (Serial.available())
  {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available())
  {
    Serial.write(SerialBT.read());
  }
  delay(20);
}