#pragma once

#include <Adafruit_NeoPixel.h>
#include <Arduino.h>

#include "modules/common/imodule.h"

#define LED_PIN 42
#define LED_COUNT 1

class LedClass : public ModuleOnce {
public:
    void loop_ui() override {}

    void loop() override {}

    void set_color(uint8_t r, uint8_t g, uint8_t b) {
        pixels.setPixelColor(0, r, g, b);
        pixels.show();
    }

    void set_brightness(uint8_t brightness) {
        led_brightness = brightness;
        pixels.setBrightness(led_brightness);
        pixels.show();
    }

    void rainbow_update(uint16_t hue_step = 256) {
        rainbow_hue = (uint16_t)(rainbow_hue + hue_step);

        const uint32_t c = pixels.gamma32(pixels.ColorHSV(rainbow_hue));
        for (uint16_t i = 0; i < pixels.numPixels(); ++i) {
            pixels.setPixelColor(i, c);
        }
        pixels.show();
    }

protected:
    void setup_impl() override {
        Serial.println("[Led] Setup");
        pixels.begin();
        pixels.setBrightness(led_brightness);
        pixels.clear();
        pixels.show();
    }

private:
    Adafruit_NeoPixel pixels{LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800};
    uint8_t led_brightness = 255;
    uint16_t rainbow_hue = 0;  // 0..65535 (Adafruit HSV)
};

extern LedClass Led;
