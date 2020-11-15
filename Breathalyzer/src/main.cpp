#include <Wire.h>
#include <Adafruit_I2CDevice.h>
#include <Adafruit_ADS1015.h>
#include "MyBLEServer.h"
#include "MyDisplay.h"

Adafruit_ADS1115 ads;

MyDisplay *display;
MyBLEServer *server;

#define BUTTON_PIN 15
#define BUZZER 5

#define SCL 27
#define SDA 26

//główna flaga zarządzająca przebiegiem programu, przełączana przerwaniem przycisku
int flag = 0;

//flagi zdarzeń przycisku
bool buttonIsHeld = false;
bool buttonWasHeld = false;
bool blockInput = false;

//zmienne odczytywanych wartości, zmienne kalibracyjne
int calibValue = 0;  //wartość kalibracyjna, x1
int measurement = 0; //wartość zwracana przez ADC
int alcValue = 0;    //wartość pomiaru w promilach * 1000

char *formattedValue; //wartość pomiaru w promilach przekształcona na tablicę znaków
int temp = 0;         //liczbowa zmienna tymczasowa

//zmienne potrzebne do sformatowania wartości promili
char buffer[5];
int ret = 0;

unsigned long lastDebounceTime = 0; //czas wywołania ostatniego przerwania
unsigned long startPressed = 0;     //czas przytrzymania przycisku
unsigned long startMeasuring = 0;   //czas wykonywania pomiaru

//parametry uruchomieniowe buzzera
int freq = 2000;
int resolution = 8;
const int ledChannel = 0;

//parametry czasowe
int heatTime = 60;              //czas nagrzewania, w sekundach
int measureTime = 5;            //czas pomiaru, w sekundach
int measurementDisplayTime = 5; //czas wyświetlania wyniku pomiaru, w sekundach

//współrzędne punktów, do aproksymacji
int ya1 = 0; //y1, nazwa inna bo tamta zarezerwowana

int x2 = 919;
float y2 = 0.13;

int x3 = 1302;
float y3 = 0.89;

//przerwanie przycisku, zabezpieczenie przez drganiami styków, obsługa zdarzeń: kliknięcie/przytrzymanie
void IRAM_ATTR buttonInterrupt()
{
  if (!blockInput)
  {
    if (digitalRead(BUTTON_PIN) == LOW)
    {
      if (startPressed == 0 && (millis() - lastDebounceTime) > 250)
      {
        startPressed = millis();
        buttonIsHeld = true;
        Serial.println("COUNTING");
      }
    }
    else
    {
      if (buttonWasHeld)
      {
        buttonWasHeld = false;
      }
      else if ((millis() - lastDebounceTime) > 100)
      {
        if (flag >= 2)
        {
          flag--;
        }
        else
        {
          flag++;
        }

        blockInput = true; //blokada przycisku
        Serial.print("FLAG SET TO ");
        Serial.println(flag);
      }

      startPressed = 0;
      buttonIsHeld = false;
    }
    lastDebounceTime = millis();
  }
}

void setup()
{
  Serial.begin(115200);

  Wire.begin(SDA, SCL);

  display = new MyDisplay();

  Serial.println("Starting BLE work!");
  server = new MyBLEServer(&display->subText, &flag);

  //konfiguracja buzzera
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  ledcSetup(ledChannel, freq, resolution);
  ledcAttachPin(BUZZER, ledChannel);

  //opóźnienie, proces nagrzewania czujnika
  for (int i = heatTime; i > 0; --i)
  {
    display->OLEDDisplay("HEATING", i);
    delay(1000);
  }

  //pobranie wartości kalibracyjnej
  calibValue = map(ads.readADC_SingleEnded(0), 0, 32768, 0, 4095);
  Serial.print("Calibration value: ");
  Serial.println(calibValue);

  //włączenie przerwania przycisku
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), buttonInterrupt, CHANGE);
}

void loop()
{
  if (flag == 0)
  {
    display->OLEDDisplay("READY", 0);
  }
  if (server->deviceConnected)
  {
    if (flag == 0)
    {
      blockInput = false;

      delay(200);
    }

    while (flag == 1)
    {
      if (startMeasuring == 0)
      {
        measurement = 0; //zerowanie wartości odczytu
        Serial.println("Started measuring");
        delay(1000);
        ledcWriteTone(ledChannel, 1000);
        startMeasuring = millis();
      }

      //odczyt wartości z ADC
      int temp = map(ads.readADC_SingleEnded(0), 0, 32768, 0, 4095);
      if (temp > measurement)
      {
        measurement = temp;
        Serial.println(measurement);
      }

      //aproksymacja
      if (measurement < 944)
      {
        alcValue = 1000 * ((((ya1 - y2) / (calibValue - x2)) * measurement) + (ya1 - ((ya1 - y2) / (calibValue - x2) * calibValue)));
      }
      else
      {
        alcValue = 1000 * ((((y2 - y3) / (x2 - x3)) * measurement) + (y2 - ((y2 - y3) / (x2 - x3)) * x2));
      }
      if (alcValue < 0)
      {
        alcValue = 0;
      }

      ret = snprintf(buffer, sizeof buffer, "%f", alcValue / 1000.0);
      display->OLEDDisplay(buffer, 0);

      //zdarzenie uruchamia się po ustalonym czasie od rozpoczęcia pomiaru
      if (startMeasuring + (measureTime * 1000) < millis())
      {
        ledcWriteTone(ledChannel, 0);

        for (int i = measurementDisplayTime * 2; i > 0; --i)
        {
          display->OLEDDisplay(buffer, ceil(i / 2));
          delay(250);

          display->OLEDDisplay("", ceil(i / 2));
          delay(250);
        }

        //końcowy druk pomiaru
        display->OLEDDisplay(buffer, 0);

        Serial.print("Zawartość alkoholu wynosi: ");
        Serial.println(alcValue);

        if (alcValue < 100)
        {
          server->writeToCharacteristic("UNLOCK");
          Serial.println("Sent UNLOCK");
        }

        flag++;

        //zerowanie czasu pomiaru
        startMeasuring = 0;

        //wyłączenie blokady przycisku
        blockInput = false;
      }
    }
  }
  else
  {
    blockInput = true;
    delay(500);
  }
}
