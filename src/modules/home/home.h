#pragma once

#include <lvgl.h>

#include <string>
#include <vector>

#include "config.h"
#include "modules/app_management/app_management.h"
#include "modules/common/helpers.h"
#include "modules/common/imodule.h"
#include "modules/led/led.h"
#include "modules/micro_sd/micro_sd.h"
#include "modules/msgbox/msgbox.h"

#define MENU_BUTTON_SIZE 30
#define APP_ICON_SIZE 48

class HomeClass : public ModuleOnce {
public:
    void loop_ui() override {
        if (loaded) return;

        if (!MicroSD.is_mounted()) {
            lv_obj_add_flag(image_wallpaper, LV_OBJ_FLAG_HIDDEN);
            loaded = true;
            return;
        }

        auto ok = MicroSD.lv_read_bmp_dsc_rgb565("/.system/wallpaper.bmp", &wallpaper_dsc, &wallpaper_pixels);
        if (!ok) {
            lv_obj_add_flag(image_wallpaper, LV_OBJ_FLAG_HIDDEN);
            loaded = true;
            Serial.println("[Home] Failed to load wallpaper.bmp (expected 24-bit uncompressed BMP)");
            return;
        }

        lv_image_set_src(image_wallpaper, &wallpaper_dsc);
        lv_obj_remove_flag(image_wallpaper, LV_OBJ_FLAG_HIDDEN);
        loaded = true;
        Serial.println("[Home] Wallpaper loaded into RAM");
    }

    void loop() override {}

    lv_obj_t* get_screen() { return screen; }

protected:
    void setup_impl() override {
        screen = lv_obj_create(NULL);
        lv_obj_set_style_pad_all(screen, 0, LV_PART_MAIN);
        lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

        image_wallpaper = lv_image_create(screen);
        lv_obj_align(image_wallpaper, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_size(image_wallpaper, lv_pct(100), lv_pct(100));
        lv_obj_add_flag(image_wallpaper, LV_OBJ_FLAG_HIDDEN);

        auto content = lv_obj_create(screen);
        lv_obj_align(content, LV_ALIGN_CENTER, 0, 0);
        lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(content, lv_pct(100), lv_pct(100));
        lv_obj_set_layout(content, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(content, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(content, 5, LV_PART_MAIN);
        lv_obj_set_style_pad_top(content, STATUS_BAR_HEIGHT + 5, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(content, LV_OPA_0, LV_PART_MAIN);
        lv_obj_set_style_border_width(content, 0, LV_PART_MAIN);

        static int32_t drawer_col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};

        auto n = AppManagement.app_count();
        auto rows = (n == 0) ? 1u : (n + 2u) / 3u;
        drawer_row_dsc_storage.clear();
        drawer_row_dsc_storage.reserve(rows + 1);
        for (size_t r = 0; r < rows; ++r) drawer_row_dsc_storage.push_back(APP_ICON_SIZE);
        drawer_row_dsc_storage.push_back(LV_GRID_TEMPLATE_LAST);

        drawer = lv_obj_create(content);
        lv_obj_set_width(drawer, lv_pct(100));
        lv_obj_set_flex_grow(drawer, 1);
        lv_obj_set_style_bg_color(drawer, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(drawer, LV_OPA_50, LV_PART_MAIN);
        lv_obj_add_flag(drawer, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_grid_dsc_array(drawer, drawer_col_dsc, drawer_row_dsc_storage.data());
        lv_obj_set_layout(drawer, LV_LAYOUT_GRID);

        auto button_menu = lv_btn_create(content);
        lv_obj_add_event_cb(button_menu, LV_OBJ_EVENT_CB(HomeClass, button_menu_clicked_cb), LV_EVENT_CLICKED, this);
        lv_obj_set_size(button_menu, MENU_BUTTON_SIZE, MENU_BUTTON_SIZE);
        lv_obj_set_style_bg_color(button_menu, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(button_menu, LV_OPA_50, LV_PART_MAIN);
        lv_obj_set_style_radius(button_menu, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_border_width(button_menu, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(button_menu, lv_color_white(), LV_PART_MAIN);

        auto menu_icon_utf8 = fa(0xf58d);
        auto label_menu = lv_label_create(button_menu);
        lv_label_set_text(label_menu, menu_icon_utf8.c_str());
        lv_obj_align(label_menu, LV_ALIGN_CENTER, 0, 0);

        for (int32_t i = 0; i < static_cast<int32_t>(n); ++i) {
            auto app = AppManagement.app_at(static_cast<size_t>(i));
            auto col = i % 3;
            auto row = i / 3;

            auto btn = lv_button_create(drawer);
            lv_obj_set_size(btn, APP_ICON_SIZE, APP_ICON_SIZE);
            lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
            lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_CENTER, col, 1, LV_GRID_ALIGN_CENTER, row, 1);
            lv_obj_add_event_cb(btn, drawer_app_clicked_cb, LV_EVENT_CLICKED, static_cast<void*>(app));
            app->set_drawer_icon(btn);

            auto icon_label = lv_label_create(btn);
            lv_obj_align(icon_label, LV_ALIGN_CENTER, 0, 0);
            lv_label_set_text(icon_label, app->get_drawer_icon_text());
        }

        lv_obj_update_layout(content);
    }

private:
    lv_obj_t* screen = nullptr;
    lv_obj_t* image_wallpaper = nullptr;
    bool loaded = false;
    lv_image_dsc_t wallpaper_dsc{};
    uint16_t* wallpaper_pixels = nullptr;
    lv_obj_t* drawer = nullptr;
    bool drawer_open = false;
    std::vector<int32_t> drawer_row_dsc_storage;

    static void drawer_app_clicked_cb(lv_event_t* e) {
        auto app = static_cast<IApplication*>(lv_event_get_user_data(e));
        AppManagement.set_current_app(app);

        if (app) app->drawer_icon_clicked();

        auto screen = app->get_screen();
        if (screen) {
            lv_scr_load(screen);
        }
    }

    void button_menu_clicked_cb() {
        if (drawer_open) {
            lv_obj_add_flag(drawer, LV_OBJ_FLAG_HIDDEN);
            drawer_open = false;
        } else {
            lv_obj_remove_flag(drawer, LV_OBJ_FLAG_HIDDEN);
            lv_obj_update_layout(lv_obj_get_parent(drawer));
            drawer_open = true;
        }
    }
};

extern HomeClass Home;
