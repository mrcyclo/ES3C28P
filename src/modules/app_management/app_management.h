#pragma once

#include <string>
#include <vector>

#include "common/helpers.h"
#include "common/iapplication.h"
#include "common/imodule.h"
#include "config.h"
#include "msgbox/msgbox.h"

class AppManagementClass : public ModuleOnce {
public:
    void register_app(IApplication& app) {
        auto p = &app;
        for (auto x : apps) {
            if (x == p) return;
        }
        apps.push_back(p);
    }

    size_t app_count() const { return apps.size(); }

    IApplication* app_at(size_t index) { return apps.at(index); }

    const IApplication* app_at(size_t index) const { return apps.at(index); }

    void loop_ui() {
        // Chỉ dispatch loop_ui cho app đang mở. App khác không hiển thị nên không cần
        // tick mỗi frame; nếu cần phản ứng theo state riêng thì dùng event/callback.
        if (current_app) current_app->loop_ui();

        if (!close_app_button) return;

        const bool should_show = (current_app != nullptr && current_app->get_screen() != nullptr);
        if (should_show == close_btn_visible) return;

        close_btn_visible = should_show;
        if (should_show)
            lv_obj_remove_flag(close_app_button, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_add_flag(close_app_button, LV_OBJ_FLAG_HIDDEN);
    }

    void loop() {
        for (auto a : apps) a->loop();
    }

    void set_current_app(IApplication* app) { current_app = app; }

    void set_home_screen(lv_obj_t* screen) { home_screen = screen; }

protected:
    void setup_impl() override {
        close_app_button = lv_button_create(lv_layer_sys());
        lv_obj_align(close_app_button, LV_ALIGN_TOP_MID, 0, STATUS_BAR_HEIGHT / 2);
        lv_obj_set_size(close_app_button, STATUS_BAR_HEIGHT, STATUS_BAR_HEIGHT);
        lv_obj_set_style_radius(close_app_button, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(close_app_button, LV_COLOR_ERROR, LV_PART_MAIN);
        lv_obj_add_flag(close_app_button, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_event_cb(close_app_button, LV_OBJ_EVENT_CB(AppManagementClass, close_app_clicked_cb), LV_EVENT_CLICKED, this);

        auto close_icon_utf8 = fa(0xf00d);
        auto close_app_label = lv_label_create(close_app_button);
        lv_label_set_text(close_app_label, close_icon_utf8.c_str());
        lv_obj_align(close_app_label, LV_ALIGN_CENTER, 0, 0);
    }

private:
    IApplication* current_app = nullptr;
    std::vector<IApplication*> apps;
    lv_obj_t* close_app_button = nullptr;
    lv_obj_t* home_screen = nullptr;
    bool close_btn_visible = false;

    void close_app_clicked_cb() {
        MsgBox.confirm("Are you sure to close the current app?", "Close App", [this](bool confirmed) {
            if (!confirmed) return;

            current_app->app_close();
            current_app = nullptr;

            if (home_screen) lv_scr_load(home_screen);
        });
    }
};

extern AppManagementClass AppManagement;
