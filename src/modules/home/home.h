#pragma once

#include <lvgl.h>
#include "config.h"
#include "micro_sd/micro_sd.h"
#include "common/helpers.h"
#include "led/led.h"
#include "common/imodule.h"
#include "msgbox/msgbox.h"
#include "app_rainbow.h"

class HomeClass : public ModuleOnce
{
public:
    void loop_ui() override
    {
        AppRainbow.loop_ui();

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

    void loop() override {}

    lv_obj_t *get_screen() { return screen; }

protected:
    void setup_impl() override
    {
        screen = lv_obj_create(NULL);
        lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);
        lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

        image_wallpaper = lv_image_create(screen);
        lv_obj_align(image_wallpaper, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_size(image_wallpaper, lv_pct(100), lv_pct(100));
        lv_obj_add_flag(image_wallpaper, LV_OBJ_FLAG_HIDDEN);

        auto button_menu = lv_btn_create(screen);
        lv_obj_set_width(button_menu, lv_pct(95));
        lv_obj_align(button_menu, LV_ALIGN_BOTTOM_MID, 0, -5);
        lv_obj_add_event_cb(button_menu, LV_OBJ_EVENT_CB(HomeClass, button_menu_clicked_cb), LV_EVENT_CLICKED, this);

        auto label_menu = lv_label_create(button_menu);
        lv_obj_align(label_menu, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(label_menu, "Applications");

        auto sample_button = lv_button_create(screen);
        lv_obj_align(sample_button, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_event_cb(sample_button, LV_OBJ_EVENT_CB(AppRainbowClass, drawer_icon_clicked), LV_EVENT_CLICKED, static_cast<void *>(&AppRainbow));
        AppRainbow.set_drawer_icon(sample_button);
        auto sample_label = lv_label_create(sample_button);
        lv_obj_align(sample_label, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(sample_label, AppRainbow.get_drawer_icon_text());
    }

private:
    lv_obj_t *screen = nullptr;
    lv_obj_t *image_wallpaper = nullptr;
    bool loaded = false;
    lv_image_dsc_t wallpaper_dsc{};
    uint16_t *wallpaper_pixels = nullptr;

    void button_menu_clicked_cb()
    {
        Serial.println("[Home] Menu button clicked");
        MsgBox.info("Applications drawer will be implemented soon!", nullptr, []() {});
    }
};

extern HomeClass Home;
