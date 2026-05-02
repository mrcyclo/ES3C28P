#pragma once

#include <vector>
#include "config.h"
#include "common/imodule.h"
#include "common/iapplication.h"
#include "common/helpers.h"
#include "msgbox/msgbox.h"

class AppManagementClass : public ModuleOnce
{
public:
    void register_app(IApplication &app)
    {
        IApplication *p = &app;
        for (IApplication *x : apps)
        {
            if (x == p)
                return;
        }
        apps.push_back(p);
    }

    size_t app_count() const { return apps.size(); }

    IApplication *app_at(size_t index) { return apps.at(index); }

    const IApplication *app_at(size_t index) const { return apps.at(index); }

    void loop_ui()
    {
        for (IApplication *a : apps)
            a->loop_ui();

        if (close_app_button)
        {
            if (current_app && current_app->get_screen())
                lv_obj_remove_flag(close_app_button, LV_OBJ_FLAG_HIDDEN);
            else
                lv_obj_add_flag(close_app_button, LV_OBJ_FLAG_HIDDEN);
        }
    }

    void loop()
    {
        for (IApplication *a : apps)
            a->loop();
    }

    void set_current_app(IApplication *app)
    {
        current_app = app;
    }

    void set_home_screen(lv_obj_t *screen)
    {
        home_screen = screen;
    }

protected:
    void setup_impl() override
    {
        close_app_button = lv_button_create(lv_layer_sys());
        lv_obj_align(close_app_button, LV_ALIGN_TOP_MID, 0, STATUS_BAR_HEIGHT / 2);
        lv_obj_set_size(close_app_button, STATUS_BAR_HEIGHT, STATUS_BAR_HEIGHT);
        lv_obj_set_style_radius(close_app_button, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(close_app_button, LV_COLOR_ERROR, LV_PART_MAIN);
        lv_obj_add_flag(close_app_button, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_event_cb(close_app_button, LV_OBJ_EVENT_CB(AppManagementClass, close_app_clicked_cb), LV_EVENT_CLICKED, this);

        auto close_app_label = lv_label_create(close_app_button);
        lv_label_set_text(close_app_label, fa(0xf00d).c_str());
        lv_obj_align(close_app_label, LV_ALIGN_CENTER, 0, 0);
    }

private:
    IApplication *current_app = nullptr;
    std::vector<IApplication *> apps;
    lv_obj_t *close_app_button = nullptr;
    lv_obj_t *home_screen = nullptr;

    void close_app_clicked_cb()
    {
        MsgBox.confirm("Are you sure to close the current app?", "Close App", [this](bool confirmed)
                       {
            if (!confirmed)
                return;


            current_app->app_close();
            current_app = nullptr;

            if (home_screen)
                lv_scr_load(home_screen); });
    }
};

extern AppManagementClass AppManagement;
