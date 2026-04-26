#pragma once

#include <lvgl.h>
#include <WiFi.h>
#include <Preferences.h>

static Preferences preferences;
static bool is_setup_completed = false;
static lv_obj_t *screen = nullptr;
static bool is_screen_shown = false;

struct WifiConnector
{
    static void setup()
    {
        if (is_setup_completed)
            return;

        is_setup_completed = true;

        screen = lv_obj_create(NULL);
        lv_obj_set_style_pad_all(screen, 10, LV_PART_MAIN);

        auto label1 = lv_label_create(screen);
        lv_label_set_text(label1, "SSID:");

        auto dropdown_ssid = lv_dropdown_create(screen);
        lv_obj_align_to(dropdown_ssid, label1, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
        lv_obj_set_width(dropdown_ssid, lv_pct(100));
        WiFi.mode(WIFI_STA);
        auto n = WiFi.scanNetworks();
        for (int i = 0; i < n; ++i)
        {
            lv_dropdown_add_option(dropdown_ssid, WiFi.SSID(i).c_str(), i);
        }
        WiFi.scanDelete();

        auto label2 = lv_label_create(screen);
        lv_obj_align_to(label2, dropdown_ssid, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
        lv_label_set_text(label2, "Password:");

        auto input_password = lv_textarea_create(screen);
        lv_obj_align_to(input_password, label2, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
        lv_obj_set_width(input_password, lv_pct(100));
        lv_textarea_set_one_line(input_password, true);
    }

    static void show_screen()
    {
        if (is_screen_shown)
            return;

        is_screen_shown = true;

        lv_screen_load(screen);
    }

    static bool canConnect()
    {
        preferences.begin("wifi", true);
        auto ssid = preferences.getString("ssid");
        auto passpharse = preferences.getString("passpharse");
        preferences.end();

        return ssid.length() > 0 && passpharse.length() > 0;
    }
};
