#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SD_MMC.h>

#define MICRO_SD_MMC_CLK 38
#define MICRO_SD_MMC_CMD 40
#define MICRO_SD_MMC_D0 39
#define MICRO_SD_MMC_D1 41
#define MICRO_SD_MMC_D2 48
#define MICRO_SD_MMC_D3 47
#define MICRO_SD_MOUNT_POINT "/sd"

class MicroSD
{
private:
    inline static bool s_mounted = false;

public:
    static bool mount()
    {
        if (s_mounted)
            return true;

        SD_MMC.setPins(MICRO_SD_MMC_CLK, MICRO_SD_MMC_CMD, MICRO_SD_MMC_D0, MICRO_SD_MMC_D1, MICRO_SD_MMC_D2, MICRO_SD_MMC_D3);
        if (!SD_MMC.begin(MICRO_SD_MOUNT_POINT, false, false))
            return false;

        s_mounted = true;
        return true;
    }

    static void unmount()
    {
        if (!s_mounted)
            return;
        SD_MMC.end();
        s_mounted = false;
    }

    static bool is_mounted() { return s_mounted; }

    static fs::FS &fs() { return SD_MMC; }

    static uint64_t card_size_bytes()
    {
        if (!s_mounted)
            return 0;
        return SD_MMC.cardSize();
    }
};
