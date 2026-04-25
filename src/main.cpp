#include <Arduino.h>
#include <TFT_eSPI.h>
#include <lvgl.h>

#define TFT_ROTATION LV_DISPLAY_ROTATION_0
#define DRAW_BUF_SIZE (TFT_WIDTH * TFT_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
static uint32_t draw_buf[DRAW_BUF_SIZE / 4];

static uint32_t lv_tick_source(void)
{
    return millis();
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

    // Initialize the (dummy) input device driver
    // lv_indev_t * indev = lv_indev_create();
    // lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER); /*Touchpad should have POINTER type*/
    // lv_indev_set_read_cb(indev, my_touchpad_read);

    auto btn = lv_button_create(lv_screen_active());
    lv_obj_align(btn, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_size(btn, 100, 50);

    auto label = lv_label_create(btn);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(label, "Button");

    Serial.println("[Setup] End setup");
}

void loop()
{
    lv_timer_handler();
    delay(20);
}
