#include "MyBLEServer.h"
#include "MyDisplay.h"
#include "MyBuzzer.h"
#include "MySensor.h"

MyDisplay *display;
MyBLEServer *server;
MyBuzzer *buzzer;
MySensor *sensor;

#define BUTTON_PIN 15

#define BUZZER_FREQUENCY 2000
#define BUZZER_RESOLUTION 8
#define BUZZER_CHANNEL 0

//główna flaga zarządzająca przebiegiem programu, przełączana przerwaniem przycisku
int flag = 0;

//flagi zdarzeń przycisku
bool buttonIsHeld = false;
bool buttonWasHeld = false;
bool blockInput = false;

char *formattedValue; //wartość pomiaru w promilach przekształcona na tablicę znaków
int temp = 0;         //liczbowa zmienna tymczasowa

unsigned long lastDebounceTime = 0; //czas wywołania ostatniego przerwania
unsigned long startPressed = 0;     //czas przytrzymania przycisku
unsigned long startMeasuring = 0;   //czas wykonywania pomiaru

//parametry czasowe
#define heatTime 60              //czas nagrzewania, w sekundach
#define measureTime 5            //czas pomiaru, w sekundach
#define measurementDisplayTime 5 //czas wyświetlania wyniku pomiaru, w sekundach

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

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  display = new MyDisplay();

  Serial.println("Starting BLE work!");
  server = new MyBLEServer(&display->subText, &flag);

  buzzer = new MyBuzzer(BUZZER_FREQUENCY, BUZZER_RESOLUTION);

  sensor = new MySensor();

  //opóźnienie, proces nagrzewania czujnika
  for (int i = heatTime; i > 0; --i)
  {
    display->OLEDDisplay("HEATING", i);
    delay(1000);
  }

  //pobranie wartości kalibracyjnej
  sensor->calibrate();

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
        sensor->clearReadings();
        Serial.println("Started measuring");
        delay(1000);
        buzzer->start();
        startMeasuring = millis();
      }

      sensor->read();

      display->OLEDDisplay(sensor->formatValue().c_str(), 0);

      //zdarzenie uruchamia się po ustalonym czasie od rozpoczęcia pomiaru
      if (startMeasuring + (measureTime * 1000) < millis())
      {
        buzzer->stop();

        for (int i = measurementDisplayTime * 2; i > 0; --i)
        {
          display->OLEDDisplay(sensor->formatValue().c_str(), ceil(i / 2));
          delay(250);

          display->OLEDDisplay("", ceil(i / 2));
          delay(250);
        }

        //końcowy druk pomiaru
        display->OLEDDisplay(sensor->formatValue().c_str(), 0);

        Serial.print("Zawartość alkoholu wynosi: ");
        Serial.println(sensor->alcValue);

        if (sensor->alcValue < 100)
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
