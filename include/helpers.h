#pragma once

#include <cstdint>
#include <string>
#include <lvgl.h>

#define LV_OBJ_EVENT_CB(Class, Method) [](lv_event_t *e) { static_cast<Class *>(lv_event_get_user_data(e))->Method(e); }

static void textarea_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target_obj(e);
    lv_obj_t *kb = (lv_obj_t *)lv_event_get_user_data(e);
    if (code == LV_EVENT_FOCUSED)
    {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_remove_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }

    if (code == LV_EVENT_DEFOCUSED)
    {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
}

static std::string fa(uint32_t cp)
{
    std::string s;
    if (cp <= 0x7FU)
    {
        s += static_cast<char>(cp);
    }
    else if (cp <= 0x7FFU)
    {
        s += static_cast<char>(0xC0U | (cp >> 6));
        s += static_cast<char>(0x80U | (cp & 0x3FU));
    }
    else if (cp <= 0xFFFFU)
    {
        if (cp >= 0xD800U && cp <= 0xDFFFU)
            return s; // surrogate — invalid đứng một mình
        s += static_cast<char>(0xE0U | (cp >> 12));
        s += static_cast<char>(0x80U | ((cp >> 6) & 0x3FU));
        s += static_cast<char>(0x80U | (cp & 0x3FU));
    }
    else if (cp <= 0x10FFFFU)
    {
        s += static_cast<char>(0xF0U | (cp >> 18));
        s += static_cast<char>(0x80U | ((cp >> 12) & 0x3FU));
        s += static_cast<char>(0x80U | ((cp >> 6) & 0x3FU));
        s += static_cast<char>(0x80U | (cp & 0x3FU));
    }
    return s;
}
