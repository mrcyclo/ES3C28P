#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 42
#define LED_COUNT 1

class LedClass
{
private:
    Adafruit_NeoPixel pixels{LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800};
    uint8_t brightness = 255;
    uint16_t rainbow_hue = 0; // 0..65535 (Adafruit HSV)

public:
    void setup()
    {
        Serial.println("[Led] Setup");
        pixels.begin();
        pixels.setBrightness(brightness);
        pixels.clear();
        pixels.show();
    }

    void set_color(uint8_t r, uint8_t g, uint8_t b)
    {
        pixels.setPixelColor(0, r, g, b);
        pixels.show();
    }

    /** Set brightness (0..255) used by rainbow effect */
    void set_brightness(uint8_t brightness)
    {
        brightness = brightness;
        pixels.setBrightness(brightness);
        pixels.show();
    }

    /** Advance the rainbow by `hue_step` and render.
     *  Call this repeatedly in your task loop.
     */
    void rainbow_update(uint16_t hue_step = 256)
    {
        rainbow_hue = (uint16_t)(rainbow_hue + hue_step);

        // ColorHSV expects hue in [0..65535]. gamma32 improves perceived smoothness.
        const uint32_t c = pixels.gamma32(pixels.ColorHSV(rainbow_hue));
        for (uint16_t i = 0; i < pixels.numPixels(); ++i)
        {
            pixels.setPixelColor(i, c);
        }
        pixels.show();
    }
};

extern LedClass Led;

