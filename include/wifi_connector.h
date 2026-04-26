#pragma once

#include <string>

#include <lvgl.h>
#include <WiFi.h>
#include <Preferences.h>

static Preferences preferences;
static bool is_setup_completed = false;
static lv_obj_t *screen = nullptr;
static bool is_screen_shown = false;
static lv_obj_t *keyboard = nullptr;
static lv_obj_t *input_password = nullptr;
static lv_obj_t *dropdown_ssid = nullptr;

static void textarea_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target_obj(e);
    lv_obj_t *kb = (lv_obj_t *)lv_event_get_user_data(e);
    if (code == LV_EVENT_FOCUSED)
    {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }

    if (code == LV_EVENT_DEFOCUSED)
    {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

struct WifiConnector
{
    static void setup()
    {
        if (is_setup_completed)
            return;

        is_setup_completed = true;

        screen = lv_obj_create(NULL);
        lv_obj_set_style_pad_all(screen, 10, LV_PART_MAIN);

        keyboard = lv_keyboard_create(lv_layer_top());
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);

        auto label_ssid = lv_label_create(screen);
        lv_label_set_text(label_ssid, "SSID:");

        dropdown_ssid = lv_dropdown_create(screen);
        lv_obj_align_to(dropdown_ssid, label_ssid, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
        lv_obj_set_width(dropdown_ssid, lv_pct(100));

        auto label_password = lv_label_create(screen);
        lv_obj_align_to(label_password, dropdown_ssid, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
        lv_label_set_text(label_password, "Password:");

        input_password = lv_textarea_create(screen);
        lv_obj_align_to(input_password, label_password, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
        lv_obj_set_width(input_password, lv_pct(100));
        lv_textarea_set_one_line(input_password, true);
        lv_obj_add_event_cb(input_password, textarea_event_cb, LV_EVENT_ALL, keyboard);

        auto button_connect = lv_button_create(screen);
        auto label_connect = lv_label_create(button_connect);
        lv_obj_set_align(label_connect, LV_ALIGN_CENTER);
        lv_label_set_text(label_connect, "Connect");
        lv_obj_align_to(button_connect, input_password, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    }

    static void show_screen()
    {
        if (is_screen_shown)
            return;

        is_screen_shown = true;

        std::string dropdown_options;

        WiFi.mode(WIFI_STA);
        auto n = WiFi.scanNetworks();
        for (int i = 0; i < n; ++i)
        {
            if (i > 0)
            {
                dropdown_options += '\n';
            }

            dropdown_options += WiFi.SSID(i).c_str();
        }
        WiFi.scanDelete();

        lv_dropdown_set_options(dropdown_ssid, dropdown_options.c_str());

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
