#pragma once

#include <functional>
#include <utility>
#include <lvgl.h>
#include "common/helpers.h"

class MsgBoxClass
{
public:
    template <typename Fn>
    inline void info(const char *text, const char *title, Fn &&fn)
    {
        dismiss_existing_silent();

        active_callback = std::function<void()>(std::forward<Fn>(fn));
        has_active_callback = true;

        msgbox = lv_msgbox_create(lv_layer_top());
        lv_obj_set_width(msgbox, lv_pct(90));
        lv_msgbox_add_title(msgbox, title ? title : "Infomation");
        lv_msgbox_add_text(msgbox, text);
        lv_obj_set_style_bg_color(lv_msgbox_get_header(msgbox), lv_color_hex(0x0291d5), LV_PART_MAIN);
        lv_obj_set_style_border_color(msgbox, lv_color_hex(0x0291d5), LV_PART_MAIN);

        lv_obj_t *btn_ok = lv_msgbox_add_footer_button(msgbox, "OK");
        lv_obj_add_event_cb(btn_ok, LV_OBJ_EVENT_CB(MsgBoxClass, ok_clicked_cb), LV_EVENT_CLICKED, this);

        create_overlay();
    }

    template <typename Fn>
    inline void error(const char *text, const char *title, Fn &&fn)
    {
        dismiss_existing_silent();

        active_callback = std::function<void()>(std::forward<Fn>(fn));
        has_active_callback = true;

        msgbox = lv_msgbox_create(lv_layer_top());
        lv_obj_set_width(msgbox, lv_pct(90));
        lv_msgbox_add_title(msgbox, title ? title : "Error");
        lv_msgbox_add_text(msgbox, text);
        lv_obj_set_style_bg_color(lv_msgbox_get_header(msgbox), lv_color_hex(0xac3e31), LV_PART_MAIN);
        lv_obj_set_style_border_color(msgbox, lv_color_hex(0xac3e31), LV_PART_MAIN);

        lv_obj_t *btn_ok = lv_msgbox_add_footer_button(msgbox, "OK");
        lv_obj_add_event_cb(btn_ok, LV_OBJ_EVENT_CB(MsgBoxClass, ok_clicked_cb), LV_EVENT_CLICKED, this);

        create_overlay();
    }

private:
    lv_obj_t *msgbox = nullptr;
    lv_obj_t *overlay = nullptr;

    std::function<void()> active_callback;
    bool has_active_callback = false;

    void ok_clicked_cb()
    {
        if (msgbox)
        {
            lv_msgbox_close(msgbox);
            lv_obj_delete(msgbox);
            msgbox = nullptr;
        }

        if (overlay)
        {
            lv_obj_delete(overlay);
            overlay = nullptr;
        }

        if (has_active_callback)
        {
            active_callback();
            active_callback = std::function<void()>();
            has_active_callback = false;
        }
    }

    void dismiss_existing_silent()
    {
        if (!msgbox && !overlay)
            return;

        active_callback = std::function<void()>();
        has_active_callback = false;

        if (msgbox)
        {
            lv_msgbox_close(msgbox);
            lv_obj_delete(msgbox);
            msgbox = nullptr;
        }

        if (overlay)
        {
            lv_obj_delete(overlay);
            overlay = nullptr;
        }
    }

    /**
     * Lớp trong suốt phủ màn hình
     * Chặn click xuống màn hình phía dưới
     * index 0 = dưới msgbox
     */
    void create_overlay()
    {
        overlay = lv_obj_create(lv_layer_top());
        lv_obj_remove_flag(overlay, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_size(overlay, lv_pct(100), lv_pct(100));
        lv_obj_set_align(overlay, LV_ALIGN_CENTER);
        lv_obj_add_flag(overlay, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_style_bg_color(overlay, lv_color_black(), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(overlay, LV_OPA_50, LV_PART_MAIN);
        lv_obj_set_style_radius(overlay, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(overlay, 0, LV_PART_MAIN);
        lv_obj_move_background(overlay);
    }
};

extern MsgBoxClass MsgBox;
