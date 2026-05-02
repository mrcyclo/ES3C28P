#pragma once

#include <lvgl.h>

#include "common/imodule.h"

class KeyboardClass;

void keyboard_textarea_event(lv_event_t* e);

class KeyboardClass : public ModuleOnce {
    friend void keyboard_textarea_event(lv_event_t* e);

public:
    void loop_ui() override {}

    void loop() override {}

    lv_obj_t* keyboard_obj() { return keyboard; }

    void bind_textarea(lv_obj_t* ta) {
        if (!ta) return;
        lv_obj_add_event_cb(ta, keyboard_textarea_event, LV_EVENT_ALL, this);
    }

    void unbind_textarea(lv_obj_t* ta) {
        if (!ta) return;
        lv_obj_remove_event_cb_with_user_data(ta, keyboard_textarea_event, this);
    }

    void dismiss() {
        if (!keyboard) return;
        lv_keyboard_set_textarea(keyboard, nullptr);
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
    }

protected:
    void setup_impl() override {
        keyboard = lv_keyboard_create(lv_layer_top());
        lv_obj_add_flag(keyboard, LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_style_text_font(keyboard, &lv_font_montserrat_14, LV_PART_MAIN);
        lv_obj_set_style_text_font(keyboard, &lv_font_montserrat_14, LV_PART_ITEMS);
    }

private:
    lv_obj_t* keyboard = nullptr;
};

inline void keyboard_textarea_event(lv_event_t* e) {
    auto* self = static_cast<KeyboardClass*>(lv_event_get_user_data(e));
    if (!self || !self->keyboard) return;

    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* ta = lv_event_get_target_obj(e);

    if (code == LV_EVENT_FOCUSED) {
        if (lv_obj_has_state(ta, LV_STATE_DISABLED) || lv_obj_has_flag(ta, LV_OBJ_FLAG_HIDDEN)) {
            lv_keyboard_set_textarea(self->keyboard, nullptr);
            lv_obj_add_flag(self->keyboard, LV_OBJ_FLAG_HIDDEN);
            return;
        }

        lv_keyboard_set_textarea(self->keyboard, ta);
        lv_obj_remove_flag(self->keyboard, LV_OBJ_FLAG_HIDDEN);
    } else if (code == LV_EVENT_DEFOCUSED) {
        lv_keyboard_set_textarea(self->keyboard, nullptr);
        lv_obj_add_flag(self->keyboard, LV_OBJ_FLAG_HIDDEN);
    }
}

extern KeyboardClass Keyboard;
