#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>
#include <lvgl.h>

#define MICRO_SD_MMC_CLK 38
#define MICRO_SD_MMC_CMD 40
#define MICRO_SD_MMC_D0 39
#define MICRO_SD_MMC_D1 41
#define MICRO_SD_MMC_D2 48
#define MICRO_SD_MMC_D3 47
#define MICRO_SD_MOUNT_POINT "/sdcard"

class MicroSDClass
{
private:
    bool s_mounted = false;

public:
    bool mount();
    void unmount();
    bool is_mounted();
    fs::FS &fs();
    uint64_t card_size_bytes();

    /** Read an uncompressed 24-bit BMP from SD and convert to RGB565 buffer.
     *  - **Allocates** `*out_pixels` (prefer PSRAM) that the caller must free with `heap_caps_free()`.
     *  - Fills `out_dsc` to reference that buffer, suitable for `lv_image_set_src(obj, out_dsc)`.
     */
    bool lv_read_bmp_dsc_rgb565(const char *path, lv_image_dsc_t *out_dsc, uint16_t **out_pixels);
};

extern MicroSDClass MicroSD;
