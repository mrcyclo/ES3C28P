#include <Arduino.h>
#include <FS.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include <Adafruit_NeoPixel.h>
#include <esp_timer.h>
#include "config.h"
#include "status_bar/status_bar.h"
#include "led/led.h"
#include "touch/touch.h"
#include "micro_sd/micro_sd.h"
#include "home/home.h"

#define DRAW_BUF_SIZE (TFT_WIDTH * TFT_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

uint32_t lv_tick_source(void)
{
    return millis();
}

void lv_touch_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    auto t = Touch.get_touch();
    if (!t.touched)
    {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    data->point.x = t.x;
    data->point.y = t.y;
    data->state = LV_INDEV_STATE_PRESSED;
}

void setup_tft_backlight_pwm(void)
{
    const int bl_pct_in = TFT_BACKLIGHT_PERCENT;
    unsigned bl_pct;
    if (bl_pct_in < 0)
        bl_pct = 0u;
    else if (bl_pct_in > 100)
        bl_pct = 100u;
    else
        bl_pct = (unsigned)bl_pct_in;

    if (bl_pct == 0)
    {
        digitalWrite(TFT_BL, TFT_BACKLIGHT_ON == HIGH ? LOW : HIGH);
        return;
    }

    if (bl_pct == 100)
    {
        digitalWrite(TFT_BL, TFT_BACKLIGHT_ON == HIGH ? HIGH : LOW);
        return;
    }

    // PWM carrier frequency; ~20 kHz is usually well above visible flicker.
    // Tần số sóng mang PWM; ~20 kHz thường cao hơn ngưỡng nháy mắt thấy được.
    constexpr uint32_t bl_pwm_hz = 20000;
    // Duty cycle resolution (bits); 8 → duty range 0 … 255.
    // Độ phân giải duty (bit); 8 bit → giá trị duty từ 0 … 255.
    constexpr uint8_t bl_pwm_bits = 8;
    // Top of the duty range for this resolution.
    // Giá trị duty tối đa ứng với độ phân giải đã chọn.
    constexpr uint32_t bl_pwm_max = (1u << bl_pwm_bits) - 1u;

    // Linear map of percent to raw duty (before optional polarity flip).
    // Ánh xạ tuyến tính từ % sang duty thô (trước khi đảo cực nếu có).
    uint32_t bl_duty = (bl_pwm_max * bl_pct) / 100u;
#if TFT_BACKLIGHT_ON == LOW
    // Active-low drive: invert duty so “more percent” still means brighter.
    // Điều khiển active-low: đảo duty để % càng cao vẫn tương ứng càng sáng.
    bl_duty = bl_pwm_max - bl_duty;
#endif
    // LEDC channel index; pick one not used elsewhere in this sketch.
    // Chỉ số kênh LEDC; chọn kênh chưa dùng ở chỗ khác trong sketch.
    constexpr uint8_t bl_ledc_channel = 0;
    // Configure LEDC timer: channel, frequency, resolution.
    // Cấu hình bộ định thời LEDC: kênh, tần số, độ phân giải.
    ledcSetup(bl_ledc_channel, bl_pwm_hz, bl_pwm_bits);
    // Route TFT_BL GPIO to that LEDC channel.
    // Gán chân GPIO TFT_BL vào kênh LEDC đó.
    ledcAttachPin(TFT_BL, bl_ledc_channel);
    // Apply computed duty to the pin.
    // Ghi duty đã tính ra chân.
    ledcWrite(bl_ledc_channel, bl_duty);
}

void LvglTask(void *parameter)
{
    // Bộ canh nhịp khung hình theo microsecond để FPS ra đúng và ổn định.
    //
    // Lý do không dùng `millis()` + `1000 / FPS`:
    // - `1000 / 60` bị chia số nguyên => 16ms (thực tế cần 16.666...ms) nên FPS sẽ bị lệch (thường ~62.5 hoặc cao hơn).
    // - `millis()` có độ phân giải 1ms nên càng làm sai số tích lũy rõ hơn.
    //
    // Cách làm ở đây:
    // - Mỗi frame tăng deadline theo microsecond.
    // - Vì `1,000,000 / FPS` cũng bị chia số nguyên, ta bù phần dư để trung bình đúng 60 FPS.
    // Thời gian 1 giây = 1_000_000 μs. Chu kỳ 1 frame (lý tưởng) = 1_000_000 / FPS μs.
    // Chia số nguyên sẽ có phần nguyên + phần dư; ta dùng cả hai để trung bình đúng FPS.
    const uint32_t frame_period_us = 1000000UL / FPS;         // Phần nguyên mỗi frame (vd FPS=60 → 16666 μs)
    const uint32_t remainder_us_per_second = 1000000UL % FPS; // Phần dư còn thiếu trong 1 giây (vd 40 μs)
    uint32_t remainder_accumulator = 0;
    int64_t next_frame_deadline_us = esp_timer_get_time();

    while (true)
    {
        Fps.loop_ui();
        StatusBar.loop_ui();
        Home.loop_ui();

        lv_timer_handler();

        // Serial.printf("Task Stack Free: %u bytes\n", uxTaskGetStackHighWaterMark(NULL));

        // --- Cập nhật mốc thời gian frame kế (next_frame_deadline_us) + bù phần dư chia số nguyên ---
        //
        // Ví dụ FPS = 60: chu kỳ lý tưởng = 1_000_000 / 60 = 16666.666... μs.
        // Máy chỉ cộng số nguyên được, nên ta lấy:
        //   frame_period_us = 16666, remainder_us_per_second = 40.
        //
        // Nếu mỗi frame chỉ cộng 16666 μs thì sau 60 frame ta có 60 × 16666 = 999_960 μs,
        // thiếu 40 μs so với đúng 1 giây → FPS thực tế sẽ hơi cao hơn 60 nếu không bù.
        //
        // Cách bù: mỗi frame cộng remainder_us_per_second vào remainder_accumulator.
        // Khi accumulator >= FPS: deadline nhận thêm 1 μs (bù một phần của tổng dư trong giây),
        // rồi trừ accumulator đi FPS để tiếp tục tích lũy cho các lần bù sau (không bù trùng).
        //
        // Với FPS=60, dư 40: trong đúng 60 frame liên tiếp, thuật toán sẽ chèn tổng cộng 40 lần +1 μs
        // (xen kẽ giữa các frame chỉ tăng frame_period_us), nên tổng thời gian 60 frame = 1_000_000 μs
        // → trung bình đúng 60 FPS.
        next_frame_deadline_us += frame_period_us;
        remainder_accumulator += remainder_us_per_second;
        if (remainder_accumulator >= (uint32_t)FPS)
        {
            next_frame_deadline_us += 1;
            remainder_accumulator -= (uint32_t)FPS;
        }

        // --- Chờ đến đúng mốc next_frame_deadline_us (đồng hồ esp_timer, đơn vị μs) ---
        //
        // Mục tiêu: sau khi xử lý xong (LVGL + status bar...), task ngủ cho tới đúng thời điểm
        // bắt đầu frame kế, để chu kỳ trung bình bám sát FPS đã tính ở trên.
        //
        // now_us: “bây giờ” theo microsecond. So sánh với deadline để biết còn phải chờ bao lâu.
        int64_t now_us = esp_timer_get_time();
        if (now_us < next_frame_deadline_us)
        {
            // Khoảng còn lại tới deadline (luôn > 0 trong nhánh này).
            uint32_t wait_us = (uint32_t)(next_frame_deadline_us - now_us);

            // Chờ thô bằng vTaskDelay:
            // - FreeRTOS chỉ “ngủ” theo bội số tick (thường 1 tick ≈ 1ms), không chờ chính xác từng μs.
            // - Nếu gọi vTaskDelay sát deadline, tick làm tròn có thể ngủ **quá lâu** và vượt deadline.
            // Vì vậy chỉ dùng vTaskDelay khi còn khá nhiều thời gian (>= 2000 μs), và chỉ ngủ **ít hơn**
            // khoảng cần thiết một chút: chuyển (wait_us - 1000) / 1000 → ms, để chừa ~1ms cho bước sau.
            if (wait_us >= 2000)
            {
                const uint32_t wait_ms = (wait_us - 1000) / 1000;
                vTaskDelay(pdMS_TO_TICKS(wait_ms));
            }

            // Chờ tinh (busy-wait):
            // - Vòng while liên tục đọc esp_timer_get_time() cho tới deadline.
            // - Tốn CPU trong khoảng thời gian rất ngắn (thường < ~1ms sau bước chờ thô), nhưng giúp
            //   chạm mốc μs chính xác hơn nhiều so với chỉ dùng vTaskDelay.
            while (esp_timer_get_time() < next_frame_deadline_us)
            {
            }
        }
        else
        {
            // Đã trễ: xử lý mất nhiều thời gian hơn một chu kỳ frame (now_us >= deadline).
            // Nếu vẫn giữ deadline cũ và tiếp tục cộng chu kỳ, các deadline sẽ nằm **trong quá khứ**
            // liên tục → vòng lặp hầu như không chờ được, số FPS đo được không còn ổn định.
            // Resync: coi “mốc hiện tại” là now_us và bắt đầu lại lịch từ đây (bỏ nợ các frame đã lỡ).
            next_frame_deadline_us = now_us;
        }
    }
}

void LoopTask(void *parameter)
{
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void setup()
{
    Serial.begin(115200);
    delay(2500); // Allow time for Serial to initialize

    Serial.println("[Setup] Begin setup");

    lv_init();
    lv_tick_set_cb(lv_tick_source);

    auto disp = lv_tft_espi_create(TFT_WIDTH, TFT_HEIGHT, draw_buf, sizeof(draw_buf));
    lv_display_set_rotation(disp, TFT_ROTATION);

    lv_theme_t *theme = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_PURPLE), true, &font_custom_merged);
    lv_display_set_theme(disp, theme);

    // Initialize the (dummy) input device driver
    auto indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lv_touch_read);

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

    setup_tft_backlight_pwm();

    Touch.setup(TFT_ROTATION);
    StatusBar.setup();
    Led.setup();

    MicroSD.mount();

    Home.setup();
    lv_scr_load(Home.get_screen());

    xTaskCreatePinnedToCore(
        LvglTask,             // Task function
        "LvglTask",           // Task name
        10000,                // Stack size (bytes)
        NULL,                 // Parameters
        configMAX_PRIORITIES, // Priority
        nullptr,              // Task handle
        0                     // Core 0
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

    Serial.println("[Setup] End setup");
}

void loop()
{
    // Main loop can perform other tasks or remain empty
}
