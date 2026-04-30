#pragma once

#include <Arduino.h>
#include "config.h"
#include "common/imodule.h"

class FpsClass : public ModuleOnce
{
public:
    void loop_ui() override
    {
        const unsigned long start_time = millis();
        if (start_time < fps_time + 1000)
        {
            fps_count++;
        }
        else
        {
            fps = fps_count;
            fps_count = 1;
            fps_time = start_time;
        }
    }
    void loop() override {}
    unsigned long get_fps() { return fps; }

protected:
    void setup_impl() override {}

private:
    unsigned long fps = 0;
    unsigned long fps_count = 0;
    unsigned long fps_time = 0;
};

extern FpsClass Fps;
