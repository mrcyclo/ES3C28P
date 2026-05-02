#pragma once

#include <TFT_eSPI.h>

#include "FT6336.h"
#include "config.h"
#include "modules/common/imodule.h"

#define TOUCH_FT6336
#define TOUCH_FT6336_SCL 15
#define TOUCH_FT6336_SDA 16
#define TOUCH_FT6336_INT 17
#define TOUCH_FT6336_RST 18

#define TOUCH_MAP_X1 0
#define TOUCH_MAP_X2 TFT_WIDTH
#define TOUCH_MAP_Y1 0
#define TOUCH_MAP_Y2 TFT_HEIGHT

struct TouchResult {
    bool touched;
    uint16_t x;
    uint16_t y;
};

class TouchClass : public ModuleOnce {
public:
    void loop_ui() override {}

    void loop() override {}

    TouchResult get_touch() {
        ts.read();

        TouchResult result{false, 0, 0};
        if (!ts.isTouched) {
            return result;
        }

        result.touched = true;
        result.x = (uint16_t)map(ts.points[0].x, min_x, max_x, 0, width - 1);
        result.y = (uint16_t)map(ts.points[0].y, min_y, max_y, 0, height - 1);
        return result;
    }

protected:
    void setup_impl() override {
        switch (TFT_ROTATION) {
            case ROTATION_NORMAL:
            case ROTATION_INVERTED:
                width = TFT_WIDTH;
                height = TFT_HEIGHT;
                min_x = TOUCH_MAP_X1;
                max_x = TOUCH_MAP_X2;
                min_y = TOUCH_MAP_Y1;
                max_y = TOUCH_MAP_Y2;
                break;

            case ROTATION_LEFT:
            case ROTATION_RIGHT:
                width = TFT_HEIGHT;
                height = TFT_WIDTH;
                min_x = TOUCH_MAP_Y1;
                max_x = TOUCH_MAP_Y2;
                min_y = TOUCH_MAP_X1;
                max_y = TOUCH_MAP_X2;
                break;
        }

        ts.begin();
        ts.setRotation(TFT_ROTATION);
    }

private:
    unsigned short int width = 0, height = 0, min_x = 0, max_x = 0, min_y = 0, max_y = 0;
    FT6336 ts = FT6336(TOUCH_FT6336_SDA, TOUCH_FT6336_SCL, TOUCH_FT6336_INT, TOUCH_FT6336_RST, max(TOUCH_MAP_X1, TOUCH_MAP_X2), max(TOUCH_MAP_Y1, TOUCH_MAP_Y2));
};

extern TouchClass Touch;
