#pragma once

#include <lvgl.h>
#include <string>
#include <vector>
#include "wifi_connector.h"
#include "time_sync.h"
#include "micro_sd.h"
#include "helpers.h"

class StatusBarClass
{
private:
    lv_obj_t *lb_left = nullptr;
    lv_obj_t *lb_right = nullptr;

public:
    void setup()
    {
        auto box = lv_obj_create(lv_layer_sys());
        lv_obj_align(box, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_size(box, lv_pct(100), 20);
        lv_obj_set_style_bg_color(box, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(box, LV_OPA_80, LV_PART_MAIN);
        lv_obj_set_style_radius(box, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(box, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(box, 2, LV_PART_MAIN);
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

    void loop()
    {
        std::vector<std::string> right_statuses;

        if (MicroSD.is_mounted())
        {
            right_statuses.push_back("#ffffff " + fa(0xf7c2) + "#");
        }

        if (WifiConnector.is_connected())
        {
            right_statuses.push_back("#ffffff " + fa(0xf1eb) + "#");
        }

        if (TimeSync.is_synced())
        {
            auto time = TimeSync.get_time();
            char buf[6]; // "HH:MM" + null
            snprintf(buf, sizeof(buf), "%02d:%02d", time.tm_hour, time.tm_min);
            right_statuses.push_back("#ffffff " + std::string(buf) + "#");
        }

        std::string right_text;
        for (size_t i = 0; i < right_statuses.size(); ++i)
        {
            if (i > 0)
                right_text += ' ';
            right_text += right_statuses[i];
        }
        lv_label_set_text(lb_right, right_text.c_str());
    }
};

extern StatusBarClass StatusBar;
