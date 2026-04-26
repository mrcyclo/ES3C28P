#pragma once

#include <functional>
#include <utility>
#include <lvgl.h>

static lv_obj_t *msgbox = nullptr;
static lv_obj_t *msgbox_overlay = nullptr;

static void ok_btn_event_cb(lv_event_t *e)
{
    lv_msgbox_close(msgbox);
    lv_obj_delete(msgbox);
    msgbox = nullptr;

    if (msgbox_overlay)
    {
        lv_obj_delete(msgbox_overlay);
        msgbox_overlay = nullptr;
    }

    auto *fn = static_cast<std::function<void()> *>(lv_event_get_user_data(e));
    if (fn)
    {
        (*fn)();
        delete fn;
    }
}

/**
 * Msgbox một nút OK. Luôn truyền callable (lambda, functor, void(*)()).
 * Gọi: my_info_msgbox("Nội dung", nullptr, []() { ... });
 */
template <typename Fn>
inline void my_info_msgbox(const char *text, const char *title, Fn &&fn)
{
    auto *stored = new std::function<void()>(std::forward<Fn>(fn));

    msgbox = lv_msgbox_create(lv_layer_top());
    lv_obj_set_width(msgbox, lv_pct(90));
    lv_msgbox_add_title(msgbox, "Infomation");
    lv_msgbox_add_text(msgbox, text);
    lv_obj_set_style_bg_color(lv_msgbox_get_header(msgbox), lv_color_hex(0x0291d5), LV_PART_MAIN);
    lv_obj_set_style_border_color(msgbox, lv_color_hex(0x0291d5), LV_PART_MAIN);

    lv_obj_t *btn_ok = lv_msgbox_add_footer_button(msgbox, "OK");
    lv_obj_add_event_cb(btn_ok, ok_btn_event_cb, LV_EVENT_CLICKED, stored);

    // Lớp trong suốt phủ màn hình: chặn click xuống màn hình phía dưới; index 0 = dưới msgbox.
    msgbox_overlay = lv_obj_create(lv_layer_top());
    lv_obj_remove_flag(msgbox_overlay, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(msgbox_overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_align(msgbox_overlay, LV_ALIGN_CENTER);
    lv_obj_add_flag(msgbox_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_style_bg_color(msgbox_overlay, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(msgbox_overlay, 25, LV_PART_MAIN);
    lv_obj_set_style_border_width(msgbox_overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_blur_radius(msgbox_overlay, 20, LV_PART_MAIN);
    lv_obj_move_to_index(msgbox_overlay, 0);
}
