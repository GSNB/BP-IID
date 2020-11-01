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
#include <string>

bool blockFlag = true;

int scanTime = 1; //In seconds
BLEScan *pBLEScan;
BLEClient *pClient;
BLEScanResults foundDevices;
BLERemoteCharacteristic *pRemoteCharacteristic;
std::string breathalyzerAddress;
std::string receivedValue;

// The remote service we wish to connect to.
#define serviceUUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
// The characteristic we wish to read.
#define characteristicUUID "1813"

#define MINIMUM_DEVICE_INFO_RESPONSE_SIZE 30
#define MAXIMUM_DEVICE_OBJECT_SIZE 72

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    Serial.printf("%s \n", advertisedDevice.getName().c_str());
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

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

bool connectToServer(std::string deviceAddress)
{
  bool connectionStatus;

  Serial.print("Forming a connection to ");
  Serial.println(deviceAddress.c_str());

  Serial.println(" - Created client");

  // Connect to the remote BLE Server.
  connectionStatus = pClient->connect(deviceAddress); // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  if (connectionStatus)
  {
    Serial.println("Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService *pRemoteService = pClient->getService(BLEUUID(serviceUUID));
    if (pRemoteService == nullptr)
    {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID);
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(BLEUUID(characteristicUUID));
    if (pRemoteCharacteristic == nullptr)
    {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(characteristicUUID);
      pClient->disconnect();
      return false;
    }
    return true;
  }
  else
  {
    Serial.println("Failed to connect");
    return false;
  }
}

bool saveToFile(const char *filename, const char *text)
{
  File f = SPIFFS.open(filename, "w");

  if (!f)
  {
    Serial.println("file open failed");
    return false;
  }
  else
  {
    //Write data to file
    Serial.println("Writing Data to File");
    Serial.println(text);
    f.print(text);
    f.close(); //Close file
    return true;
  }
}

std::string readFromFile(const char *filename)
{
  std::string text = "";
  File f = SPIFFS.open(filename, "r");

  if (!f)
  {
    Serial.println("file open failed");
  }
  else
  {
    Serial.println("Reading Data from File:");
    //Data from file
    text = f.readStringUntil('\n').c_str();
    Serial.println(text.c_str());
    f.close(); //Close file
    Serial.println("File Closed");
  }
  return text;
}

void setup()
{
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH);

  //FS
  if (!SPIFFS.begin(true))
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  //BT
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99); // less or equal setInterval value
  pClient = BLEDevice::createClient();

  WiFi.persistent(false);
  WiFi.setAutoReconnect(false);
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  WiFi.softAP(ssid, password);

  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());

  // Send a GET request to <IP>/get?message=<message>
  server.on("/api/getconfiguration", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Starting BLE scan");
    foundDevices = pBLEScan->start(scanTime, false);
    delay(scanTime * 1000);
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
      if (strlen(foundDevices.getDevice(i).getName().c_str()) != 0)
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
    }
    request->send(200, "application/json", cJSON_Print(json));

  end:
    cJSON_Delete(json);
    pBLEScan->clearResults();
  });

  // Send a POST request to <IP>/post with a form field message set to <message>
  server.on(
      "/api/savebtsettings", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    Serial.println("Save Bluetooth configuration event fired");
    std::string body = "";
    std::string param = "Address";
    int position = 0;

    for (size_t i = 0; i < len; i++)
    {
      body += data[i];
    }
    position = body.find(param);
    if(position != 0){
      breathalyzerAddress = body.substr(position + param.length() + 3, 17);
      saveToFile("/ble.conf", breathalyzerAddress.c_str());
      Serial.print("Got address: ");
      Serial.println(breathalyzerAddress.c_str());
    }
    Serial.println(body.c_str());
    Serial.println(param.c_str()); });

  server.on(
      "/api/savewifisettings", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    Serial.println("Save WiFi configuration event fired");
    std::string body = "";
    int paramCount = 2;
    std::string params[paramCount] = {"SSID", "Password"};
    std::string results[paramCount] = {"" , ""};
    int position = 0;
    int length = 0;
    int resultPos = 0;
    for (size_t i = 0; i < len; i++)
    {
      body += data[i];
    }
    for (size_t i = 0; i < paramCount; i++) {
      position = body.find(params[i]);
      if(position != 0){
        resultPos = position + params[i].length() + 3;
        length = body.find('"', resultPos) - resultPos;
        results[i] = body.substr(resultPos, length);
      }
      position = 0;
      length = 0;
      resultPos = 0;
      Serial.println(results[i].c_str());
    }
    
    Serial.println(body.c_str()); });

  breathalyzerAddress = readFromFile("/ble.conf").c_str();
  Serial.print("Loaded address from memory: ");
  Serial.println(breathalyzerAddress.c_str());
  /*
  events.onConnect([](AsyncEventSourceClient *client) {
    client->send("hello!", NULL, millis(), 1000);
  });
  */
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html");
  });

  server.serveStatic("/", SPIFFS, "/");

  server.begin();
  delay(5000);
}

void loop()
{
  if (blockFlag)
  {
    if (pClient->isConnected())
    {
      // Read the value of the characteristic.
      if (pRemoteCharacteristic->canRead())
      {
        Serial.print("Read characteristic value is: ");
        Serial.println(pRemoteCharacteristic->readValue().c_str());
        receivedValue = pRemoteCharacteristic->readValue().c_str();
        if (receivedValue == "UNLOCK")
        {
          blockFlag = false;
          digitalWrite(RELAY_PIN, LOW);
        }
      }
    }
    else if (breathalyzerAddress.length() == 17)
    {
      Serial.println("Trying to connect");
      connectToServer(breathalyzerAddress.c_str());
    }
  }
  delay(2000);
}