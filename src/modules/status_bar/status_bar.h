#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <lvgl.h>

#include <cstdio>
#include <cstring>

#include "config.h"
#include "modules/common/helpers.h"
#include "modules/common/imodule.h"
#include "modules/fps/fps.h"
#include "modules/micro_sd/micro_sd.h"
#include "modules/msgbox/msgbox.h"
#include "modules/time_sync/time_sync.h"

#define STATUS_BAR_REFRESH_MS 500U
#define STATUS_BAR_BUF_SIZE 96

class StatusBarClass : public ModuleOnce {
public:
    void loop_ui() override {
        // Throttle: status bar chỉ chứa FPS + đồng hồ phút + vài icon → 500ms là quá đủ
        // và đã đủ smooth cho mắt người. Cắt 60Hz xuống 2Hz giảm rất nhiều tải LVGL.
        auto now = millis();
        if (now - last_refresh_ms < STATUS_BAR_REFRESH_MS) return;
        last_refresh_ms = now;

        build_left_text(scratch_buf, sizeof(scratch_buf));
        if (std::strcmp(scratch_buf, last_left_text) != 0) {
            std::strncpy(last_left_text, scratch_buf, sizeof(last_left_text) - 1);
            last_left_text[sizeof(last_left_text) - 1] = '\0';
            lv_label_set_text(lb_left, last_left_text);
        }

        build_right_text(scratch_buf, sizeof(scratch_buf));
        if (std::strcmp(scratch_buf, last_right_text) != 0) {
            std::strncpy(last_right_text, scratch_buf, sizeof(last_right_text) - 1);
            last_right_text[sizeof(last_right_text) - 1] = '\0';
            lv_label_set_text(lb_right, last_right_text);
        }
    }

    void loop() override {}

protected:
    void setup_impl() override {
        auto box = lv_obj_create(lv_layer_sys());
        lv_obj_align(box, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_size(box, lv_pct(100), STATUS_BAR_HEIGHT);
        lv_obj_set_style_bg_color(box, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(box, LV_OPA_80, LV_PART_MAIN);
        lv_obj_set_style_radius(box, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(box, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_ver(box, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_hor(box, 4, LV_PART_MAIN);
        lv_obj_remove_flag(box, LV_OBJ_FLAG_SCROLLABLE);

        lb_left = lv_label_create(box);
        lv_obj_align(lb_left, LV_ALIGN_LEFT_MID, 0, 0);
        lv_label_set_recolor(lb_left, true);
        lv_label_set_text(lb_left, "");

        lb_right = lv_label_create(box);
        lv_obj_align(lb_right, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_label_set_recolor(lb_right, true);
        lv_label_set_text(lb_right, "");
    }

private:
    lv_obj_t* lb_left = nullptr;
    lv_obj_t* lb_right = nullptr;
    unsigned long last_refresh_ms = 0;
    char scratch_buf[STATUS_BAR_BUF_SIZE]{};
    char last_left_text[STATUS_BAR_BUF_SIZE]{};
    char last_right_text[STATUS_BAR_BUF_SIZE]{};

    void build_left_text(char* buf, size_t buf_size) { std::snprintf(buf, buf_size, "#ffffff %lu#", Fps.get_fps()); }

    void build_right_text(char* buf, size_t buf_size) {
        size_t pos = 0;
        auto append = [&](const char* s) {
            if (!s || pos >= buf_size - 1) return;
            if (pos > 0 && pos < buf_size - 1) buf[pos++] = ' ';
            auto left = buf_size - 1 - pos;
            auto n = std::strlen(s);
            auto take = n < left ? n : left;
            std::memcpy(buf + pos, s, take);
            pos += take;
            buf[pos] = '\0';
        };

        if (MicroSD.is_mounted()) {
            auto s = std::string("#ffffff ") + fa(0xf7c2) + "#";
            append(s.c_str());
        }
        if (WiFi.status() == WL_CONNECTED) {
            auto s = std::string("#ffffff ") + fa(0xf1eb) + "#";
            append(s.c_str());
        }
        if (TimeSync.is_synced()) {
            auto t = TimeSync.get_time();
            char clk[24];
            std::snprintf(clk, sizeof(clk), "#ffffff %02d:%02d#", t.tm_hour, t.tm_min);
            append(clk);
        }

        if (pos == 0) buf[0] = '\0';
    }
};

extern StatusBarClass StatusBar;
