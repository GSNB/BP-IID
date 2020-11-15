#pragma once
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>
#include <Arduino.h>

#define SERVICE_UUID "1813"
#define CHARACTERISTIC_UUID "2A4D"

class myServerCallback : public BLEServerCallbacks
{
    public:
    std::string *subTextRef;
    bool *deviceConnectedRef;
    int *stateFlagRef;
    
    myServerCallback(std::string *subText, bool *deviceConnected, int *stateFlag)
    {
        subTextRef = subText;
        deviceConnectedRef = deviceConnected;
        stateFlagRef = stateFlag;
    }

    void onConnect(BLEServer *pServer, esp_ble_gatts_cb_param_t *param)
    {

        char remoteAddress[18];

        sprintf(
            remoteAddress,
            "%.2X:%.2X:%.2X:%.2X:%.2X:%.2X",
            param->connect.remote_bda[0],
            param->connect.remote_bda[1],
            param->connect.remote_bda[2],
            param->connect.remote_bda[3],
            param->connect.remote_bda[4],
            param->connect.remote_bda[5]);

        Serial.print("Device connected, MAC: ");
        Serial.println(remoteAddress);

        pServer->getAdvertising()->stop();
        pServer->getServiceByUUID(SERVICE_UUID)->getCharacteristic(CHARACTERISTIC_UUID)->notify();

        *subTextRef = "CONNECTED";
        *deviceConnectedRef = true;
    }

    void onDisconnect(BLEServer *pServer)
    {

        Serial.println("Device disconnected");

        pServer->getAdvertising()->start();

        *deviceConnectedRef = false;

        *subTextRef = "NOT CONNECTED";

        *stateFlagRef = 0;
    }
};

class MyBLEServer
{
public:
    BLEServer *pServer = NULL;
    BLECharacteristic *pCharacteristic = NULL;
    BLEAdvertising *pAdvertising = NULL;
    bool deviceConnected = false;

    MyBLEServer(std::string *subText, int *stateFlag)
    {
        BLEDevice::init("Breathalyzer");
        esp_ble_auth_req_t auth_req = ESP_LE_AUTH_NO_BOND;
        esp_ble_io_cap_t iocap = ESP_IO_CAP_NONE;
        uint8_t key_size = 16;
        uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
        uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
        esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
        esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
        esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
        esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
        esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));

        pServer = BLEDevice::createServer();
        pServer->setCallbacks(new myServerCallback(subText, &deviceConnected, stateFlag));
        BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID));
        pCharacteristic = pService->createCharacteristic(
            BLEUUID(CHARACTERISTIC_UUID),
            BLECharacteristic::PROPERTY_READ |
                BLECharacteristic::PROPERTY_WRITE |
                BLECharacteristic::PROPERTY_NOTIFY |
                BLECharacteristic::PROPERTY_INDICATE);

        pCharacteristic->addDescriptor(new BLE2902());
        pCharacteristic->setValue("BLOCK");
        Serial.println("Set to BLOCK");
        pService->start();

        pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
        pAdvertising->setMinPreferred(0x12);
        pServer->startAdvertising();
    }
    void writeToCharacteristic(std::string message)
    {
        pCharacteristic->setValue(message.c_str());
        pCharacteristic->notify();
    }
};