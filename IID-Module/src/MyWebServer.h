#pragma once
#include <Arduino.h>
#include "memory.h"
#include "MyBLE.h"
#include "Myhotspot.h"
#include "ESPAsyncWebServer.h"
#include <AsyncTCP.h>
#include <cJSON.h>
#include <FS.h>
#include "SPIFFS.h"

void notFound(AsyncWebServerRequest *request)
{
    request->send(404, "text/plain", "Not found");
}

class MyWebServer
{
public:
    AsyncWebServer *server;
    AsyncEventSource *events;

    MyWebServer()
    {
        server = new AsyncWebServer(80);
        events = new AsyncEventSource("/events");
    }

    void setupBTEndpoint(std::string &breathalyzerAddress, MyBLE *bluetooth)
    {
        server->on(
            "/api/savebtsettings", HTTP_POST, [&breathalyzerAddress, bluetooth](AsyncWebServerRequest *request) {
                bluetooth->pClient->disconnect();
                Serial.println("Save Bluetooth configuration event fired");
                std::string addresstmp;

                if (request->hasParam("Address", true))
                {
                    addresstmp = request->getParam("Address", true)->value().c_str();

                    Serial.print("Received BT device address: ");
                    Serial.println(addresstmp.c_str());
                    Serial.println(breathalyzerAddress.c_str());
                    breathalyzerAddress = addresstmp;
                    Serial.println(breathalyzerAddress.c_str());
                    saveToFile("/ble.conf", addresstmp.c_str());
                }
                request->send(200, "text/plain", "BLE Device saved");
            });
    }

    void setupWIFIEndpoint()
    {
        server->on(
            "/api/savewifisettings", HTTP_POST, [](AsyncWebServerRequest *request) {
                Serial.println("Save WiFi configuration event fired");
                String ssidtmp;
                String passwordtmp;

                if (request->hasParam("SSID", true) && request->hasParam("Password", true))
                {
                    ssidtmp = request->getParam("SSID", true)->value();
                    passwordtmp = request->getParam("Password", true)->value();

                    Serial.print("Received SSID: ");
                    Serial.println(ssidtmp);

                    Serial.print("And Password: ");
                    Serial.println(passwordtmp);

                    Serial.println("Saving hotspot configuration to memory");
                    saveToFile("/ssid.conf", ssidtmp.c_str());
                    saveToFile("/pass.conf", passwordtmp.c_str());

                    request->send(200, "text/plain", "WiFi configuration saved");

                    Serial.println("Rebooting...");

                    ESP.restart();
                }
            });
    }

    void setupInfoEndpoint(MyBLE *bluetooth, MyHotspot *hotspot, std::string &breathalyzerAddress, bool &flag, int time)
    {
        // Send a GET request to <IP>/get?message=<message>
        server->on("/api/getconfiguration", HTTP_GET, [bluetooth, hotspot, &breathalyzerAddress, &flag, time](AsyncWebServerRequest *request) {
            Serial.println("Starting BLE scan");
            flag = true;
            bluetooth->pClient->disconnect();
            bluetooth->foundDevices = bluetooth->pBLEScan->start(time, false);
            delay(time * 1000);

            cJSON *devices = NULL;
            cJSON *device = NULL;
            cJSON *name = NULL;
            cJSON *address = NULL;
            cJSON *wifi = NULL;
            cJSON *savedDevice = NULL;
            cJSON *ssidtmp = NULL;

            cJSON *json = cJSON_CreateObject();

            if (json == NULL)
            {
                goto end;
            }

            wifi = cJSON_CreateObject();
            if (wifi == NULL)
            {
                goto end;
            }
            cJSON_AddItemToObject(json, "wifi", wifi);
            ssidtmp = cJSON_CreateString(hotspot->ssid.c_str());
            if (ssidtmp == NULL)
            {
                goto end;
            }
            cJSON_AddItemToObject(wifi, "SSID", ssidtmp);

            devices = cJSON_CreateArray();
            if (devices == NULL)
            {
                goto end;
            }
            cJSON_AddItemToObject(json, "devices", devices);

            for (int i = 0; i < bluetooth->foundDevices.getCount(); i++)
            {
                if (strlen(bluetooth->foundDevices.getDevice(i).getName().c_str()) != 0)
                {
                    device = cJSON_CreateObject();
                    if (device == NULL)
                    {
                        goto end;
                    }
                    if (bluetooth->foundDevices.getDevice(i).getAddress().toString() == breathalyzerAddress)
                    {
                        savedDevice = cJSON_CreateObject();
                        if (savedDevice == NULL)
                        {
                            goto end;
                        }
                        cJSON_AddItemToObject(json, "savedDevice", savedDevice);

                        name = cJSON_CreateString(bluetooth->foundDevices.getDevice(i).getName().c_str());
                        if (name == NULL)
                        {
                            goto end;
                        }
                        cJSON_AddItemToObject(savedDevice, "Name", name);

                        address = cJSON_CreateString(bluetooth->foundDevices.getDevice(i).getAddress().toString().c_str());
                        if (address == NULL)
                        {
                            goto end;
                        }
                        cJSON_AddItemToObject(savedDevice, "Address", address);
                    }
                    else
                    {
                        cJSON_AddItemToArray(devices, device);
                        name = cJSON_CreateString(bluetooth->foundDevices.getDevice(i).getName().c_str());
                        if (name == NULL)
                        {
                            goto end;
                        }
                        cJSON_AddItemToObject(device, "Name", name);

                        address = cJSON_CreateString(bluetooth->foundDevices.getDevice(i).getAddress().toString().c_str());
                        if (address == NULL)
                        {
                            goto end;
                        }
                        cJSON_AddItemToObject(device, "Address", address);
                    }
                }
            }
            flag = false;
            request->send(200, "application/json", cJSON_Print(json));

        end:
            cJSON_Delete(json);
            bluetooth->pBLEScan->clearResults();
        });
    }

    void setupWebpageEndpoint()
    {
        server->on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/index.html");
        });
    }

    void start()
    {
        server->serveStatic("/", SPIFFS, "/");
        server->begin();
    }
};