#pragma once

#include <Arduino.h>
#include "common/iapplication.h"
#include "led/led.h"
#include "common/helpers.h"

class AppRainbowClass : public Application
{
public:
    const char *get_drawer_icon_text() override
    {
        return fa(0xf0eb).c_str();
    }

    void drawer_icon_clicked() override
    {
        Serial.println("[AppRainbow] Drawer icon clicked");

        if (enabled)
        {
            Serial.println("[AppRainbow] Turning off LED rainbow");
            if (task_handle)
                vTaskDelete(task_handle);
            task_handle = nullptr;
            Led.set_color(0, 0, 0);
            enabled = false;
        }
        else
        {
            Serial.println("[AppRainbow] Turning on LED rainbow");
            xTaskCreatePinnedToCore(
                FREERTOS_TASK_CB(AppRainbowClass, led_rainbow_task), // Task function
                "led_rainbow_task",                                  // Task name
                2000,                                                // Stack size (bytes)
                this,                                                // Parameters
                1,                                                   // Priority
                &task_handle,                                        // Task handle
                1                                                    // Core 1
            );
            enabled = true;
        }
    }

    void app_close() override {}

    void loop_ui() override
    {
        if (enabled)
        {
            lv_obj_set_style_bg_color(drawer_icon, lv_color_hex(0xac3e31), LV_PART_MAIN);
        }
        else
        {
            lv_obj_remove_local_style_prop(drawer_icon, LV_STYLE_BG_COLOR, LV_PART_MAIN);
        }
    }

    void loop() override {}

private:
    bool enabled = false;
    TaskHandle_t task_handle = nullptr;

    void led_rainbow_task()
    {
        while (true)
        {
            if (enabled)
                Led.rainbow_update(256);
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
};

extern AppRainbowClass AppRainbow;
