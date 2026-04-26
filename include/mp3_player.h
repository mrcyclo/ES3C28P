#pragma once

#include <lvgl.h>
#include "micro_sd.h"

class MP3PlayerClass
{
private:
    lv_obj_t *screen = nullptr;
    lv_obj_t *label_error = nullptr;

public:
    void setup()
    {
        screen = lv_obj_create(lv_screen_active());

        label_error = lv_label_create(screen);
        lv_obj_set_align(label_error, LV_ALIGN_CENTER);
        lv_label_set_text(label_error, "Error");
        lv_obj_add_flag(label_error, LV_OBJ_FLAG_HIDDEN);
    }

    void loop()
    {
        if (!MicroSD.is_mounted())
        {
            lv_label_set_text(label_error, "SD Card is not mounted");
            lv_obj_remove_flag(label_error, LV_OBJ_FLAG_HIDDEN);
            return;
        }
    }

    lv_obj_t *get_screen()
    {
        return screen;
    }
};

extern MP3PlayerClass MP3Player;
