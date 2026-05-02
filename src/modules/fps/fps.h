#pragma once

#include <Arduino.h>
#include <lvgl.h>

#include "modules/common/imodule.h"

class FpsClass : public ModuleOnce {
public:
    // Gọi từ `LV_EVENT_FLUSH_FINISH` khi `lv_display_flush_is_last(disp)` — một lần / frame thực tế lên TFT.
    void notify_frame_flushed() { fps_count++; }

    // Cập nhật bộ đếm theo giây (gọi từ `lvgl_task` mỗi vòng; không còn đếm “vòng lặp task”).
    void loop_ui() override {
        const unsigned long t = millis();
        if (t < fps_time + 1000) return;

        fps = fps_count;
        fps_count = 0;
        fps_time = t;
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
