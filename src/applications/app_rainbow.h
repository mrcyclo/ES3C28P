#pragma once

#include <Arduino.h>

#include <string>

#include "common/helpers.h"
#include "common/iapplication.h"
#include "config.h"
#include "led/led.h"

class AppRainbowClass : public Application {
public:
    AppRainbowClass() : drawer_icon_utf8(fa(0xf0eb)) {}

    const char* get_drawer_icon_text() override { return drawer_icon_utf8.c_str(); }

    void drawer_icon_clicked() override {
        Serial.println("[AppRainbow] Drawer icon clicked");

        if (enabled) {
            Serial.println("[AppRainbow] Turning off LED rainbow");
            if (task_handle) vTaskDelete(task_handle);
            task_handle = nullptr;
            Led.set_color(0, 0, 0);
            enabled = false;
        } else {
            Serial.println("[AppRainbow] Turning on LED rainbow");
            xTaskCreatePinnedToCore(
                FREERTOS_TASK_CB(AppRainbowClass, led_rainbow_task),  // Task function
                "led_rainbow_task",                                   // Task name
                2000,                                                 // Stack size (bytes)
                this,                                                 // Parameters
                1,                                                    // Priority
                &task_handle,                                         // Task handle
                1                                                     // Core 1
            );
            enabled = true;
        }

        // Cập nhật màu icon ngay tại đây (event-driven), khỏi cần loop_ui chạy mỗi frame.
        update_drawer_icon_style();
    }

    void app_close() override {}

    void loop_ui() override {}

    void loop() override {}

private:
    std::string drawer_icon_utf8;

    bool enabled = false;
    TaskHandle_t task_handle = nullptr;

    void update_drawer_icon_style() {
        if (!drawer_icon) return;
        if (enabled)
            lv_obj_set_style_bg_color(drawer_icon, LV_COLOR_SUCCESS, LV_PART_MAIN);
        else
            lv_obj_remove_local_style_prop(drawer_icon, LV_STYLE_BG_COLOR, LV_PART_MAIN);
    }

    void led_rainbow_task() {
        while (true) {
            if (enabled) Led.rainbow_update(256);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
};

extern AppRainbowClass AppRainbow;
