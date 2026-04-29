#pragma once

#include <Arduino.h>
#include "config.h"

class FpsClass
{
private:
    unsigned long fps = 0;
    unsigned long fps_count = 0;
    unsigned long fps_time = 0;

public:
    void loop_ui()
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

    unsigned long get_fps()
    {
        return fps;
    }
};

extern FpsClass Fps;
