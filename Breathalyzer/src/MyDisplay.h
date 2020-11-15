#pragma once
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Deklaracje dla sterownika SSD1306 (SPI)
#define OLED_MOSI 19
#define OLED_CLK 18
#define OLED_DC 22
#define OLED_CS 23
#define OLED_RESET 21

class MyDisplay
{
public:
    Adafruit_SSD1306 *display = NULL;
    //zmienna tymczasowa wykorzystywana do druku statusu połączenia na wyświetlacz
    std::string subText;

    MyDisplay()
    {
        display = new Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT,
                                       OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
        subText = "NOT CONNECTED";

        //konfiguracja wyświetlacza
        if (!display->begin(SSD1306_SWITCHCAPVCC))
        {
            Serial.println(F("SSD1306 allocation failed"));
            for (;;)
                ;
        }
        display->clearDisplay();
        display->display();
    }
    void OLED(int x, int y, std::string text, int font_size)
    {
        display->setCursor(x, y);
        display->setTextSize(font_size);
        display->setTextColor(SSD1306_WHITE);
        display->println(text.c_str());
    }

    //fukcja (przeciążona) do wyświetlania liczby o podstawie dziesiątkowej
    void OLED(int x, int y, int value_decimal, int font_size)
    {
        display->setCursor(x, y);
        display->setTextSize(font_size);
        display->setTextColor(SSD1306_WHITE);
        display->println(value_decimal, 10);
    }

    void OLEDDisplay(std::string main, int count)
    {
        display->clearDisplay();
        if (count > 0)
        {
            OLED(0, 0, count, 1);
        }
        OLED(0, SCREEN_HEIGHT - 8, subText.c_str(), 1);
        OLED((SCREEN_WIDTH - (strlen(main.c_str()) * 11)) / 2, (SCREEN_HEIGHT - 16) / 2, main, 2);
        display->display();
    }
};