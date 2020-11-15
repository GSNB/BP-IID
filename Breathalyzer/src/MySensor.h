#include <Adafruit_I2CDevice.h>
#include <Adafruit_ADS1015.h>
#include <Wire.h>

#define SCL 27
#define SDA 26

class MySensor
{
public:
    Adafruit_ADS1115 ads;
    //zmienne odczytywanych wartości, zmienne kalibracyjne
    int calibValue;  //wartość kalibracyjna, x1
    int measurement; //wartość zwracana przez ADC
    int alcValue;    //wartość pomiaru w promilach * 1000

    //współrzędne punktów, do aproksymacji
    int y1 = 0;

    int x2 = 919;
    float y2 = 0.13;

    int x3 = 1302;
    float y3 = 0.89;

    MySensor()
    {
        calibValue = 0;
        measurement = 0;
        alcValue = 0;

        Wire.begin(SDA, SCL);
    }

    void read()
    {
        int temp = map(ads.readADC_SingleEnded(0), 0, 32768, 0, 4095);
        if (temp > measurement)
        {
            measurement = temp;
            Serial.println(measurement);
        }

        //aproksymacja
        if (measurement < 944)
        {
            alcValue = 1000 * ((((y1 - y2) / (calibValue - x2)) * measurement) + (y1 - ((y1 - y2) / (calibValue - x2) * calibValue)));
        }
        else
        {
            alcValue = 1000 * ((((y2 - y3) / (x2 - x3)) * measurement) + (y2 - ((y2 - y3) / (x2 - x3)) * x2));
        }
        if (alcValue < 0)
        {
            alcValue = 0;
        }
    }

    std::string formatValue()
    {
        char buffer[5];
        int ret = 0;

        ret = snprintf(buffer, sizeof buffer, "%f", alcValue / 1000.0);

        return buffer;
    }

    void calibrate()
    {
        calibValue = map(ads.readADC_SingleEnded(0), 0, 32768, 0, 4095);
        Serial.print("Calibration value: ");
        Serial.println(calibValue);
    }

    void clearReadings()
    {
        measurement = 0;
        alcValue = 0;
    }
};