#pragma once

#include <Adafruit_NeoPixel.h>

#define LED_PIN 42
#define LED_COUNT 1

class LedClass
{
private:
    Adafruit_NeoPixel pixels{LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800};

public:
    void setup()
    {
        Serial.println("[Led] Setup");
        pixels.begin();
        pixels.clear();
    }

    void set_color(uint8_t r, uint8_t g, uint8_t b)
    {
        pixels.setPixelColor(0, r, g, b);
        pixels.show();
    }
};

extern LedClass Led;
