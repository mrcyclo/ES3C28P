#pragma once

#include <lvgl.h>
#include "config.h"
#include "micro_sd.h"
#include "helpers.h"
#include "led.h"

namespace
{
    void led_rainbow_task(void *parameter)
    {
        while (true)
        {
            Led.rainbow_update(256); // đổi số để nhanh/chậm
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}

class HomeClass
{
private:
    lv_obj_t *screen = nullptr;
    lv_obj_t *image_wallpaper = nullptr;
    bool loaded = false;
    lv_image_dsc_t wallpaper_dsc{};
    uint16_t *wallpaper_pixels = nullptr;
    lv_obj_t *button_led = nullptr;
    bool led_enabled = false;
    TaskHandle_t led_rainbow_handle = NULL;

    void home_led_clicked_cb(lv_event_t *e)
    {
        if (led_enabled)
        {
            vTaskDelete(led_rainbow_handle);
            Led.set_color(0, 0, 0);
            lv_obj_remove_local_style_prop(button_led, LV_STYLE_BG_COLOR, LV_PART_MAIN);
            led_enabled = false;
        }
        else
        {
            xTaskCreatePinnedToCore(
                led_rainbow_task,    // Task function
                "led_rainbow_task",  // Task name
                2000,                // Stack size (bytes)
                NULL,                // Parameters
                0,                   // Priority
                &led_rainbow_handle, // Task handle
                0                    // Core 0
            );
            lv_obj_set_style_bg_color(button_led, lv_color_hex(0xac3e31), LV_PART_MAIN);
            led_enabled = true;
        }
    }

public:
    void setup()
    {
        screen = lv_obj_create(NULL);
        lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);

        image_wallpaper = lv_image_create(screen);
        lv_obj_align(image_wallpaper, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_size(image_wallpaper, lv_pct(100), lv_pct(100));
        lv_obj_add_flag(image_wallpaper, LV_OBJ_FLAG_HIDDEN);

        auto drawer_box = lv_obj_create(screen);
        lv_obj_remove_flag(drawer_box, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_align(drawer_box, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_width(drawer_box, lv_pct(100));
        lv_obj_set_height(drawer_box, 50);
        lv_obj_set_layout(drawer_box, LV_LAYOUT_FLEX);
        lv_obj_set_flex_align(drawer_box, LV_FLEX_ALIGN_SPACE_AROUND, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(drawer_box, 5, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(drawer_box, LV_OPA_0, LV_PART_MAIN);
        lv_obj_set_style_border_opa(drawer_box, LV_OPA_0, LV_PART_MAIN);

        auto button_menu = lv_btn_create(drawer_box);
        auto label_menu = lv_label_create(button_menu);
        lv_label_set_text(label_menu, "Menu");
        lv_obj_align(label_menu, LV_ALIGN_CENTER, 0, 0);

        button_led = lv_btn_create(drawer_box);
        auto label_led = lv_label_create(button_led);
        lv_label_set_text(label_led, fa(0xf0eb).c_str());
        lv_obj_align(label_led, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_event_cb(button_led, LV_OBJ_EVENT_CB(HomeClass, home_led_clicked_cb), LV_EVENT_CLICKED, this);
    }

    void loop_ui()
    {
        if (loaded)
            return;

        if (!MicroSD.is_mounted())
        {
            lv_obj_add_flag(image_wallpaper, LV_OBJ_FLAG_HIDDEN);
            loaded = true;
            return;
        }

        bool ok = MicroSD.lv_read_bmp_dsc_rgb565("/.system/wallpaper.bmp", &wallpaper_dsc, &wallpaper_pixels);
        if (!ok)
        {
            lv_obj_add_flag(image_wallpaper, LV_OBJ_FLAG_HIDDEN);
            loaded = true;
            Serial.println("[Home] Failed to load wallpaper.bmp (expected 24-bit uncompressed BMP)");
            return;
        }

        lv_image_set_src(image_wallpaper, &wallpaper_dsc);
        lv_obj_remove_flag(image_wallpaper, LV_OBJ_FLAG_HIDDEN);
        loaded = true;
        Serial.println("[Home] Wallpaper loaded into RAM");
    }

    lv_obj_t *get_screen()
    {
        return screen;
    }
};

extern HomeClass Home;
