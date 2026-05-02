#pragma once

#include <FS.h>
#include <lvgl.h>

#include <string>
#include <vector>

#include "common/imodule.h"
#include "micro_sd/micro_sd.h"

class MP3PlayerClass : public ModuleOnce {
public:
    void loop_ui() override {}

    void loop() override {
        // Sau lần đầu xử lý lỗi/scan, hàm không còn việc gì → return ngay để không
        // chạm widget hay duyệt SD mỗi 250ms cho không.
        if (done) return;

        if (is_error) {
            lv_obj_remove_flag(label_error, LV_OBJ_FLAG_HIDDEN);
            done = true;
            return;
        }

        if (!MicroSD.is_mounted()) {
            is_error = true;
            lv_label_set_text(label_error, "SD Card is not mounted");
            return;
        }

        if (!is_scanned) {
            is_scanned = true;

            auto& fs = MicroSD.fs();
            auto dir = fs.open("/");
            if (!dir || !dir.isDirectory()) {
                is_error = true;
                lv_label_set_text(label_error, "SD Card is not a directory");
                return;
            }

            for (fs::File f = dir.openNextFile(); f; f = dir.openNextFile()) {
                String name(f.name());
                name.toLowerCase();
                if (!f.isDirectory() && f.size() > 0 && name.endsWith(".mp3")) {
                    files.push_back(f.name());
                }
                f.close();
            }
            dir.close();
        }

        if (files.size() == 0) {
            is_error = true;
            lv_label_set_text(label_error, "No MP3 files found");
            return;
        }

        // Đã scan xong, có file → kết thúc trạng thái khởi tạo.
        done = true;
    }

    lv_obj_t* get_screen() { return screen; }

protected:
    void setup_impl() override {
        screen = lv_obj_create(NULL);
        lv_obj_set_style_pad_all(screen, 10, LV_PART_MAIN);

        label_error = lv_label_create(screen);
        lv_obj_align(label_error, LV_ALIGN_CENTER, 0, 0);
        lv_label_set_text(label_error, "Error Text Here");
        lv_obj_add_flag(label_error, LV_OBJ_FLAG_HIDDEN);
    }

private:
    lv_obj_t* screen = nullptr;
    lv_obj_t* label_error = nullptr;
    bool is_scanned = false;
    bool is_error = false;
    bool done = false;
    std::vector<std::string> files;
};

extern MP3PlayerClass MP3Player;
