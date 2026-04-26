#pragma once

#include <Arduino.h>
#include <esp_sntp.h>
#include <time.h>

#define TIME_SYNC_GMT_OFFSET_SEC (7 * 3600) // GMT+7 (Việt Nam)
#define TIME_SYNC_DAYLIGHT_OFFSET_SEC 0
#define TIME_SYNC_NTP_SERVER_1 "pool.ntp.org"
#define TIME_SYNC_NTP_SERVER_2 "time.google.com"
#define TIME_SYNC_NTP_SERVER_3 "time.cloudflare.com"
#define TIME_SYNC_POLL_SLICE_MS 200
#define TIME_SYNC_TOTAL_TIMEOUT_MS 60000
#define TIME_SYNC_MIN_VALID_EPOCH __TIMESTAMP__

static bool s_synced = false;
static bool s_abandoned = false;
static bool s_ntp_configured = false;
static unsigned long s_ntp_start_ms = 0;

class TimeSync
{
public:
    static bool is_synced() { return s_synced; }

    static struct tm get_time()
    {
        struct tm ti{};
        getLocalTime(&ti, 0);
        return ti;
    }

    static void loop()
    {
        if (s_synced || s_abandoned)
        {
            return;
        }

        if (!s_ntp_configured)
        {
            configTime(TIME_SYNC_GMT_OFFSET_SEC, TIME_SYNC_DAYLIGHT_OFFSET_SEC, TIME_SYNC_NTP_SERVER_1, TIME_SYNC_NTP_SERVER_2, TIME_SYNC_NTP_SERVER_3);
            s_ntp_configured = true;
            s_ntp_start_ms = millis();
        }

        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED && time(nullptr) >= (time_t)TIME_SYNC_MIN_VALID_EPOCH)
        {
            s_synced = true;
            return;
        }

        if (!s_synced && (millis() - s_ntp_start_ms >= TIME_SYNC_TOTAL_TIMEOUT_MS))
        {
            s_abandoned = true;
        }
    }
};
