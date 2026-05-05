#pragma once

#include <Arduino.h>
#include <Audio.h>
#include <lvgl.h>

#include <string>
#include <vector>

#include "config.h"
#include "es8311.h"
#include "modules/common/helpers.h"
#include "modules/common/iapplication.h"
#include "modules/micro_sd/micro_sd.h"
#include "modules/msgbox/msgbox.h"

// BCLK=5, LRC=7, DOUT=8 (khong phai 6 — 6 la DIN mic), MCLK=4
#define APP_MP3_PLAYER_I2S_BCLK 5
#define APP_MP3_PLAYER_I2S_WS 7
#define APP_MP3_PLAYER_I2S_DOUT 8
#define APP_MP3_PLAYER_I2S_MCLK 4

#define APP_MP3_PLAYER_I2C_SDA 16
#define APP_MP3_PLAYER_I2C_SCL 15

#define APP_MP3_PLAYER_PA_ENABLE_GPIO 1
#define APP_MP3_PLAYER_PA_ACTIVE_LOW 1

#define APP_MP3_PLAYER_PUMP_TASK_STACK 5000
#define APP_MP3_PLAYER_PUMP_TASK_PRIORITY configMAX_PRIORITIES - 2
#define APP_MP3_PLAYER_PUMP_TASK_CORE 0
#define APP_MP3_PLAYER_PLAYING_TASK_STACK 10000
#define APP_MP3_PLAYER_PLAYING_TASK_PRIORITY 1
#define APP_MP3_PLAYER_PLAYING_TASK_CORE 1

class AppMp3PlayerClass : public Application {
public:
    AppMp3PlayerClass() {
        drawer_icon_utf8 = fa(0xf001);
        volume = audio.maxVolume();
        volume_steps = audio.maxVolume() / 10;
    }

    const char* get_drawer_icon_text() override { return drawer_icon_utf8.c_str(); }

    void drawer_icon_clicked() override {
        if (screen) return;

        screen = lv_obj_create(nullptr);

        static int32_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        static int32_t row_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
        auto lv_container = lv_obj_create(screen);
        lv_obj_set_layout(lv_container, LV_LAYOUT_GRID);
        lv_obj_align(lv_container, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_size(lv_container, lv_pct(100), lv_pct(100));
        lv_obj_set_grid_dsc_array(lv_container, col_dsc, row_dsc);
        lv_obj_set_style_bg_opa(lv_container, LV_OPA_0, LV_PART_MAIN);
        lv_obj_set_style_border_width(lv_container, 0, LV_PART_MAIN);
        lv_obj_remove_flag(lv_container, LV_OBJ_FLAG_SCROLLABLE);

        auto lv_list = lv_list_create(lv_container);
        lv_obj_set_grid_cell(lv_list, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 0, 1);
        lv_obj_set_style_pad_all(lv_list, 5, LV_PART_MAIN);

        auto lv_btn_container = lv_obj_create(lv_container);
        lv_obj_set_grid_cell(lv_btn_container, LV_GRID_ALIGN_STRETCH, 0, 1, LV_GRID_ALIGN_STRETCH, 1, 1);
        lv_obj_set_style_bg_opa(lv_btn_container, LV_OPA_0, LV_PART_MAIN);
        lv_obj_set_style_border_width(lv_btn_container, 0, LV_PART_MAIN);
        lv_obj_set_style_pad_all(lv_btn_container, 0, LV_PART_MAIN);
        lv_obj_remove_flag(lv_btn_container, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_remove_flag(lv_btn_container, LV_OBJ_FLAG_HIDDEN);

        auto lv_button_bg = lv_obj_create(lv_btn_container);
        lv_obj_align(lv_button_bg, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_size(lv_button_bg, 140, 140);
        lv_obj_set_style_bg_color(lv_button_bg, LV_COLOR_INFO, LV_PART_MAIN);
        lv_obj_set_style_radius(lv_button_bg, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_pad_all(lv_button_bg, 0, LV_PART_MAIN);

        auto lv_btn_play = lv_button_create(lv_button_bg);
        lv_obj_align(lv_btn_play, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_size(lv_btn_play, lv_pct(50), lv_pct(50));
        lv_obj_set_style_radius(lv_btn_play, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_add_event_cb(lv_btn_play, LV_OBJ_EVENT_CB(AppMp3PlayerClass, on_btn_play_clicked), LV_EVENT_CLICKED, this);

        lv_label_play = lv_label_create(lv_btn_play);
        lv_obj_align(lv_label_play, LV_ALIGN_CENTER, 0, 0);
        auto lv_label_play_text = fa(0xf04b);
        lv_label_set_text(lv_label_play, lv_label_play_text.c_str());

        auto lv_btn_next = lv_button_create(lv_button_bg);
        lv_obj_align(lv_btn_next, LV_ALIGN_RIGHT_MID, 0, 0);
        lv_obj_set_size(lv_btn_next, lv_pct(25), lv_pct(25));
        lv_obj_set_style_radius(lv_btn_next, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lv_btn_next, LV_OPA_0, LV_PART_MAIN);
        lv_obj_add_event_cb(lv_btn_next, LV_OBJ_EVENT_CB(AppMp3PlayerClass, on_btn_next_clicked), LV_EVENT_CLICKED, this);
        lv_obj_add_event_cb(lv_btn_next, LV_OBJ_EVENT_CB(AppMp3PlayerClass, on_btn_next_long_pressed), LV_EVENT_LONG_PRESSED, this);

        auto lv_label_next = lv_label_create(lv_btn_next);
        lv_obj_align(lv_label_next, LV_ALIGN_CENTER, 0, 0);
        auto lv_label_next_text = fa(0xf050);
        lv_label_set_text(lv_label_next, lv_label_next_text.c_str());

        auto lv_btn_prev = lv_button_create(lv_button_bg);
        lv_obj_align(lv_btn_prev, LV_ALIGN_LEFT_MID, 0, 0);
        lv_obj_set_size(lv_btn_prev, lv_pct(25), lv_pct(25));
        lv_obj_set_style_radius(lv_btn_prev, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lv_btn_prev, LV_OPA_0, LV_PART_MAIN);
        lv_obj_add_event_cb(lv_btn_prev, LV_OBJ_EVENT_CB(AppMp3PlayerClass, on_btn_prev_clicked), LV_EVENT_CLICKED, this);
        lv_obj_add_event_cb(lv_btn_prev, LV_OBJ_EVENT_CB(AppMp3PlayerClass, on_btn_prev_long_pressed), LV_EVENT_LONG_PRESSED, this);

        auto lv_label_prev = lv_label_create(lv_btn_prev);
        lv_obj_align(lv_label_prev, LV_ALIGN_CENTER, 0, 0);
        auto lv_label_prev_text = fa(0xf049);
        lv_label_set_text(lv_label_prev, lv_label_prev_text.c_str());

        auto lv_btn_vol_up = lv_button_create(lv_button_bg);
        lv_obj_align(lv_btn_vol_up, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_size(lv_btn_vol_up, lv_pct(25), lv_pct(25));
        lv_obj_set_style_radius(lv_btn_vol_up, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lv_btn_vol_up, LV_OPA_0, LV_PART_MAIN);
        lv_obj_add_event_cb(lv_btn_vol_up, LV_OBJ_EVENT_CB(AppMp3PlayerClass, on_btn_vol_up_clicked), LV_EVENT_CLICKED, this);

        auto lv_label_vol_up = lv_label_create(lv_btn_vol_up);
        lv_obj_align(lv_label_vol_up, LV_ALIGN_CENTER, 0, 0);
        auto lv_label_vol_up_text = fa(0xf028);
        lv_label_set_text(lv_label_vol_up, lv_label_vol_up_text.c_str());

        auto lv_btn_vol_down = lv_button_create(lv_button_bg);
        lv_obj_align(lv_btn_vol_down, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_set_size(lv_btn_vol_down, lv_pct(25), lv_pct(25));
        lv_obj_set_style_radius(lv_btn_vol_down, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_opa(lv_btn_vol_down, LV_OPA_0, LV_PART_MAIN);
        lv_obj_add_event_cb(lv_btn_vol_down, LV_OBJ_EVENT_CB(AppMp3PlayerClass, on_btn_vol_down_clicked), LV_EVENT_CLICKED, this);

        auto lv_label_vol_down = lv_label_create(lv_btn_vol_down);
        lv_obj_align(lv_label_vol_down, LV_ALIGN_CENTER, 0, 0);
        auto lv_label_vol_down_text = fa(0xf027);
        lv_label_set_text(lv_label_vol_down, lv_label_vol_down_text.c_str());

        lv_btn_loop = lv_button_create(lv_btn_container);
        lv_obj_align(lv_btn_loop, LV_ALIGN_TOP_RIGHT, 0, 0);
        lv_obj_set_size(lv_btn_loop, 35, 35);
        lv_obj_set_style_radius(lv_btn_loop, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(lv_btn_loop, LV_COLOR_UNNECCESSARY, LV_PART_MAIN);
        lv_obj_add_event_cb(lv_btn_loop, LV_OBJ_EVENT_CB(AppMp3PlayerClass, on_btn_loop_clicked), LV_EVENT_CLICKED, this);

        auto lv_label_loop = lv_label_create(lv_btn_loop);
        lv_obj_align(lv_label_loop, LV_ALIGN_CENTER, 0, 0);
        auto lv_label_loop_text = fa(0xf363);
        lv_label_set_text(lv_label_loop, lv_label_loop_text.c_str());

        get_files();
        list_item_click_ctx.resize(files.size());
        auto lv_music_symbol = fa(0xf001);
        lv_obj_t* btn;
        for (size_t i = 0; i < files.size(); i++) {
            ListItemClickCtx& row = list_item_click_ctx[i];
            row.app = this;
            row.idx = static_cast<uint16_t>(i);
            btn = lv_list_add_button(lv_list, lv_music_symbol.c_str(), files[i].c_str());
            lv_obj_add_event_cb(btn, on_list_item_lv_event, LV_EVENT_CLICKED, &row);

            auto label = lv_obj_get_child_by_type(btn, 0, &lv_label_class);
            if (label != nullptr) lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_CLIP);
        }

        if (!audio.setPinout(APP_MP3_PLAYER_I2S_BCLK, APP_MP3_PLAYER_I2S_WS, APP_MP3_PLAYER_I2S_DOUT, APP_MP3_PLAYER_I2S_MCLK)) {
            MsgBox.error("audio.setPinout thất bại", nullptr, [](bool) {});
            lv_obj_delete(screen);
            screen = nullptr;
            lv_label_play = nullptr;
            lv_btn_loop = nullptr;
            list_item_click_ctx.clear();
            return;
        }

        audio.setVolume(volume);

        pinMode(APP_MP3_PLAYER_PA_ENABLE_GPIO, OUTPUT);
        digitalWrite(APP_MP3_PLAYER_PA_ENABLE_GPIO, APP_MP3_PLAYER_PA_ACTIVE_LOW ? LOW : HIGH);

        if (!es8311.begin(APP_MP3_PLAYER_I2C_SDA, APP_MP3_PLAYER_I2C_SCL, 400000)) {
            MsgBox.error("ES8311::begin thất bại", nullptr, [](bool) {});
            lv_obj_delete(screen);
            screen = nullptr;
            lv_label_play = nullptr;
            lv_btn_loop = nullptr;
            list_item_click_ctx.clear();
            return;
        }

        es8311.setVolume(75);
        es8311.setBitsPerSample(16);

        app_closing = false;

        xTaskCreatePinnedToCore(
            FREERTOS_TASK_CB(AppMp3PlayerClass, pump_task),
            "pump_task",
            APP_MP3_PLAYER_PUMP_TASK_STACK,
            this,
            APP_MP3_PLAYER_PUMP_TASK_PRIORITY,
            &pump_task_handle,
            APP_MP3_PLAYER_PUMP_TASK_CORE
        );
        xTaskCreatePinnedToCore(
            FREERTOS_TASK_CB(AppMp3PlayerClass, playing_task),
            "playing_task",
            APP_MP3_PLAYER_PLAYING_TASK_STACK,
            this,
            APP_MP3_PLAYER_PLAYING_TASK_PRIORITY,
            &playing_task_handle,
            APP_MP3_PLAYER_PLAYING_TASK_CORE
        );
    }

    void app_close() override {
        app_closing = true;

        audio.stopSong();

        wait_until_task_exited(playing_task_handle, 3000);
        if (playing_task_handle) {
            vTaskDelete(playing_task_handle);
            playing_task_handle = nullptr;
        }

        wait_until_task_exited(pump_task_handle, 3000);
        if (pump_task_handle) {
            vTaskDelete(pump_task_handle);
            pump_task_handle = nullptr;
        }

        request_next_song = false;
        app_closing = false;

        if (!screen) return;

        lv_obj_delete(screen);
        screen = nullptr;
        lv_label_play = nullptr;
        lv_btn_loop = nullptr;
        list_item_click_ctx.clear();
    }

    void loop_ui() override {}

    void loop() override {}

    void audio_eof_cb(const char* /*filename*/) {
        if (loop_one) {
            play(current_song_idx);
            return;
        }
        auto next_idx = static_cast<uint16_t>(current_song_idx + 1);
        if (next_idx >= files.size()) next_idx = 0;
        play(next_idx);
    }

private:
    struct ListItemClickCtx {
        AppMp3PlayerClass* app = nullptr;
        uint16_t idx = 0;
    };

    static void on_list_item_lv_event(lv_event_t* e) {
        auto* ctx = static_cast<ListItemClickCtx*>(lv_event_get_user_data(e));
        if (!ctx || !ctx->app) return;

        ctx->app->on_list_item_clicked(ctx->idx);
    }

    std::string drawer_icon_utf8;

    lv_obj_t* lv_label_play = nullptr;
    lv_obj_t* lv_btn_loop = nullptr;
    std::vector<std::string> files;
    Audio audio;
    ES8311 es8311;
    TaskHandle_t pump_task_handle = nullptr;
    TaskHandle_t playing_task_handle = nullptr;
    bool suppress_click_cb = false;
    bool request_next_song = false;
    uint16_t next_song_idx = 0;
    uint16_t current_song_idx = 0;
    uint8_t volume = 0;
    uint8_t volume_steps = 0;
    bool loop_one = false;
    std::vector<ListItemClickCtx> list_item_click_ctx;

    volatile bool app_closing = false;

    void get_files() {
        files.clear();

        auto fs = MicroSD.fs();

        auto dir = fs.open("/");
        if (!dir || !dir.isDirectory()) {
            Serial.println("[AppMp3Player] Cannot read SD card");
            return;
        }

        while (true) {
            auto f = dir.openNextFile();
            if (!f) break;

            if (!f.isDirectory()) {
                String filename(f.name());
                filename.toLowerCase();
                if (filename.endsWith(".mp3")) files.emplace_back(f.name());
            }

            f.close();
        }
    }

    void pump_task() {
        while (!app_closing) {
            audio.loop();
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        pump_task_handle = nullptr;
        vTaskDelete(nullptr);
    }

    void playing_task() {
        while (!app_closing) {
            if (request_next_song) {
                request_next_song = false;
                audio.stopSong();
                audio.connecttoFS(MicroSD.fs(), files[next_song_idx].c_str());
                current_song_idx = next_song_idx;
            }
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        playing_task_handle = nullptr;
        vTaskDelete(nullptr);
    }

    void pause_resume() {
        if (!audio.pauseResume()) return;

        if (audio.isRunning()) {
            auto text = fa(0xf04c);
            lv_label_set_text(lv_label_play, text.c_str());
        } else {
            auto text = fa(0xf04b);
            lv_label_set_text(lv_label_play, text.c_str());
        }
    }

    void stop() {
        audio.stopSong();

        auto text = fa(0xf04b);
        lv_label_set_text(lv_label_play, text.c_str());
    }

    void play(uint16_t idx) {
        Serial.printf("[AppMp3Player] Play index: %d\n", idx);

        next_song_idx = idx;
        request_next_song = true;

        auto text = fa(0xf04c);
        lv_label_set_text(lv_label_play, text.c_str());
    }

    void on_list_item_clicked(uint16_t idx) {
        if (idx >= files.size()) return;

        play(idx);
    }

    void on_btn_play_clicked() {
        if (files.size() == 0) return;

        if (audio.getFileSize() == 0)
            play(0);
        else
            pause_resume();
    }

    void on_btn_next_clicked() {
        if (suppress_click_cb) {
            suppress_click_cb = false;
            return;
        }

        if (audio.getFileSize() == 0) return;

        auto next_idx = static_cast<uint16_t>(current_song_idx + 1);
        if (next_idx >= files.size()) next_idx = 0;
        play(next_idx);
    }

    void on_btn_next_long_pressed() {
        suppress_click_cb = true;

        if (audio.getFileSize() == 0) return;
        if (!audio.isRunning()) return;

        auto pos = audio.getFilePos();
        auto next_pos = pos + 100;
        if (next_pos >= audio.getFileSize()) return;

        audio.pauseResume();
        audio.setFilePos(next_pos);
        audio.pauseResume();
    }

    void on_btn_prev_clicked() {
        if (suppress_click_cb) {
            suppress_click_cb = false;
            return;
        }

        if (audio.getFileSize() == 0) return;

        int32_t prev_idx = static_cast<int32_t>(current_song_idx) - 1;
        if (prev_idx < 0) prev_idx = static_cast<int32_t>(files.size()) - 1;
        play(static_cast<uint16_t>(prev_idx));
    }

    void on_btn_prev_long_pressed() {
        suppress_click_cb = true;

        if (audio.getFileSize() == 0) return;
        if (!audio.isRunning()) return;

        auto pos = audio.getFilePos();
        int32_t prev_pos = static_cast<int32_t>(pos) - 100;
        if (prev_pos < 0) prev_pos = 0;

        audio.pauseResume();
        audio.setFilePos(static_cast<uint32_t>(prev_pos));
        audio.pauseResume();
    }

    void on_btn_vol_up_clicked() {
        volume = min(audio.maxVolume(), static_cast<uint8_t>(volume + volume_steps));
        audio.setVolume(volume);
    }

    void on_btn_vol_down_clicked() {
        volume = max(0, volume - volume_steps);
        audio.setVolume(volume);
    }

    void on_btn_loop_clicked() {
        loop_one = !loop_one;
        lv_obj_set_style_bg_color(lv_btn_loop, loop_one ? LV_COLOR_INFO : LV_COLOR_UNNECCESSARY, LV_PART_MAIN);
    }
};

extern AppMp3PlayerClass AppMp3Player;
