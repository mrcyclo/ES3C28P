#include "micro_sd.h"
#include <esp_heap_caps.h>
#include <cstdint>
#include <cstring>

namespace
{
    uint16_t read_le16(File &f)
    {
        uint8_t b[2];
        if (f.read(b, 2) != 2)
            return 0;
        return (uint16_t)b[0] | ((uint16_t)b[1] << 8);
    }

    uint32_t read_le32(File &f)
    {
        uint8_t b[4];
        if (f.read(b, 4) != 4)
            return 0;
        return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
    }
}

bool MicroSDClass::mount()
{
    if (s_mounted)
        return true;

    SD_MMC.setPins(MICRO_SD_MMC_CLK, MICRO_SD_MMC_CMD, MICRO_SD_MMC_D0, MICRO_SD_MMC_D1, MICRO_SD_MMC_D2, MICRO_SD_MMC_D3);
    if (!SD_MMC.begin(MICRO_SD_MOUNT_POINT, false, false))
    {
        Serial.println("[MicroSD] Failed to mount SD_MMC");
        return false;
    }

    Serial.println("[MicroSD] SD_MMC mounted successfully");
    s_mounted = true;
    return true;
}

void MicroSDClass::unmount()
{
    if (!s_mounted)
        return;
    SD_MMC.end();
    s_mounted = false;
}

bool MicroSDClass::is_mounted()
{
    return s_mounted;
}

fs::FS &MicroSDClass::fs()
{
    return SD_MMC;
}

uint64_t MicroSDClass::card_size_bytes()
{
    if (!s_mounted)
        return 0;
    return SD_MMC.cardSize();
}

bool MicroSDClass::lv_read_bmp_dsc_rgb565(const char *path, lv_image_dsc_t *out_dsc, uint16_t **out_pixels)
{
    if (!s_mounted || !path || !out_dsc || !out_pixels)
        return false;

    *out_pixels = nullptr;
    memset(out_dsc, 0, sizeof(*out_dsc));

    File f = fs().open(path, FILE_READ);
    if (!f)
        return false;

    // --- BMP FILE HEADER (14 bytes) ---
    const uint16_t bfType = read_le16(f); // 'BM'
    if (bfType != 0x4D42)
    {
        f.close();
        return false;
    }

    (void)read_le32(f);                      // bfSize
    (void)read_le16(f);                      // bfReserved1
    (void)read_le16(f);                      // bfReserved2
    const uint32_t bfOffBits = read_le32(f); // pixel data offset

    // --- DIB HEADER (BITMAPINFOHEADER expected) ---
    const uint32_t biSize = read_le32(f);
    if (biSize < 40)
    {
        f.close();
        return false;
    }

    const int32_t biWidth = (int32_t)read_le32(f);
    const int32_t biHeightSigned = (int32_t)read_le32(f);
    const uint16_t biPlanes = read_le16(f);
    const uint16_t biBitCount = read_le16(f);
    const uint32_t biCompression = read_le32(f);

    // Skip the rest of BITMAPINFOHEADER
    (void)read_le32(f); // biSizeImage
    (void)read_le32(f); // biXPelsPerMeter
    (void)read_le32(f); // biYPelsPerMeter
    (void)read_le32(f); // biClrUsed
    (void)read_le32(f); // biClrImportant

    if (biPlanes != 1 || biWidth <= 0 || biHeightSigned == 0)
    {
        f.close();
        return false;
    }

    // Only handle uncompressed 24-bit BMP
    if (biBitCount != 24 || biCompression != 0)
    {
        f.close();
        return false;
    }

    const uint32_t w = (uint32_t)biWidth;
    const uint32_t h = (uint32_t)(biHeightSigned > 0 ? biHeightSigned : -biHeightSigned);
    const bool bottom_up = (biHeightSigned > 0);

    const size_t pixel_count = (size_t)w * (size_t)h;
    const size_t bytes_needed = pixel_count * sizeof(uint16_t);

    uint16_t *buf = (uint16_t *)heap_caps_malloc(bytes_needed, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buf)
        buf = (uint16_t *)heap_caps_malloc(bytes_needed, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!buf)
    {
        f.close();
        return false;
    }

    // Row size is padded to 4 bytes
    const uint32_t row_bytes_raw = w * 3u;
    const uint32_t row_stride = (row_bytes_raw + 3u) & ~3u;

    if (!f.seek(bfOffBits))
    {
        heap_caps_free(buf);
        f.close();
        return false;
    }

    uint8_t tmp[64];
    for (uint32_t y = 0; y < h; ++y)
    {
        const uint32_t dst_y = bottom_up ? (h - 1u - y) : y;
        uint16_t *dst = buf + (size_t)dst_y * (size_t)w;

        for (uint32_t x = 0; x < w; ++x)
        {
            uint8_t bgr[3];
            if (f.read(bgr, 3) != 3)
            {
                heap_caps_free(buf);
                f.close();
                return false;
            }
            const uint8_t b = bgr[0];
            const uint8_t g = bgr[1];
            const uint8_t r = bgr[2];
            dst[x] = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
        }

        uint32_t pad = row_stride - row_bytes_raw;
        while (pad)
        {
            const uint32_t chunk = pad > sizeof(tmp) ? (uint32_t)sizeof(tmp) : pad;
            if (f.read(tmp, chunk) != (int)chunk)
            {
                heap_caps_free(buf);
                f.close();
                return false;
            }
            pad -= chunk;
        }
    }

    f.close();

    out_dsc->header.w = (uint16_t)w;
    out_dsc->header.h = (uint16_t)h;
    out_dsc->header.cf = LV_COLOR_FORMAT_RGB565;
    out_dsc->data = (const uint8_t *)buf;
    out_dsc->data_size = (uint32_t)bytes_needed;

    *out_pixels = buf;
    return true;
}

MicroSDClass MicroSD;
