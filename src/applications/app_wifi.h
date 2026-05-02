#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <lvgl.h>

#include <string>
#include <vector>

#include "config.h"
#include "modules/common/helpers.h"
#include "modules/common/iapplication.h"
#include "modules/home/home.h"
#include "modules/keyboard/keyboard.h"
#include "modules/micro_sd/micro_sd.h"
#include "modules/msgbox/msgbox.h"

#define APP_WIFI_CONNECT_TIMEOUT_MS 15000U
#define APP_WIFI_SCAN_TASK_STACK 8192
#define APP_WIFI_SCAN_TASK_PRIORITY 1
#define APP_WIFI_SCAN_TASK_CORE 1
#define APP_WIFI_RSSI_REFRESH_MS 1500U
#define APP_WIFI_LOOP_UI_THROTTLE_MS 1000U

class AppWifiClass : public Application {
public:
    AppWifiClass() : drawer_icon_utf8(fa(0xf1eb)) {}

    const char* get_drawer_icon_text() override { return drawer_icon_utf8.c_str(); }

    void drawer_icon_clicked() override {
        if (!screen) {
            screen = lv_obj_create(nullptr);
            lv_obj_set_style_pad_all(screen, 10, LV_PART_MAIN);
            lv_obj_set_style_pad_top(screen, STATUS_BAR_HEIGHT + 10, LV_PART_MAIN);
            lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

            panel_setup = lv_obj_create(screen);
            lv_obj_set_size(panel_setup, lv_pct(100), lv_pct(100));
            lv_obj_align(panel_setup, LV_ALIGN_TOP_MID, 0, 0);
            lv_obj_set_style_pad_all(panel_setup, 0, LV_PART_MAIN);
            lv_obj_set_style_border_width(panel_setup, 0, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(panel_setup, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_remove_flag(panel_setup, LV_OBJ_FLAG_SCROLLABLE);

            auto lbl_ssid = lv_label_create(panel_setup);
            lv_label_set_text(lbl_ssid, "SSID:");

            auto ssid_row = lv_obj_create(panel_setup);
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
            lv_obj_add_event_cb(dropdown_ssid, LV_OBJ_EVENT_CB(AppWifiClass, on_dropdown_ssid_changed), LV_EVENT_VALUE_CHANGED, this);

            btn_scan_wifi = lv_button_create(ssid_row);
            lv_obj_set_size(btn_scan_wifi, 40, 40);

            auto scan_icon_utf8 = fa(0xf2f1);
            auto lbl_scan = lv_label_create(btn_scan_wifi);
            lv_obj_align(lbl_scan, LV_ALIGN_CENTER, 0, 0);
            lv_label_set_text(lbl_scan, scan_icon_utf8.c_str());
            lv_obj_add_event_cb(btn_scan_wifi, LV_OBJ_EVENT_CB(AppWifiClass, on_scan_wifi_clicked), LV_EVENT_CLICKED, this);

            auto lbl_pw = lv_label_create(panel_setup);
            lv_obj_align_to(lbl_pw, ssid_row, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
            lv_label_set_text(lbl_pw, "Password:");

            input_password = lv_textarea_create(panel_setup);
            lv_obj_align_to(input_password, lbl_pw, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
            lv_obj_set_width(input_password, lv_pct(100));
            lv_textarea_set_one_line(input_password, true);
            Keyboard.bind_textarea(input_password);

            btn_connect = lv_button_create(panel_setup);
            auto lbl_connect = lv_label_create(btn_connect);
            lv_obj_align(lbl_connect, LV_ALIGN_CENTER, 0, 0);
            lv_label_set_text(lbl_connect, "Connect");
            lv_obj_add_event_cb(btn_connect, LV_OBJ_EVENT_CB(AppWifiClass, on_connect_clicked), LV_EVENT_CLICKED, this);
            lv_obj_align_to(btn_connect, input_password, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);

            panel_connected = lv_obj_create(screen);
            lv_obj_set_size(panel_connected, lv_pct(100), lv_pct(100));
            lv_obj_align(panel_connected, LV_ALIGN_TOP_MID, 0, 0);
            lv_obj_set_style_pad_all(panel_connected, 0, LV_PART_MAIN);
            lv_obj_set_style_border_width(panel_connected, 0, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(panel_connected, LV_OPA_TRANSP, LV_PART_MAIN);
            lv_obj_remove_flag(panel_connected, LV_OBJ_FLAG_SCROLLABLE);

            auto row_connected_status = lv_obj_create(panel_connected);
            lv_obj_set_width(row_connected_status, lv_pct(100));
            lv_obj_set_height(row_connected_status, LV_SIZE_CONTENT);
            lv_obj_align(row_connected_status, LV_ALIGN_TOP_LEFT, 0, 0);
            lv_obj_set_layout(row_connected_status, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(row_connected_status, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row_connected_status, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_column(row_connected_status, 8, LV_PART_MAIN);
            lv_obj_set_style_pad_all(row_connected_status, 0, LV_PART_MAIN);
            lv_obj_set_style_border_width(row_connected_status, 0, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(row_connected_status, LV_OPA_TRANSP, LV_PART_MAIN);

            auto lbl_status = lv_label_create(row_connected_status);
            lv_label_set_text(lbl_status, "Connected to");

            label_connected_ssid = lv_label_create(row_connected_status);
            lv_obj_set_flex_grow(label_connected_ssid, 1);
            lv_label_set_long_mode(label_connected_ssid, LV_LABEL_LONG_MODE_DOTS);

            auto row_connected_rssi = lv_obj_create(panel_connected);
            lv_obj_set_width(row_connected_rssi, lv_pct(100));
            lv_obj_set_height(row_connected_rssi, LV_SIZE_CONTENT);
            lv_obj_align_to(row_connected_rssi, row_connected_status, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
            lv_obj_set_layout(row_connected_rssi, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(row_connected_rssi, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row_connected_rssi, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_column(row_connected_rssi, 8, LV_PART_MAIN);
            lv_obj_set_style_pad_all(row_connected_rssi, 0, LV_PART_MAIN);
            lv_obj_set_style_border_width(row_connected_rssi, 0, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(row_connected_rssi, LV_OPA_TRANSP, LV_PART_MAIN);

            auto lbl_rssi = lv_label_create(row_connected_rssi);
            lv_label_set_text(lbl_rssi, "RSSI:");

            label_connected_rssi = lv_label_create(row_connected_rssi);
            lv_obj_set_flex_grow(label_connected_rssi, 1);
            lv_label_set_long_mode(label_connected_rssi, LV_LABEL_LONG_MODE_CLIP);

            auto row_connected_ip = lv_obj_create(panel_connected);
            lv_obj_set_width(row_connected_ip, lv_pct(100));
            lv_obj_set_height(row_connected_ip, LV_SIZE_CONTENT);
            lv_obj_align_to(row_connected_ip, row_connected_rssi, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
            lv_obj_set_layout(row_connected_ip, LV_LAYOUT_FLEX);
            lv_obj_set_flex_flow(row_connected_ip, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row_connected_ip, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
            lv_obj_set_style_pad_column(row_connected_ip, 8, LV_PART_MAIN);
            lv_obj_set_style_pad_all(row_connected_ip, 0, LV_PART_MAIN);
            lv_obj_set_style_border_width(row_connected_ip, 0, LV_PART_MAIN);
            lv_obj_set_style_bg_opa(row_connected_ip, LV_OPA_TRANSP, LV_PART_MAIN);

            auto lbl_ip = lv_label_create(row_connected_ip);
            lv_label_set_text(lbl_ip, "IP:");

            label_connected_ip = lv_label_create(row_connected_ip);
            lv_obj_set_flex_grow(label_connected_ip, 1);
            lv_label_set_long_mode(label_connected_ip, LV_LABEL_LONG_MODE_DOTS);

            btn_disconnect = lv_button_create(panel_connected);
            auto lbl_disc = lv_label_create(btn_disconnect);
            lv_obj_align(lbl_disc, LV_ALIGN_CENTER, 0, 0);
            lv_label_set_text(lbl_disc, "Disconnect");
            lv_obj_add_event_cb(btn_disconnect, LV_OBJ_EVENT_CB(AppWifiClass, on_disconnect_clicked), LV_EVENT_CLICKED, this);
            lv_obj_align_to(btn_disconnect, row_connected_ip, LV_ALIGN_OUT_BOTTOM_MID, 0, 16);

            wifi_evt_sta_connected = WiFi.onEvent([this](arduino_event_id_t, arduino_event_info_t) { show_connected_panel(); }, ARDUINO_EVENT_WIFI_STA_CONNECTED);
            wifi_evt_sta_disconnected = WiFi.onEvent([this](arduino_event_id_t, arduino_event_info_t) { show_setup_panel(); }, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);
            // GOT_IP đến sau STA_CONNECTED khi DHCP cấp IP xong → cập nhật lại label IP/RSSI
            // và clear cờ `connecting` để loop_ui không phải poll WiFi.status() mỗi giây.
            wifi_evt_sta_got_ip = WiFi.onEvent(
                [this](arduino_event_id_t, arduino_event_info_t) {
                    connecting = false;
                    if (panel_connected) {
                        refresh_connected_rssi_label();
                        refresh_connected_ip_label();
                    }
                    set_controls_state(true);
                },
                ARDUINO_EVENT_WIFI_STA_GOT_IP
            );

            wifi_stored_load_from_sd();
        }

        if (WiFi.status() == WL_CONNECTED) {
            show_connected_panel();
        } else {
            show_setup_panel();
        }
    }

    void app_close() override {
        connecting = false;

        if (wifi_evt_sta_connected) {
            WiFi.removeEvent(wifi_evt_sta_connected);
            wifi_evt_sta_connected = 0;
        }
        if (wifi_evt_sta_disconnected) {
            WiFi.removeEvent(wifi_evt_sta_disconnected);
            wifi_evt_sta_disconnected = 0;
        }
        if (wifi_evt_sta_got_ip) {
            WiFi.removeEvent(wifi_evt_sta_got_ip);
            wifi_evt_sta_got_ip = 0;
        }

        if (!screen) return;

        wifi_stored_entries.clear();

        // Reset cache: widget label sắp bị xoá, lần mở lại sẽ là widget mới (text rỗng).
        last_rssi_text.clear();
        last_ip_text.clear();
        last_loop_ui_ms = 0;
        last_connected_rssi_ui_ms = 0;

        Keyboard.unbind_textarea(input_password);
        Keyboard.dismiss();

        stop_wifi_scan_task_if_running();

        lv_obj_delete(screen);
        screen = nullptr;

        panel_connected = nullptr;
        panel_setup = nullptr;
        label_connected_ssid = nullptr;
        label_connected_rssi = nullptr;
        label_connected_ip = nullptr;
        btn_disconnect = nullptr;
        dropdown_ssid = nullptr;
        btn_scan_wifi = nullptr;
        input_password = nullptr;
        btn_connect = nullptr;
    }

    void loop_ui() override {
        if (!screen) return;

        // Throttle 1Hz: tất cả việc trong loop_ui đều có thể chờ tới 1s mới xử lý:
        // - Scan-done được lv_async_call gọi trực tiếp, không poll ở đây.
        // - Connect thành công: event GOT_IP đã clear `connecting`.
        // - Connect thất bại: timeout check chỉ cần độ chính xác 1s.
        // - RSSI/IP: thay đổi rất chậm.
        const unsigned long now = millis();
        if (now - last_loop_ui_ms < APP_WIFI_LOOP_UI_THROTTLE_MS) return;
        last_loop_ui_ms = now;

        if (connecting && now - connect_started_ms >= APP_WIFI_CONNECT_TIMEOUT_MS) {
            connecting = false;
            WiFi.disconnect();
            MsgBox.error("Wifi connect failed!", nullptr, [this](bool) { set_controls_state(true); });
        }

        if (panel_connected && WiFi.status() == WL_CONNECTED) {
            if (now - last_connected_rssi_ui_ms >= APP_WIFI_RSSI_REFRESH_MS) {
                last_connected_rssi_ui_ms = now;
                refresh_connected_rssi_label();
                refresh_connected_ip_label();
            }
        }
    }

    void loop() override {}

private:
    // UTF-8 icon phải sống lâu: `fa(...).c_str()` trên temporary là UB → label rỗng/garbage.
    std::string drawer_icon_utf8;

    bool connecting = false;
    unsigned long connect_started_ms = 0;
    unsigned long last_connected_rssi_ui_ms = 0;
    unsigned long last_loop_ui_ms = 0;

    std::vector<std::string> scanned_wifi_names;
    TaskHandle_t task_handle_wifi_scan = nullptr;

    lv_obj_t* panel_connected = nullptr;
    lv_obj_t* panel_setup = nullptr;
    lv_obj_t* label_connected_ssid = nullptr;
    lv_obj_t* label_connected_rssi = nullptr;
    lv_obj_t* label_connected_ip = nullptr;
    lv_obj_t* btn_disconnect = nullptr;
    lv_obj_t* dropdown_ssid = nullptr;
    lv_obj_t* btn_scan_wifi = nullptr;
    lv_obj_t* input_password = nullptr;
    lv_obj_t* btn_connect = nullptr;

    std::string last_rssi_text;
    std::string last_ip_text;

    wifi_event_id_t wifi_evt_sta_connected = 0;
    wifi_event_id_t wifi_evt_sta_disconnected = 0;
    wifi_event_id_t wifi_evt_sta_got_ip = 0;

    struct wifi_stored_entry {
        std::string ssid;
        std::string passpharse;
    };

    std::vector<wifi_stored_entry> wifi_stored_entries;

    void wifi_stored_load_from_sd() {
        wifi_stored_entries.clear();
        if (!MicroSD.is_mounted()) return;

        auto f = MicroSD.fs().open(APP_WIFI_JSON_PATH, FILE_READ);
        if (!f) return;

        auto doc = JsonDocument();
        auto err = deserializeJson(doc, f);
        f.close();
        if (err) return;

        auto arr = doc["stored"].as<JsonArray>();
        if (arr.isNull()) return;

        for (auto v : arr) {
            auto o = v.as<JsonObject>();
            if (o.isNull()) continue;

            auto s = o["ssid"].as<const char*>();
            auto p = o["passpharse"].as<const char*>();
            if (!s || !s[0]) continue;

            wifi_stored_entry e;
            e.ssid = s;
            e.passpharse = p ? p : "";
            wifi_stored_entries.push_back(std::move(e));
        }
    }

    void wifi_stored_merge_in_memory(const char* ssid, const char* pass) {
        if (!ssid || !ssid[0]) return;

        auto ps = std::string(pass ? pass : "");
        for (wifi_stored_entry& e : wifi_stored_entries) {
            if (e.ssid == ssid) {
                e.passpharse = ps;
                return;
            }
        }

        wifi_stored_entries.push_back({std::string(ssid), ps});
    }

    bool wifi_stored_write_file(const char* connected_ssid) {
        if (!MicroSD.is_mounted()) {
            Serial.println("[AppWifi] SD not mounted; wifi.json not saved");
            return false;
        }

        MicroSD.fs().mkdir("/.system");

        auto doc = JsonDocument();
        doc["connected"] = connected_ssid ? connected_ssid : "";
        auto arr = doc["stored"].to<JsonArray>();
        for (auto& e : wifi_stored_entries) {
            auto o = arr.add<JsonObject>();
            o["ssid"] = e.ssid;
            o["passpharse"] = e.passpharse;
        }

        auto f = MicroSD.fs().open(APP_WIFI_JSON_PATH, FILE_WRITE);
        if (!f) {
            Serial.println("[AppWifi] open wifi.json for write failed");
            return false;
        }

        if (serializeJson(doc, f) == 0) {
            f.close();
            return false;
        }

        f.close();
        return true;
    }

    void refresh_password_from_stored() {
        if (!dropdown_ssid || !input_password) return;

        lv_textarea_set_text(input_password, "");

        char ssid[64]{};
        lv_dropdown_get_selected_str(dropdown_ssid, ssid, sizeof(ssid));
        if (!ssid[0]) return;

        for (auto& e : wifi_stored_entries) {
            if (e.ssid != ssid) continue;
            lv_textarea_set_text(input_password, e.passpharse.c_str());
            break;
        }
    }

    void on_dropdown_ssid_changed() { refresh_password_from_stored(); }

    void stop_wifi_scan_task_if_running() {
        if (task_handle_wifi_scan == nullptr) return;

        vTaskDelete(task_handle_wifi_scan);
        task_handle_wifi_scan = nullptr;
        WiFi.scanDelete();
        scanned_wifi_names.clear();
    }

    void wifi_scan_task() {
        WiFi.scanDelete();
        WiFi.disconnect();

        scanned_wifi_names.clear();
        auto n = WiFi.scanNetworks();
        for (int i = 0; i < n; ++i) {
            scanned_wifi_names.push_back(WiFi.SSID(i).c_str());
        }
        WiFi.scanDelete();

        // Bàn giao kết quả về thread LVGL qua lv_async_call (thread-safe).
        // Hàm sẽ chạy 1 lần ở lần `lv_timer_handler()` kế tiếp → không cần poll cờ trong loop_ui.
        lv_async_call(scan_done_async_cb, this);

        auto self = xTaskGetCurrentTaskHandle();
        task_handle_wifi_scan = nullptr;
        vTaskDelete(self);
    }

    static void scan_done_async_cb(void* user_data) {
        auto self = static_cast<AppWifiClass*>(user_data);
        if (!self || !self->screen) return;

        if (self->dropdown_ssid) {
            auto opts = std::string();
            for (size_t i = 0; i < self->scanned_wifi_names.size(); ++i) {
                if (i > 0) opts += '\n';
                opts += self->scanned_wifi_names[i];
            }
            lv_dropdown_set_options(self->dropdown_ssid, opts.c_str());
            self->refresh_password_from_stored();
        }
        self->set_controls_state(true);
    }

    bool start_wifi_scan_task() {
        if (task_handle_wifi_scan != nullptr) return true;

        auto ok = xTaskCreatePinnedToCore(
            FREERTOS_TASK_CB(AppWifiClass, wifi_scan_task),
            "app_wifi_scan_task",
            APP_WIFI_SCAN_TASK_STACK,
            this,
            APP_WIFI_SCAN_TASK_PRIORITY,
            &task_handle_wifi_scan,
            APP_WIFI_SCAN_TASK_CORE
        );

        if (ok != pdPASS) {
            task_handle_wifi_scan = nullptr;
            return false;
        }

        return true;
    }

    void on_scan_wifi_clicked() {
        set_controls_state(false);
        if (!start_wifi_scan_task()) {
            set_controls_state(true);
        }
    }

    void set_controls_state(bool enabled) {
        if (enabled) {
            if (dropdown_ssid && task_handle_wifi_scan == nullptr) lv_obj_remove_state(dropdown_ssid, LV_STATE_DISABLED);
            if (input_password) {
                lv_obj_remove_state(input_password, LV_STATE_DISABLED);
                lv_obj_add_flag(input_password, LV_OBJ_FLAG_CLICK_FOCUSABLE);
            }
            if (btn_connect) lv_obj_remove_state(btn_connect, LV_STATE_DISABLED);
            if (btn_scan_wifi) lv_obj_remove_state(btn_scan_wifi, LV_STATE_DISABLED);
        } else {
            if (dropdown_ssid) lv_obj_add_state(dropdown_ssid, LV_STATE_DISABLED);
            if (input_password) {
                lv_obj_remove_flag(input_password, LV_OBJ_FLAG_CLICK_FOCUSABLE);
                lv_obj_clear_state(input_password, LV_STATE_FOCUSED);
                lv_obj_add_state(input_password, LV_STATE_DISABLED);
            }
            if (btn_connect) lv_obj_add_state(btn_connect, LV_STATE_DISABLED);
            if (btn_scan_wifi) lv_obj_add_state(btn_scan_wifi, LV_STATE_DISABLED);
        }
    }

    void refresh_connected_rssi_label() {
        if (!label_connected_rssi) return;
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%d dBm", (int)WiFi.RSSI());
        if (last_rssi_text == buf) return;
        last_rssi_text = buf;
        lv_label_set_text(label_connected_rssi, buf);
    }

    void refresh_connected_ip_label() {
        if (!label_connected_ip) return;
        auto ip = WiFi.localIP().toString();
        const char* show = (ip.length() && ip != "0.0.0.0") ? ip.c_str() : "(none)";
        if (last_ip_text == show) return;
        last_ip_text = show;
        lv_label_set_text(label_connected_ip, show);
    }

    void show_connected_panel() {
        auto ssid = WiFi.SSID();
        lv_label_set_text(label_connected_ssid, ssid.length() ? ssid.c_str() : "(unknown)");

        refresh_connected_rssi_label();
        last_connected_rssi_ui_ms = millis();

        refresh_connected_ip_label();

        lv_obj_remove_flag(panel_connected, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(panel_setup, LV_OBJ_FLAG_HIDDEN);
    }

    void show_setup_panel() {
        set_controls_state(false);
        if (!start_wifi_scan_task()) set_controls_state(true);
        lv_obj_remove_flag(panel_setup, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(panel_connected, LV_OBJ_FLAG_HIDDEN);
    }

    void on_connect_clicked() {
        if (connecting) return;

        stop_wifi_scan_task_if_running();

        Keyboard.dismiss();

        set_controls_state(false);

        char ssid[64]{};
        lv_dropdown_get_selected_str(dropdown_ssid, ssid, sizeof(ssid));
        const char* pass = lv_textarea_get_text(input_password);

        if (ssid[0]) {
            wifi_stored_merge_in_memory(ssid, pass);
            wifi_stored_write_file(ssid);
        }

        WiFi.persistent(false);
        WiFi.begin(ssid, pass);
        connecting = true;
        connect_started_ms = millis();
    }

    void on_disconnect_clicked() {
        WiFi.disconnect();
        Keyboard.dismiss();
        connecting = false;
    }
};

extern AppWifiClass AppWifi;
