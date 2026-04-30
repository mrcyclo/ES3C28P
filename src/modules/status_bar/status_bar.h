#pragma once

#include <lvgl.h>
#include <string>
#include <vector>
#include "fps/fps.h"
#include "wifi_connector/wifi_connector.h"
#include "time_sync/time_sync.h"
#include "micro_sd/micro_sd.h"
#include "common/helpers.h"
#include "msgbox/msgbox.h"
#include "common/imodule.h"
#include "config.h"

class StatusBarClass : public ModuleOnce
{
public:
    void loop_ui() override
    {
        std::vector<std::string> left_statuses;
        left_statuses.push_back("#ffffff " + std::to_string(Fps.get_fps()) + "#");

        std::string left_text;
        for (size_t i = 0; i < left_statuses.size(); ++i)
        {
            if (i > 0)
                left_text += ' ';
            left_text += left_statuses[i];
        }
        lv_label_set_text(lb_left, left_text.c_str());

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
            char buf[6];
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

    void loop() override {}

protected:
    void setup_impl() override
    {
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
    lv_obj_t *lb_left = nullptr;
    lv_obj_t *lb_right = nullptr;
};

extern StatusBarClass StatusBar;
