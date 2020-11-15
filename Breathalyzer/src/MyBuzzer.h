#include <Wire.h>

#define BUZZER 5

class MyBuzzer
{
public:
    //parametry uruchomieniowe buzzera
    int frequency;
    int resolution;
    const int channel = 0;

    MyBuzzer(int frequency, int resolution) : frequency(frequency), resolution(resolution)
    {
        ledcSetup(channel, frequency, resolution);
        ledcAttachPin(BUZZER, channel);
        ledcWriteTone(channel, 0);
    }

    void start()
    {
        ledcWriteTone(channel, 1000);
    }
    void stop()
    {
        ledcWriteTone(channel, 0);
    }
};