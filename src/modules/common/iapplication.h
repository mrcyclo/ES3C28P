#pragma once

#include <lvgl.h>

class IApplication
{
public:
    virtual ~IApplication() = default;
    virtual const char *get_drawer_icon_text() = 0;
    virtual void set_drawer_icon(lv_obj_t *icon) = 0;
    virtual lv_obj_t *get_screen() = 0;
    virtual void drawer_icon_clicked() = 0;
    virtual void app_close() = 0;
    virtual void loop_ui() = 0;
    virtual void loop() = 0;
};

class Application : public IApplication
{
public:
    void set_drawer_icon(lv_obj_t *icon) final
    {
        drawer_icon = icon;
    }

    lv_obj_t *get_screen() final
    {
        return screen;
    }

protected:
    lv_obj_t *drawer_icon = nullptr;
    lv_obj_t *screen = nullptr;
};
