// #include <Arduino.h>
// /*
//  * Why FS.h must come before TFT_eSPI / Vì sao FS.h phải đứng trước TFT_eSPI
//  *
//  * EN:
//  *   - TFT_eSPI can pull in FS.h with FS_NO_GLOBALS (e.g. SMOOTH_FONT).
//  *     Then only fs::File exists — no global "File".
//  *   - FS.h is include-guarded; the first include wins for the whole build unit.
//  *   - If that first include was with FS_NO_GLOBALS, ESP32-audioI2S (Audio.h)
//  *     fails: 'File' undeclared.
//  *   - Including FS.h here first applies the normal global aliases before TFT.
//  *
//  * VI:
//  *   - TFT_eSPI có thể include FS.h khi đã bật FS_NO_GLOBALS (vd. SMOOTH_FONT),
//  *     lúc đó chỉ có fs::File, không có tên File ở global.
//  *   - FS.h chỉ được parse một lần (include guard); lần đầu quyết định hết.
//  *   - Nếu lần đầu là với FS_NO_GLOBALS thì thư viện audio (Audio.h) lỗi không
//  *     nhận ra File.
//  *   - Include FS.h ở đây trước để alias global được áp dụng trước khi TFT load.
//  */
// #include <FS.h>
// #include <TFT_eSPI.h>
// #include <lvgl.h>
// #include "config.h"
// #include "touch.h"
// #include "wifi_connector.h"
// #include "time_sync.h"
// #include "micro_sd.h"
// #include "status_bar.h"
// #include "fps.h"
// #include "vnindex.h"

// #define DRAW_BUF_SIZE (TFT_WIDTH * TFT_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
// uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// uint32_t lv_tick_source(void)
// {
//     return millis();
// }

// void lv_touch_read(lv_indev_t *indev, lv_indev_data_t *data)
// {
//     auto t = Touch.get_touch();
//     if (!t.touched)
//     {
//         data->state = LV_INDEV_STATE_RELEASED;
//         return;
//     }

//     data->point.x = t.x;
//     data->point.y = t.y;
//     data->state = LV_INDEV_STATE_PRESSED;
// }

// #if defined(TFT_BL) && (TFT_BL >= 0)
// /** Configure TFT backlight pin (TFT_BL) with LEDC PWM from TFT_BACKLIGHT_PERCENT.
//  *  Cấu hình chân đèn nền TFT (TFT_BL) bằng PWM LEDC theo macro TFT_BACKLIGHT_PERCENT.
//  */
// void setup_tft_backlight_pwm(void)
// {
//     const int bl_pct_in = TFT_BACKLIGHT_PERCENT;
//     unsigned bl_pct;
//     if (bl_pct_in < 0)
//         bl_pct = 0u;
//     else if (bl_pct_in > 100)
//         bl_pct = 100u;
//     else
//         bl_pct = (unsigned)bl_pct_in;

//     if (bl_pct == 0)
//     {
//         digitalWrite(TFT_BL, TFT_BACKLIGHT_ON == HIGH ? LOW : HIGH);
//         return;
//     }

//     if (bl_pct == 100)
//     {
//         digitalWrite(TFT_BL, TFT_BACKLIGHT_ON == HIGH ? HIGH : LOW);
//         return;
//     }

//     // PWM carrier frequency; ~20 kHz is usually well above visible flicker.
//     // Tần số sóng mang PWM; ~20 kHz thường cao hơn ngưỡng nháy mắt thấy được.
//     constexpr uint32_t bl_pwm_hz = 20000;
//     // Duty cycle resolution (bits); 8 → duty range 0 … 255.
//     // Độ phân giải duty (bit); 8 bit → giá trị duty từ 0 … 255.
//     constexpr uint8_t bl_pwm_bits = 8;
//     // Top of the duty range for this resolution.
//     // Giá trị duty tối đa ứng với độ phân giải đã chọn.
//     constexpr uint32_t bl_pwm_max = (1u << bl_pwm_bits) - 1u;

//     // Linear map of percent to raw duty (before optional polarity flip).
//     // Ánh xạ tuyến tính từ % sang duty thô (trước khi đảo cực nếu có).
//     uint32_t bl_duty = (bl_pwm_max * bl_pct) / 100u;
// #if TFT_BACKLIGHT_ON == LOW
//     // Active-low drive: invert duty so “more percent” still means brighter.
//     // Điều khiển active-low: đảo duty để % càng cao vẫn tương ứng càng sáng.
//     bl_duty = bl_pwm_max - bl_duty;
// #endif
//     // LEDC channel index; pick one not used elsewhere in this sketch.
//     // Chỉ số kênh LEDC; chọn kênh chưa dùng ở chỗ khác trong sketch.
//     constexpr uint8_t bl_ledc_channel = 0;
//     // Configure LEDC timer: channel, frequency, resolution.
//     // Cấu hình bộ định thời LEDC: kênh, tần số, độ phân giải.
//     ledcSetup(bl_ledc_channel, bl_pwm_hz, bl_pwm_bits);
//     // Route TFT_BL GPIO to that LEDC channel.
//     // Gán chân GPIO TFT_BL vào kênh LEDC đó.
//     ledcAttachPin(TFT_BL, bl_ledc_channel);
//     // Apply computed duty to the pin.
//     // Ghi duty đã tính ra chân.
//     ledcWrite(bl_ledc_channel, bl_duty);
// }
// #endif

// void setup()
// {
//     Serial.begin(115200);
//     delay(2500); // Allow time for Serial to initialize

//     Serial.println("[Setup] Begin setup");

//     lv_init();
//     lv_tick_set_cb(lv_tick_source);

//     auto disp = lv_tft_espi_create(TFT_WIDTH, TFT_HEIGHT, draw_buf, sizeof(draw_buf));
//     lv_display_set_rotation(disp, TFT_ROTATION);

//     // lv_tft_espi_create() allocates its own TFT_eSPI (see LVGL lv_tft_espi.cpp).
//     // Invert must run on that internal instance.
//     typedef struct
//     {
//         TFT_eSPI *tft;
//     } display_driver_data_t;
//     auto tft_dsc = static_cast<display_driver_data_t *>(lv_display_get_driver_data(disp));
//     if (tft_dsc && tft_dsc->tft)
//     {
//         tft_dsc->tft->invertDisplay(true);
//     }

// #if defined(TFT_BL) && (TFT_BL >= 0)
//     setup_tft_backlight_pwm();
// #endif

//     Touch.setup(TFT_ROTATION);
//     WifiConnector.setup();
//     MicroSD.mount();
//     VnIndex.setup();

//     WifiConnector.set_screen_after_connected(VnIndex.get_screen());

//     StatusBar.setup();

//     // Initialize the (dummy) input device driver
//     auto indev = lv_indev_create();
//     lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
//     lv_indev_set_read_cb(indev, lv_touch_read);

//     Serial.println("[Setup] End setup");
// }

// void loop()
// {
//     const unsigned long start_time = millis();

//     lv_timer_handler();

//     StatusBar.loop();
//     Fps.loop(start_time);
//     WifiConnector.loop();

//     if (WifiConnector.is_connected())
//     {
//         TimeSync.loop();
//         VnIndex.loop();
//     }

//     const unsigned long process_time = millis() - start_time;
//     if (process_time >= 1000 / FPS)
//     {
//         return;
//     }

//     delay(1000 / FPS - process_time);
// }

#include <Arduino.h>
#include <FS.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"
#include "status_bar.h"

#define DRAW_BUF_SIZE (TFT_WIDTH * TFT_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

uint32_t lv_tick_source(void)
{
    return millis();
}

Adafruit_NeoPixel pixels(1, 42, NEO_GRB + NEO_KHZ800);

void LvglTask(void *parameter)
{
    for (;;)
    {
        const unsigned long start_time = millis();

        lv_timer_handler();

        Fps.loop_ui(start_time);
        StatusBar.loop_ui();

        // Serial.printf("BlinkTask running on core: %d\n", xPortGetCoreID());
        // Serial.printf("Task Stack Free: %u bytes\n", uxTaskGetStackHighWaterMark(NULL));

        const unsigned long process_time = millis() - start_time;
        if (process_time >= 1000 / FPS)
            continue;

        const unsigned long delay_ms = 1000 / FPS - process_time;
        vTaskDelay(delay_ms / portTICK_PERIOD_MS);
    }
}

void LoopTask(void *parameter)
{
    for (;;)
    {
        vTaskDelay(1);
    }
}

void setup()
{
    Serial.begin(115200);

    pixels.begin();
    pixels.clear();

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

    StatusBar.setup();

    xTaskCreatePinnedToCore(
        LvglTask,   // Task function
        "LvglTask", // Task name
        5000,       // Stack size (bytes)
        NULL,       // Parameters
        99,         // Priority
        nullptr,    // Task handle
        0           // Core 0
    );

    xTaskCreatePinnedToCore(
        LoopTask,   // Task function
        "LoopTask", // Task name
        10000,      // Stack size (bytes)
        NULL,       // Parameters
        1,          // Priority
        nullptr,    // Task handle
        1           // Core 0
    );
}

void loop()
{
    // Main loop can perform other tasks or remain empty
}
