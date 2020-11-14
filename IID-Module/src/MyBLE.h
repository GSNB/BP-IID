#pragma once
#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

// The remote service we wish to connect to.
#define serviceUUID "1813"
// The characteristic we wish to read.
#define characteristicUUID "2A4D"

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        Serial.printf("%s \n", advertisedDevice.getName().c_str());
    }
};

class MyBLE
{
public:
    BLEScan *pBLEScan;
    BLEClient *pClient;
    BLEScanResults foundDevices;
    BLERemoteCharacteristic *pRemoteCharacteristic;
    std::string breathalyzerAddress;

    MyBLE()
    {
        BLEDevice::init("");
        pBLEScan = BLEDevice::getScan(); //create new scan
        pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
        pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
        pBLEScan->setInterval(100);
        pBLEScan->setWindow(99); // less or equal setInterval value
        pClient = BLEDevice::createClient();
    }

    bool connectToServer(std::string deviceAddress)
    {
        bool connectionStatus;

        Serial.print("Forming a connection to ");
        Serial.println(deviceAddress.c_str());

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
            Serial.println("Found our service");

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
};