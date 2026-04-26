#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include "touch.h"
#include "wifi_connector.h"

#define TFT_ROTATION LV_DISPLAY_ROTATION_0
#define DRAW_BUF_SIZE (TFT_WIDTH * TFT_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

#define FPS 60
lv_obj_t *lb_fps = nullptr;
unsigned long fps_count = 0;
unsigned long fps_time = 0;

uint32_t lv_tick_source(void)
{
    return millis();
}

void lv_touch_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    auto t = Touch::get_touch();
    if (!t.touched)
    {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    data->point.x = t.x;
    data->point.y = t.y;
    data->state = LV_INDEV_STATE_PRESSED;
}

void setup()
{
    Serial.begin(115200);

    Serial.println("[Setup] Begin setup");

    lv_init();
    lv_tick_set_cb(lv_tick_source);

    auto disp = lv_tft_espi_create(TFT_WIDTH, TFT_HEIGHT, draw_buf, sizeof(draw_buf));
    lv_display_set_rotation(disp, TFT_ROTATION);

    // lv_tft_espi_create() allocates its own TFT_eSPI (see LVGL lv_tft_espi.cpp).
    // Invert must run on that internal instance.
    typedef struct
    {
        TFT_eSPI *tft;
    } display_driver_data_t;
    auto tft_dsc = static_cast<display_driver_data_t *>(lv_display_get_driver_data(disp));
    if (tft_dsc && tft_dsc->tft)
    {
        tft_dsc->tft->invertDisplay(true);
    }

    Touch::setup(TFT_ROTATION);

    // Initialize the (dummy) input device driver
    auto indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lv_touch_read);

    lb_fps = lv_label_create(lv_layer_sys());
    lv_obj_align(lb_fps, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_label_set_recolor(lb_fps, true);
    lv_label_set_text(lb_fps, "0");

    Serial.println("[Setup] End setup");
}

void loop()
{
    const unsigned long start_time = millis();

    lv_timer_handler();

    if (!WifiConnector::canConnect())
    {
        WifiConnector::setup();
        WifiConnector::show_screen();
    }

    if (start_time < fps_time + 1000)
    {
        fps_count++;
    }
    else
    {
        lv_label_set_text_fmt(lb_fps, "#0077ff %d (%d)#", fps_count, start_time);
        fps_count = 1;
        fps_time = start_time;
    }

    const unsigned long process_time = millis() - start_time;
    if (process_time >= 1000 / FPS)
    {
        return;
    }

    delay(1000 / FPS - process_time);
}
