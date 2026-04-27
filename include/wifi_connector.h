#pragma once

#include <esp_wifi.h>
#include <lvgl.h>
#include <string>
#include <WiFi.h>
#include "my_msgbox.h"
#include "helpers.h"
#include "config.h"

#define ENUM_STATE_READY_TO_CONNECT 0
#define ENUM_STATE_CONNECTING 1
#define ENUM_STATE_CONNECTED 2

class WifiConnectorClass
{
private:
    const ulong connect_timeout = 15000;
    ulong connect_time = 0;
    uint8_t state = ENUM_STATE_READY_TO_CONNECT;

    bool is_screen_shown = false;
    lv_obj_t *screen = nullptr;
    lv_obj_t *keyboard = nullptr;
    lv_obj_t *dropdown_ssid = nullptr;
    lv_obj_t *input_password = nullptr;
    lv_obj_t *button_connect = nullptr;
    lv_obj_t *screen_after_connected = nullptr;

    void button_connect_clicked_cb(lv_event_t *e)
    {
        lv_obj_add_state(button_connect, LV_STATE_DISABLED);
        lv_obj_add_state(dropdown_ssid, LV_STATE_DISABLED);
        lv_obj_add_state(input_password, LV_STATE_DISABLED);

        char ssid[64];
        ssid[0] = '\0';
        lv_dropdown_get_selected_str(dropdown_ssid, ssid, sizeof(ssid));

        auto passpharse = lv_textarea_get_text(input_password);

        WiFi.begin(ssid, passpharse);
        connect_time = millis();

        state = ENUM_STATE_CONNECTING;
    }

    /**
     * Có profile STA do stack Wi‑Fi lưu trong NVS (dùng cho `WiFi.begin()` không đối số).
     * Chỉ gọi sau khi đã `WiFi.mode(WIFI_STA)` (ví dụ sau `setup()`).
     */
    bool hasDriverStoredStaCredentials()
    {
        wifi_config_t cfg{};
        if (esp_wifi_get_config(WIFI_IF_STA, &cfg) != ESP_OK)
            return false;
        return cfg.sta.ssid[0] != '\0';
    }

    void show_screen()
    {
        if (is_screen_shown)
            return;

        is_screen_shown = true;

        std::string dropdown_options;
        WiFi.disconnect();
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

    // void show_wifi_status()
    // {
    //     switch (WiFi.status())
    //     {
    //     case WL_IDLE_STATUS:
    //         lv_label_set_text(label_ip, "IDLE_STATUS");
    //         break;

    //     case WL_NO_SSID_AVAIL:
    //         lv_label_set_text(label_ip, "NO_SSID_AVAIL");
    //         break;

    //     case WL_SCAN_COMPLETED:
    //         lv_label_set_text(label_ip, "SCAN_COMPLETED");
    //         break;

    //     case WL_CONNECTED:
    //         lv_label_set_text(label_ip, "CONNECTED");
    //         break;

    //     case WL_CONNECT_FAILED:
    //         lv_label_set_text(label_ip, "CONNECT_FAILED");
    //         break;

    //     case WL_CONNECTION_LOST:
    //         lv_label_set_text(label_ip, "CONNECTION_LOST");
    //         break;

    //     case WL_DISCONNECTED:
    //         lv_label_set_text(label_ip, "DISCONNECTED");
    //         break;

    //     default:
    //         lv_label_set_text_fmt(label_ip, "Status: %d", WiFi.status());
    //         break;
    //     }
    // }

public:
    void setup()
    {
        WiFi.mode(WIFI_STA);

        screen = lv_obj_create(NULL);
        lv_obj_set_style_pad_all(screen, 10, LV_PART_MAIN);
        lv_obj_set_style_pad_top(screen, STATUS_BAR_HEIGHT + 10, LV_PART_MAIN);

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

        button_connect = lv_button_create(screen);
        auto label_connect = lv_label_create(button_connect);
        lv_obj_set_align(label_connect, LV_ALIGN_CENTER);
        lv_label_set_text(label_connect, "Connect");
        lv_obj_align_to(button_connect, input_password, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
        lv_obj_add_event_cb(button_connect, LV_OBJ_EVENT_CB(WifiConnectorClass, button_connect_clicked_cb), LV_EVENT_CLICKED, this);
    }

    void loop()
    {
        if (!is_screen_shown)
        {
            if (state == ENUM_STATE_READY_TO_CONNECT)
            {
                if (hasDriverStoredStaCredentials())
                {
                    WiFi.begin();
                    connect_time = millis();
                    state = ENUM_STATE_CONNECTING;
                }
                else
                {
                    show_screen();
                    state = ENUM_STATE_READY_TO_CONNECT;
                }
            }

            if (state == ENUM_STATE_CONNECTING)
            {
                if (WiFi.status() == WL_CONNECTED)
                {
                    WiFi.setAutoReconnect(true);
                    state = ENUM_STATE_CONNECTED;

                    if (screen_after_connected)
                    {
                        lv_screen_load(screen_after_connected);
                        lv_obj_delete(screen);
                    }
                }
                else if (millis() - connect_time >= connect_timeout)
                {
                    WiFi.disconnect();
                    show_screen();
                    state = ENUM_STATE_READY_TO_CONNECT;
                }
                else
                {
                    // show_wifi_status();
                }
            }
        }
        else
        {
            if (state == ENUM_STATE_CONNECTING)
            {
                if (WiFi.status() == WL_CONNECTED)
                {
                    WiFi.setAutoReconnect(true);
                    my_info_msgbox("Wifi connect success!", nullptr, []() {});
                    state = ENUM_STATE_CONNECTED;

                    if (screen_after_connected)
                    {
                        lv_screen_load(screen_after_connected);
                        lv_obj_delete(screen);
                    }
                }
                else if (millis() - connect_time >= connect_timeout)
                {
                    WiFi.disconnect();
                    my_error_msgbox("Wifi connect failed!", nullptr, [=]()
                                    {
                        lv_obj_remove_state(button_connect, LV_STATE_DISABLED);
                        lv_obj_remove_state(dropdown_ssid, LV_STATE_DISABLED);
                        lv_obj_remove_state(input_password, LV_STATE_DISABLED); });
                    state = ENUM_STATE_READY_TO_CONNECT;
                }
                else
                {
                    // show_wifi_status();
                }
            }
        }
    }

    bool is_connected()
    {
        return WiFi.status() == WL_CONNECTED;
    }

    void set_screen_after_connected(lv_obj_t *screen)
    {
        screen_after_connected = screen;
    }
};

extern WifiConnectorClass WifiConnector;
