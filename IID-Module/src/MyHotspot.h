#pragma once
#include <WiFi.h>
#include "memory.h"

class MyHotspot
{
    std::string password;

public:
    std::string ssid;
    MyHotspot(std::string pass, std::string name)
    {
        this->password = pass;
        this->ssid = name;
    }

    void start()
    {
        WiFi.persistent(false);
        WiFi.setAutoReconnect(false);
        WiFi.softAPdisconnect();
        WiFi.disconnect();
        WiFi.softAP(ssid.c_str(), password.c_str());

        Serial.print("IP Address: ");
        Serial.println(WiFi.softAPIP());
    }

    void setSettings(std::string pass, std::string name)
    {
        this->ssid = pass;
        this->ssid = name;
    }
    
    void getMemConfig()
    {
        std::string temp = "";
        temp = readFromFile("/ssid.conf").c_str();
        if (temp.length() > 0)
        {
            this->ssid = temp;
            Serial.print("Loaded SSID from memory: ");
            Serial.println(ssid.c_str());
        }

        this->password = readFromFile("/pass.conf").c_str();
        Serial.print("Loaded password from memory: ");
        Serial.println(password.c_str());
        return;
    }
};