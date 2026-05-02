#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <lvgl.h>

#include <cstring>

#include "applications/rainbow/app_rainbow.h"
#include "applications/wifi/app_wifi.h"
#include "config.h"
#include "modules/app_management/app_management.h"
#include "modules/fps/fps.h"
#include "modules/home/home.h"
#include "modules/keyboard/keyboard.h"
#include "modules/led/led.h"
#include "modules/micro_sd/micro_sd.h"
#include "modules/status_bar/status_bar.h"
#include "modules/touch/touch.h"

// Register applications in the app management system here
void register_apps() {
    AppManagement.register_app(AppRainbow);
    AppManagement.register_app(AppWifi);
}

#define DRAW_BUF_SIZE (TFT_WIDTH * TFT_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

uint32_t lv_tick_source(void) { return millis(); }

// FPS = số frame thực sự flush xong (partial buffer: chỉ đếm khi flush cuối của một chu kỳ vẽ).
static void fps_on_display_flush_finish(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_FLUSH_FINISH) return;

    auto disp = static_cast<lv_display_t*>(lv_event_get_target(e));
    if (!disp || !lv_display_flush_is_last(disp)) return;

    Fps.notify_frame_flushed();
}

void lv_touch_read(lv_indev_t* indev, lv_indev_data_t* data) {
    auto t = Touch.get_touch();
    if (!t.touched) {
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    data->point.x = t.x;
    data->point.y = t.y;
    data->state = LV_INDEV_STATE_PRESSED;
}

void setup_tft_backlight_pwm(void) {
    auto bl_pct_in = TFT_BACKLIGHT_PERCENT;
    unsigned bl_pct;
    if (bl_pct_in < 0)
        bl_pct = 0u;
    else if (bl_pct_in > 100)
        bl_pct = 100u;
    else
        bl_pct = (unsigned)bl_pct_in;

    if (bl_pct == 0) {
        digitalWrite(TFT_BL, TFT_BACKLIGHT_ON == HIGH ? LOW : HIGH);
        return;
    }

    if (bl_pct == 100) {
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
    auto bl_duty = (bl_pwm_max * bl_pct) / 100u;
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

void lvgl_task(void* parameter) {
    // Lịch chạy adaptive bám theo `lv_timer_handler()`:
    // - Khi UI tĩnh, LVGL trả `LV_NO_TIMER_READY` (rất lớn) → task ngủ tới `LVGL_MAX_IDLE_MS`,
    //   CPU Core 0 gần như rảnh, giảm nhiệt rõ rệt.
    // - Khi có animation/dirty, LVGL trả khoảng `LV_DEF_REFR_PERIOD` ms (33ms ~ 30 FPS) →
    //   task ngủ đúng tới lần render kế. Không busy-wait, không deadline cứng.
    while (true) {
        Fps.loop_ui();  // gom số frame flush theo từng giây (đếm thực tế ở LV_EVENT_FLUSH_FINISH)
        StatusBar.loop_ui();
        AppManagement.loop_ui();
        Home.loop_ui();

        auto next_ms = lv_timer_handler();

        if (next_ms < LVGL_MIN_PERIOD_MS) next_ms = LVGL_MIN_PERIOD_MS;
        if (next_ms > LVGL_MAX_IDLE_MS) next_ms = LVGL_MAX_IDLE_MS;

        vTaskDelay(pdMS_TO_TICKS(next_ms));
    }
}

void loop_task(void* parameter) {
    while (true) {
        TimeSync.loop();
        AppManagement.loop();
        vTaskDelay(pdMS_TO_TICKS(250));
    }
}

void wifi_setup() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(false);

    String ssid_to_use;
    String pass_to_use;

    if (MicroSD.is_mounted()) {
        auto f = MicroSD.fs().open(APP_WIFI_JSON_PATH, FILE_READ);
        if (f) {
            auto doc = JsonDocument();
            auto err = deserializeJson(doc, f);
            f.close();

            if (!err) {
                auto connected = doc["connected"].as<const char*>();
                if (connected && connected[0]) {
                    ssid_to_use = connected;
                    auto stored = doc["stored"].as<JsonArray>();
                    if (!stored.isNull()) {
                        for (auto v : stored) {
                            auto o = v.as<JsonObject>();
                            if (o.isNull()) continue;

                            auto s = o["ssid"].as<const char*>();
                            if (!s || std::strcmp(s, connected) != 0) continue;

                            auto p = o["passpharse"].as<const char*>();
                            pass_to_use = p ? p : "";
                            break;
                        }
                    }
                }
            }
        }
    }

    if (ssid_to_use.length() == 0) return;

    WiFi.persistent(false);
    WiFi.begin(ssid_to_use.c_str(), pass_to_use.c_str());
}

void setup() {
    Serial.begin(115200);
    delay(2500);  // Allow time for Serial to initialize

    Serial.println("[Setup] Begin setup");

    lv_init();
    lv_tick_set_cb(lv_tick_source);

    auto disp = lv_tft_espi_create(TFT_WIDTH, TFT_HEIGHT, draw_buf, sizeof(draw_buf));
    lv_display_set_rotation(disp, TFT_ROTATION);
    lv_display_add_event_cb(disp, fps_on_display_flush_finish, LV_EVENT_FLUSH_FINISH, nullptr);

    auto theme = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_PURPLE), true, &font_custom_merged);
    lv_display_set_theme(disp, theme);

    // Initialize the (dummy) input device driver
    auto indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lv_touch_read);

    // lv_tft_espi_create() allocates its own TFT_eSPI (see LVGL lv_tft_espi.cpp).
    // Invert must run on that internal instance.
    typedef struct {
        TFT_eSPI* tft;
    } display_driver_data_t;

    auto tft_dsc = static_cast<display_driver_data_t*>(lv_display_get_driver_data(disp));
    if (tft_dsc && tft_dsc->tft) {
        tft_dsc->tft->invertDisplay(true);
        tft_dsc->tft->fillScreen(TFT_BLACK);
    }

    setup_tft_backlight_pwm();

    Touch.setup();
    Fps.setup();
    Led.setup();
    MicroSD.setup();
    StatusBar.setup();

    wifi_setup();

    Keyboard.setup();

    AppManagement.setup();
    register_apps();

    Home.setup();
    lv_scr_load(Home.get_screen());

    AppManagement.set_home_screen(Home.get_screen());

    xTaskCreatePinnedToCore(
        lvgl_task,             // Task function
        "lvgl_task",           // Task name
        10000,                 // Stack size (bytes)
        NULL,                  // Parameters
        configMAX_PRIORITIES,  // Priority
        nullptr,               // Task handle
        0                      // Core 0
    );

    xTaskCreatePinnedToCore(
        loop_task,    // Task function
        "loop_task",  // Task name
        10000,        // Stack size (bytes)
        NULL,         // Parameters
        1,            // Priority
        nullptr,      // Task handle
        1             // Core 0
    );

    Serial.println("[Setup] End setup");
}

void loop() {
    // Main loop can perform other tasks or remain empty
}
