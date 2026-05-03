#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <lvgl.h>

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>

#include "config.h"
#include "modules/common/helpers.h"
#include "modules/common/iapplication.h"

#define APP_WEATHER_REFRESH_MS (15U * 60U * 1000U)
#define APP_WEATHER_FETCH_STACK 20480U
#define APP_WEATHER_FETCH_PRIORITY 1
#define APP_WEATHER_FETCH_CORE 1
#define APP_WEATHER_HANOI_LAT 21.0285
#define APP_WEATHER_HANOI_LON 105.8542

struct WeatherSnapshot {
    bool valid = false;
    char error[96]{};
    char icon_utf8[8]{};
    char summary[160]{};
    char details[3072]{};
};

class AppWeatherClass : public Application {
public:
    AppWeatherClass() : drawer_icon_utf8(fa(0xf6c4)) {}

    const char* get_drawer_icon_text() override { return drawer_icon_utf8.c_str(); }

    void drawer_icon_clicked() override {
        if (screen) return;

        was_wifi_up = (WiFi.status() == WL_CONNECTED);
        need_refresh = true;
        wifi_hint_applied = false;

        screen = lv_obj_create(nullptr);
        lv_obj_set_layout(screen, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(screen, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_style_pad_all(screen, 8, LV_PART_MAIN);
        lv_obj_set_style_pad_top(screen, STATUS_BAR_HEIGHT + 8, LV_PART_MAIN);
        lv_obj_set_style_pad_row(screen, 6, LV_PART_MAIN);
        lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

        auto row_top = lv_obj_create(screen);
        lv_obj_set_width(row_top, lv_pct(100));
        lv_obj_set_height(row_top, LV_SIZE_CONTENT);
        lv_obj_set_layout(row_top, LV_LAYOUT_FLEX);
        lv_obj_set_flex_flow(row_top, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(row_top, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_set_style_pad_all(row_top, 0, LV_PART_MAIN);
        lv_obj_set_style_border_width(row_top, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(row_top, LV_OPA_TRANSP, LV_PART_MAIN);

        label_main_icon = lv_label_create(row_top);
        lv_obj_set_flex_grow(label_main_icon, 1);
        lv_label_set_long_mode(label_main_icon, LV_LABEL_LONG_MODE_CLIP);
        lv_obj_set_style_text_align(label_main_icon, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_label_set_text(label_main_icon, fa(0xf185).c_str());

        btn_refresh = lv_button_create(row_top);
        lv_obj_set_size(btn_refresh, 44, 44);
        auto lbl_ref = lv_label_create(btn_refresh);
        lv_label_set_text(lbl_ref, fa(0xf2f1).c_str());
        lv_obj_align(lbl_ref, LV_ALIGN_CENTER, 0, 0);
        lv_obj_add_event_cb(btn_refresh, LV_OBJ_EVENT_CB(AppWeatherClass, on_refresh_clicked), LV_EVENT_CLICKED, this);

        label_summary = lv_label_create(screen);
        lv_obj_set_width(label_summary, lv_pct(100));
        lv_label_set_long_mode(label_summary, LV_LABEL_LONG_MODE_WRAP);

        panel_scroll = lv_obj_create(screen);
        lv_obj_set_width(panel_scroll, lv_pct(100));
        lv_obj_set_flex_grow(panel_scroll, 1);
        lv_obj_set_style_pad_all(panel_scroll, 4, LV_PART_MAIN);
        lv_obj_set_style_border_width(panel_scroll, 0, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(panel_scroll, LV_OPA_TRANSP, LV_PART_MAIN);
        lv_obj_add_flag(panel_scroll, LV_OBJ_FLAG_SCROLLABLE);

        label_details = lv_label_create(panel_scroll);
        lv_obj_set_width(label_details, lv_pct(100));
        lv_label_set_long_mode(label_details, LV_LABEL_LONG_MODE_WRAP);
        lv_label_set_text(label_details, "");

        apply_placeholder_ui();
    }

    void app_close() override {
        const uint32_t deadline = millis() + 5000U;
        while (fetch_task_handle != nullptr && millis() < deadline) vTaskDelay(pdMS_TO_TICKS(40));

        if (fetch_task_handle != nullptr) {
            vTaskDelete(fetch_task_handle);
            fetch_task_handle = nullptr;
        }

        if (!screen) return;

        lv_obj_delete(screen);
        screen = nullptr;
        label_main_icon = nullptr;
        label_summary = nullptr;
        label_details = nullptr;
        panel_scroll = nullptr;
        btn_refresh = nullptr;

        {
            const std::lock_guard<std::mutex> lock(mtx);
            snapshot = WeatherSnapshot{};
            ui_dirty = false;
        }

        was_wifi_up = false;
        need_refresh = false;
        wifi_hint_applied = false;
    }

    void loop_ui() override {
        if (!screen) return;

        WeatherSnapshot local{};
        bool dirty = false;
        {
            const std::lock_guard<std::mutex> lock(mtx);
            if (!ui_dirty) return;
            local = snapshot;
            ui_dirty = false;
            dirty = true;
        }

        if (!dirty) return;

        if (!local.valid) {
            lv_label_set_text(label_main_icon, fa(0xf071).c_str());
            lv_label_set_text(label_summary, "Lỗi");
            lv_label_set_text(label_details, local.error[0] ? local.error : "Không có dữ liệu.");
            return;
        }

        lv_label_set_text(label_main_icon, local.icon_utf8);
        lv_label_set_text(label_summary, local.summary);
        lv_label_set_text(label_details, local.details);
    }

    void loop() override {
        if (!screen) return;

        const bool wifi_up = (WiFi.status() == WL_CONNECTED);
        if (wifi_up && !was_wifi_up) need_refresh = true;
        was_wifi_up = wifi_up;

        if (fetch_task_handle != nullptr) return;

        if (!wifi_up) {
            if (!wifi_hint_applied) {
                wifi_hint_applied = true;
                {
                    const std::lock_guard<std::mutex> lock(mtx);
                    snapshot.valid = false;
                    std::snprintf(snapshot.error, sizeof(snapshot.error), "Chưa kết nối WiFi.");
                    ui_dirty = true;
                }
            }
            return;
        }

        wifi_hint_applied = false;

        const uint32_t last_ok = last_fetch_ok_ms.load(std::memory_order_relaxed);
        if (!need_refresh && last_ok != 0U && millis() - last_ok < APP_WEATHER_REFRESH_MS) return;

        need_refresh = false;

        if (xTaskCreatePinnedToCore(
                FREERTOS_TASK_CB(AppWeatherClass, fetch_weather_task),
                "app_weather_fetch",
                APP_WEATHER_FETCH_STACK,
                this,
                APP_WEATHER_FETCH_PRIORITY,
                &fetch_task_handle,
                APP_WEATHER_FETCH_CORE
            ) != pdPASS) {
            fetch_task_handle = nullptr;
            {
                const std::lock_guard<std::mutex> lock(mtx);
                snapshot.valid = false;
                std::snprintf(snapshot.error, sizeof(snapshot.error), "Không tạo được tác vụ tải.");
                ui_dirty = true;
            }
        }
    }

private:
    std::string drawer_icon_utf8;
    std::mutex mtx;
    WeatherSnapshot snapshot{};
    bool ui_dirty = false;
    TaskHandle_t fetch_task_handle = nullptr;
    std::atomic<uint32_t> last_fetch_ok_ms{0U};
    bool need_refresh = false;
    bool was_wifi_up = false;
    bool wifi_hint_applied = false;

    lv_obj_t* label_main_icon = nullptr;
    lv_obj_t* label_summary = nullptr;
    lv_obj_t* label_details = nullptr;
    lv_obj_t* panel_scroll = nullptr;
    lv_obj_t* btn_refresh = nullptr;

    void apply_placeholder_ui() {
        lv_label_set_text(label_summary, "Hà Nội - đang tải...");
        lv_label_set_text(label_details, "");
    }

    void on_refresh_clicked() { need_refresh = true; }

    static const char* wind_dir_vi8(int idx8) {
        static const char* names[] = {"Bắc", "Đông Bắc", "Đông", "Đông Nam", "Nam", "Tây Nam", "Tây", "Tây Bắc"};
        if (idx8 < 0 || idx8 > 7) return "?";
        return names[idx8];
    }

    static int wind_sector_from_deg(float deg) {
        auto d = std::fmod(deg + 360.0f, 360.0f);
        const int idx = static_cast<int>((d + 22.5f) / 45.0f) % 8;
        return idx;
    }

    static const char* weather_code_vi(int code) {
        if (code == 0) return "Trời quang";
        if (code == 1) return "Chủ yếu quang";
        if (code == 2) return "Có mây, ít nắng";
        if (code == 3) return "Nhiều mây u ám";
        if (code == 45 || code == 48) return "Sương mù / sương đọng";
        if (code >= 51 && code <= 55) return "Mưa phùn";
        if (code >= 56 && code <= 57) return "Mưa phùn đóng băng";
        if (code >= 61 && code <= 65) return "Mưa";
        if (code >= 66 && code <= 67) return "Mưa đóng băng";
        if (code >= 71 && code <= 77) return "Tuyết / hạt tuyết";
        if (code == 80 || code == 81 || code == 82) return "Mưa rào";
        if (code >= 85 && code <= 86) return "Mưa tuyết";
        if (code == 95) return "Dông kèm sét";
        if (code == 96) return "Dông sét, mưa đá nhẹ";
        if (code == 99) return "Dông sét, mưa đá nặng";
        return "Thời tiết";
    }

    static std::string icon_for_conditions(int code, float rain_mm, float showers_mm, float cloud_pct) {
        if (code >= 95) return fa(0xf0e7);
        if (code >= 71) return fa(0xf2dc);
        if (code >= 51 || rain_mm > 0.05f || showers_mm > 0.05f) return fa(0xf73d);
        if (code >= 45) return fa(0xf75f);
        if (code == 3 || cloud_pct >= 75.0f) return fa(0xf0c2);
        if (code == 2 || cloud_pct >= 35.0f) return fa(0xf6c4);
        if (code == 1) return fa(0xf6c4);
        return fa(0xf185);
    }

    static const char* european_aqi_band_vi(int aqi) {
        if (aqi < 0) return "—";
        if (aqi <= 20) return "Tốt";
        if (aqi <= 40) return "Khá";
        if (aqi <= 60) return "Trung bình";
        if (aqi <= 80) return "Kém";
        if (aqi <= 100) return "Rất kém";
        return "Cực kém";
    }

    static const char* us_aqi_band_vi(int aqi) {
        if (aqi < 0) return "—";
        if (aqi <= 50) return "Tốt";
        if (aqi <= 100) return "Trung bình";
        if (aqi <= 150) return "Kém (nhóm nhạy cảm)";
        if (aqi <= 200) return "Kém";
        if (aqi <= 300) return "Rất kém";
        return "Nguy hiểm";
    }

    static void format_float_or_dash(float v, char* out, size_t out_sz) {
        if (std::isnan(v)) {
            std::snprintf(out, out_sz, "—");
            return;
        }
        std::snprintf(out, out_sz, "%.1f", static_cast<double>(v));
    }

    static void append_air_quality_to_details(char* details, size_t details_cap, WiFiClientSecure& client) {
        const char* aq_url =
            "https://air-quality-api.open-meteo.com/v1/air-quality?"
            "latitude=21.0285&longitude=105.8542"
            "&timezone=Asia%2FHo_Chi_Minh"
            "&current=european_aqi,us_aqi,pm10,pm2_5,carbon_monoxide,nitrogen_dioxide,sulphur_dioxide,ozone";

        const size_t off0 = std::strlen(details);
        if (off0 >= details_cap) return;

        const String aq_body = http_get(client, String(aq_url));
        if (aq_body.length() == 0) {
            std::snprintf(details + off0, details_cap - off0, "\n\n(Không tải được chỉ số chất lượng không khí.)");
            return;
        }

        JsonDocument aq_doc;
        const DeserializationError aq_err = deserializeJson(aq_doc, aq_body);
        if (aq_err) {
            std::snprintf(details + off0, details_cap - off0, "\n\n(Lỗi JSON không khí: %s)", aq_err.c_str());
            return;
        }

        JsonObject aq_cur = aq_doc["current"];
        if (aq_cur.isNull()) {
            std::snprintf(details + off0, details_cap - off0, "\n\n(Thiếu trường current không khí.)");
            return;
        }

        const int eu_aqi = aq_cur["european_aqi"].isNull() ? -1 : aq_cur["european_aqi"].as<int>();
        const int us_aqi = aq_cur["us_aqi"].isNull() ? -1 : aq_cur["us_aqi"].as<int>();
        const float pm10 = aq_cur["pm10"].isNull() ? NAN : aq_cur["pm10"].as<float>();
        const float pm25 = aq_cur["pm2_5"].isNull() ? NAN : aq_cur["pm2_5"].as<float>();
        const float co = aq_cur["carbon_monoxide"].isNull() ? NAN : aq_cur["carbon_monoxide"].as<float>();
        const float no2 = aq_cur["nitrogen_dioxide"].isNull() ? NAN : aq_cur["nitrogen_dioxide"].as<float>();
        const float so2 = aq_cur["sulphur_dioxide"].isNull() ? NAN : aq_cur["sulphur_dioxide"].as<float>();
        const float o3 = aq_cur["ozone"].isNull() ? NAN : aq_cur["ozone"].as<float>();

        char s_pm10[24]{};
        char s_pm25[24]{};
        char s_co[24]{};
        char s_no2[24]{};
        char s_so2[24]{};
        char s_o3[24]{};
        format_float_or_dash(pm10, s_pm10, sizeof(s_pm10));
        format_float_or_dash(pm25, s_pm25, sizeof(s_pm25));
        format_float_or_dash(co, s_co, sizeof(s_co));
        format_float_or_dash(no2, s_no2, sizeof(s_no2));
        format_float_or_dash(so2, s_so2, sizeof(s_so2));
        format_float_or_dash(o3, s_o3, sizeof(s_o3));

        char eu_num[16]{};
        char us_num[16]{};
        if (eu_aqi >= 0)
            std::snprintf(eu_num, sizeof(eu_num), "%d", eu_aqi);
        else
            std::snprintf(eu_num, sizeof(eu_num), "—");

        if (us_aqi >= 0)
            std::snprintf(us_num, sizeof(us_num), "%d", us_aqi);
        else
            std::snprintf(us_num, sizeof(us_num), "—");

        std::snprintf(
            details + off0,
            details_cap - off0,
            "\n\n--- Chất lượng không khí ---\n"
            "AQI châu Âu: %s (%s)\n"
            "AQI Mỹ: %s (%s)\n"
            "PM10: %s µg/m³\n"
            "PM2.5: %s µg/m³\n"
            "CO: %s µg/m³\n"
            "NO2: %s µg/m³\n"
            "SO2: %s µg/m³\n"
            "O3: %s µg/m³",
            eu_num,
            eu_aqi < 0 ? "—" : european_aqi_band_vi(eu_aqi),
            us_num,
            us_aqi < 0 ? "—" : us_aqi_band_vi(us_aqi),
            s_pm10,
            s_pm25,
            s_co,
            s_no2,
            s_so2,
            s_o3
        );
    }

    void fetch_weather_task() {
        // `WeatherSnapshot` (~3.3 KiB) + TLS/JSON dễ vượt stack canary nếu để trên stack task 8 KiB.
        static WeatherSnapshot next_storage;
        next_storage = WeatherSnapshot{};
        WeatherSnapshot& next = next_storage;

        WiFiClientSecure client;
        client.setInsecure();

        const char* url =
            "https://api.open-meteo.com/v1/forecast?"
            "latitude=21.0285&longitude=105.8542"
            "&timezone=Asia%2FHo_Chi_Minh"
            "&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,"
            "precipitation,rain,showers,snowfall,weather_code,cloud_cover,"
            "pressure_msl,surface_pressure,wind_speed_10m,wind_direction_10m,wind_gusts_10m"
            "&wind_speed_unit=kmh";

        const String body = http_get(client, String(url));

        if (body.length() == 0) {
            next.valid = false;
            std::snprintf(next.error, sizeof(next.error), "Lỗi mạng (không tải được dữ liệu).");
        } else {
            JsonDocument doc;
            const DeserializationError err = deserializeJson(doc, body);
            if (err) {
                next.valid = false;
                std::snprintf(next.error, sizeof(next.error), "Lỗi JSON: %s", err.c_str());
            } else {
                JsonObject cur = doc["current"];
                if (cur.isNull()) {
                    next.valid = false;
                    std::snprintf(next.error, sizeof(next.error), "Thiếu trường current.");
                } else {
                    const float temp = cur["temperature_2m"].as<float>();
                    const float apparent = cur["apparent_temperature"].as<float>();
                    const int humidity = cur["relative_humidity_2m"].as<int>();
                    const int is_day = cur["is_day"].as<int>();
                    const float precip = cur["precipitation"].as<float>();
                    const float rain = cur["rain"].as<float>();
                    const float showers = cur["showers"].as<float>();
                    const float snow = cur["snowfall"].as<float>();
                    const int wcode = cur["weather_code"].as<int>();
                    const float cloud = cur["cloud_cover"].as<float>();
                    const float p_msl = cur["pressure_msl"].as<float>();
                    const float p_surf = cur["surface_pressure"].as<float>();
                    const float wind_kmh = cur["wind_speed_10m"].as<float>();
                    const float wind_deg = cur["wind_direction_10m"].as<float>();
                    const float gust_kmh = cur["wind_gusts_10m"].as<float>();

                    const auto icon_str = icon_for_conditions(wcode, rain + showers, showers, cloud);
                    std::strncpy(next.icon_utf8, icon_str.c_str(), sizeof(next.icon_utf8) - 1U);
                    next.icon_utf8[sizeof(next.icon_utf8) - 1U] = '\0';

                    const int sec = wind_sector_from_deg(wind_deg);
                    const char* dir_vi = wind_dir_vi8(sec);

                    std::snprintf(next.summary, sizeof(next.summary), "Hà Nội - %s", weather_code_vi(wcode));

                    std::snprintf(
                        next.details,
                        sizeof(next.details),
                        "Mã thời tiết WMO: %d\n"
                        "%s\n\n"
                        "Nhiệt độ: %.1f °C\n"
                        "Cảm nhận: %.1f °C\n"
                        "Độ ẩm: %d %%\n"
                        "Mây che phủ: %.0f %%\n"
                        "Ngày / đêm: %s\n\n"
                        "Lượng mưa (tổng): %.1f mm\n"
                        "Mưa: %.1f mm | Mưa rào: %.1f mm\n"
                        "Tuyết: %.1f cm\n\n"
                        "Gió: %.1f km/h\n"
                        "Hướng: %.0f° (%s)\n"
                        "Giật gió: %.1f km/h\n\n"
                        "Áp suất (MSL): %.0f hPa\n"
                        "Áp suất mặt đất: %.0f hPa",
                        wcode,
                        weather_code_vi(wcode),
                        static_cast<double>(temp),
                        static_cast<double>(apparent),
                        humidity,
                        static_cast<double>(cloud),
                        is_day ? "Ngày" : "Đêm",
                        static_cast<double>(precip),
                        static_cast<double>(rain),
                        static_cast<double>(showers),
                        static_cast<double>(snow),
                        static_cast<double>(wind_kmh),
                        static_cast<double>(wind_deg),
                        dir_vi,
                        static_cast<double>(gust_kmh),
                        static_cast<double>(p_msl),
                        static_cast<double>(p_surf)
                    );

                    append_air_quality_to_details(next.details, sizeof(next.details), client);

                    next.valid = true;
                }
            }
        }

        {
            const std::lock_guard<std::mutex> lock(mtx);
            snapshot = next;
            ui_dirty = true;
        }

        if (next.valid) last_fetch_ok_ms.store(millis(), std::memory_order_relaxed);

        fetch_task_handle = nullptr;
        vTaskDelete(nullptr);
    }
};

extern AppWeatherClass AppWeather;
