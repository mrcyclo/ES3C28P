#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <vector>
#include <WiFi.h>
#include <string>
#include "common/iapplication.h"
#include "common/helpers.h"
#include "config.h"
#include "keyboard/keyboard.h"
#include "home/home.h"
#include "msgbox/msgbox.h"

#define APP_WIFI_CONNECT_TIMEOUT_MS 15000U
#define APP_WIFI_SCAN_TASK_STACK 8192
#define APP_WIFI_SCAN_TASK_PRIORITY 1
#define APP_WIFI_SCAN_TASK_CORE 1

class AppWifiClass : public Application
{
public:
    const char *get_drawer_icon_text() override { return fa(0xf1eb).c_str(); }

    void drawer_icon_clicked() override
    {
        if (!screen)
        {
            screen = lv_obj_create(nullptr);
            lv_obj_set_style_pad_all(screen, 10, LV_PART_MAIN);
            lv_obj_set_style_pad_top(screen, STATUS_BAR_HEIGHT + 10, LV_PART_MAIN);
            lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

            panel_setup = lv_obj_create(screen);
            lv_obj_set_size(panel_setup, lv_pct(100), lv_pct(88));
            lv_obj_align(panel_setup, LV_ALIGN_TOP_MID, 0, 0);
            lv_obj_set_style_pad_all(panel_setup, 0, LV_PART_MAIN);
            lv_obj_set_style_border_width(panel_setup, 0, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(panel_setup, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_remove_flag(panel_setup, LV_OBJ_FLAG_SCROLLABLE);

            auto lbl_ssid = lv_label_create(panel_setup);
            lv_label_set_text(lbl_ssid, "SSID:");

            ssid_row = lv_obj_create(panel_setup);
            lv_obj_set_width(ssid_row, lv_pct(100));
            lv_obj_set_height(ssid_row, LV_SIZE_CONTENT);
            lv_obj_align_to(ssid_row, lbl_ssid, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
            lv_obj_set_layout(ssid_row, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(ssid_row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(ssid_row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_column(ssid_row, 6, LV_PART_MAIN);
            lv_obj_set_style_pad_all(ssid_row, 0, LV_PART_MAIN);
            lv_obj_set_style_border_width(ssid_row, 0, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(ssid_row, LV_OPA_TRANSP, LV_PART_MAIN);

            dropdown_ssid = lv_dropdown_create(ssid_row);
            lv_obj_set_flex_grow(dropdown_ssid, 1);
            lv_dropdown_set_options(dropdown_ssid, "");

            btn_scan_wifi = lv_button_create(ssid_row);
            lv_obj_set_size(btn_scan_wifi, 40, 40);

            auto lbl_scan = lv_label_create(btn_scan_wifi);
            lv_obj_align(lbl_scan, LV_ALIGN_CENTER, 0, 0);
            lv_label_set_text(lbl_scan, fa(0xf2f1).c_str());
            lv_obj_add_event_cb(btn_scan_wifi, LV_OBJ_EVENT_CB(AppWifiClass, on_scan_wifi_clicked), LV_EVENT_CLICKED, this);

            auto lbl_pw = lv_label_create(panel_setup);
            lv_obj_align_to(lbl_pw, ssid_row, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
            lv_label_set_text(lbl_pw, "Password:");

            input_password = lv_textarea_create(panel_setup);
            lv_obj_align_to(input_password, lbl_pw, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
            lv_obj_set_width(input_password, lv_pct(100));
            lv_textarea_set_one_line(input_password, true);

            btn_connect = lv_button_create(panel_setup);
            auto lbl_connect = lv_label_create(btn_connect);
            lv_obj_align(lbl_connect, LV_ALIGN_CENTER, 0, 0);
            lv_label_set_text(lbl_connect, "Connect");
            lv_obj_add_event_cb(btn_connect, LV_OBJ_EVENT_CB(AppWifiClass, on_connect_clicked), LV_EVENT_CLICKED, this);
            lv_obj_align_to(btn_connect, input_password, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

            Keyboard.bind_textarea(input_password);

            panel_connected = lv_obj_create(screen);
            lv_obj_set_size(panel_connected, lv_pct(100), lv_pct(88));
            lv_obj_align(panel_connected, LV_ALIGN_TOP_MID, 0, 0);
            lv_obj_set_style_pad_all(panel_connected, 0, LV_PART_MAIN);
            lv_obj_set_style_border_width(panel_connected, 0, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(panel_connected, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_remove_flag(panel_connected, LV_OBJ_FLAG_SCROLLABLE);

            auto lbl_status = lv_label_create(panel_connected);
            lv_label_set_text(lbl_status, "Connected to");
            label_connected_ssid = lv_label_create(panel_connected);
            lv_obj_align_to(label_connected_ssid, lbl_status, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);
            lv_label_set_long_mode(label_connected_ssid, LV_LABEL_LONG_WRAP);
            lv_obj_set_width(label_connected_ssid, lv_pct(100));

            btn_disconnect = lv_button_create(panel_connected);
            auto lbl_disc = lv_label_create(btn_disconnect);
            lv_obj_align(lbl_disc, LV_ALIGN_CENTER, 0, 0);
            lv_label_set_text(lbl_disc, "Disconnect");
            lv_obj_add_event_cb(btn_disconnect, LV_OBJ_EVENT_CB(AppWifiClass, on_disconnect_clicked), LV_EVENT_CLICKED, this);
            lv_obj_align_to(btn_disconnect, label_connected_ssid, LV_ALIGN_OUT_BOTTOM_MID, 0, 16);

            btn_back = lv_button_create(screen);
            lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, -4);
            lv_obj_set_width(btn_back, lv_pct(100));
            auto lbl_back = lv_label_create(btn_back);
            lv_obj_align(lbl_back, LV_ALIGN_CENTER, 0, 0);
            lv_label_set_text(lbl_back, "Back");
            lv_obj_add_event_cb(btn_back, LV_OBJ_EVENT_CB(AppWifiClass, on_back_clicked), LV_EVENT_CLICKED, this);
        }

        if (WiFi.status() == WL_CONNECTED)
            show_connected_panel();
        else
            show_setup_panel();
    }

    void app_close() override
    {
        reset_connection_state();
        teardown_ui();
    }

    void loop_ui() override
    {
        if (!screen)
            return;

        if (is_scan_wifi_completed)
        {
            is_scan_wifi_completed = false;
            if (dropdown_ssid)
            {
                std::string opts;
                for (size_t i = 0; i < scanned_wifi_names.size(); ++i)
                {
                    if (i > 0)
                        opts += '\n';
                    opts += scanned_wifi_names[i];
                }
                lv_dropdown_set_options(dropdown_ssid, opts.c_str());
            }
            set_controls_state(true);
        }

        if (connecting && WiFi.status() == WL_CONNECTED)
        {
            connecting = false;
            set_controls_state(true);
            show_connected_panel();
            return;
        }

        if (connecting && millis() - connect_started_ms >= APP_WIFI_CONNECT_TIMEOUT_MS)
        {
            WiFi.disconnect();
            connecting = false;
            MsgBox.error("Wifi connect failed!", nullptr, [this]()
                         { set_controls_state(true); });
        }
    }

    void loop() override {}

private:
    bool connecting = false;
    unsigned long connect_started_ms = 0;

    std::vector<std::string> scanned_wifi_names;
    bool is_scan_wifi_completed = false;
    TaskHandle_t task_handle_wifi_scan = nullptr;

    lv_obj_t *panel_connected = nullptr;
    lv_obj_t *panel_setup = nullptr;
    lv_obj_t *label_connected_ssid = nullptr;
    lv_obj_t *btn_disconnect = nullptr;
    lv_obj_t *ssid_row = nullptr;
    lv_obj_t *dropdown_ssid = nullptr;
    lv_obj_t *btn_scan_wifi = nullptr;
    lv_obj_t *input_password = nullptr;
    lv_obj_t *btn_connect = nullptr;
    lv_obj_t *btn_back = nullptr;

    void reset_connection_state()
    {
        connecting = false;
    }

    void stop_wifi_scan_task_if_running()
    {
        if (task_handle_wifi_scan == nullptr)
            return;

        vTaskDelete(task_handle_wifi_scan);
        task_handle_wifi_scan = nullptr;
        WiFi.scanDelete();
        scanned_wifi_names.clear();
        is_scan_wifi_completed = false;
    }

    void teardown_ui()
    {
        if (!screen)
            return;

        if (input_password)
            Keyboard.unbind_textarea(input_password);
        Keyboard.dismiss();

        stop_wifi_scan_task_if_running();

        lv_obj_delete(screen);
        screen = nullptr;

        panel_connected = nullptr;
        panel_setup = nullptr;
        label_connected_ssid = nullptr;
        btn_disconnect = nullptr;
        ssid_row = nullptr;
        dropdown_ssid = nullptr;
        btn_scan_wifi = nullptr;
        input_password = nullptr;
        btn_connect = nullptr;
        btn_back = nullptr;
    }

    void wifi_scan_task()
    {
        WiFi.scanDelete();
        WiFi.disconnect();
        const int n = WiFi.scanNetworks();

        std::vector<std::string> names;
        if (n > 0)
        {
            names.reserve(static_cast<size_t>(n));
            for (int i = 0; i < n; ++i)
                names.emplace_back(WiFi.SSID(i).c_str());
        }
        WiFi.scanDelete();

        scanned_wifi_names = std::move(names);
        is_scan_wifi_completed = true;

        TaskHandle_t self = xTaskGetCurrentTaskHandle();
        task_handle_wifi_scan = nullptr;
        vTaskDelete(self);
    }

    bool start_wifi_scan_task()
    {
        if (task_handle_wifi_scan != nullptr)
            return true;

        const BaseType_t ok = xTaskCreatePinnedToCore(
            FREERTOS_TASK_CB(AppWifiClass, wifi_scan_task),
            "app_wifi_scan_task",
            APP_WIFI_SCAN_TASK_STACK,
            this,
            APP_WIFI_SCAN_TASK_PRIORITY,
            &task_handle_wifi_scan,
            APP_WIFI_SCAN_TASK_CORE);

        if (ok != pdPASS)
        {
            task_handle_wifi_scan = nullptr;
            return false;
        }

        return true;
    }

    void on_scan_wifi_clicked()
    {
        if (task_handle_wifi_scan != nullptr || !dropdown_ssid)
            return;

        set_controls_state(false);
        if (!start_wifi_scan_task())
            set_controls_state(true);
    }

    void set_controls_state(bool enabled)
    {
        if (enabled)
        {
            if (dropdown_ssid && task_handle_wifi_scan == nullptr)
                lv_obj_remove_state(dropdown_ssid, LV_STATE_DISABLED);
            if (input_password)
                lv_obj_remove_state(input_password, LV_STATE_DISABLED);
            if (btn_connect)
                lv_obj_remove_state(btn_connect, LV_STATE_DISABLED);
            if (btn_scan_wifi)
                lv_obj_remove_state(btn_scan_wifi, LV_STATE_DISABLED);
            if (btn_back)
                lv_obj_remove_state(btn_back, LV_STATE_DISABLED);
        }
        else
        {
            if (dropdown_ssid)
                lv_obj_add_state(dropdown_ssid, LV_STATE_DISABLED);
            if (input_password)
                lv_obj_add_state(input_password, LV_STATE_DISABLED);
            if (btn_connect)
                lv_obj_add_state(btn_connect, LV_STATE_DISABLED);
            if (btn_scan_wifi)
                lv_obj_add_state(btn_scan_wifi, LV_STATE_DISABLED);
            if (btn_back)
                lv_obj_add_state(btn_back, LV_STATE_DISABLED);
        }
    }

    void show_connected_panel()
    {
        if (!panel_connected || !panel_setup || !label_connected_ssid)
            return;

        const String s = WiFi.SSID();
        lv_label_set_text(label_connected_ssid, s.length() ? s.c_str() : "(unknown)");

        lv_obj_remove_flag(panel_connected, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(panel_setup, LV_OBJ_FLAG_HIDDEN);
    }

    void show_setup_panel()
    {
        if (!panel_connected || !panel_setup || !dropdown_ssid)
            return;

        set_controls_state(false);
        if (!start_wifi_scan_task())
            set_controls_state(true);
        lv_obj_remove_flag(panel_setup, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(panel_connected, LV_OBJ_FLAG_HIDDEN);
    }

    void on_connect_clicked()
    {
        if (connecting)
            return;

        stop_wifi_scan_task_if_running();

        Keyboard.dismiss();

        set_controls_state(false);

        char ssid[64]{};
        lv_dropdown_get_selected_str(dropdown_ssid, ssid, sizeof(ssid));
        const char *pass = lv_textarea_get_text(input_password);

        WiFi.begin(ssid, pass);
        connecting = true;
        connect_started_ms = millis();
    }

    void on_disconnect_clicked()
    {
        WiFi.disconnect();
        Keyboard.dismiss();
        connecting = false;
        show_setup_panel();
    }

    void on_back_clicked()
    {
        reset_connection_state();
        teardown_ui();
        lv_screen_load(Home.get_screen());
    }
};

extern AppWifiClass AppWifi;
