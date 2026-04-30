#pragma once

#include <Arduino.h>
#include <esp_sntp.h>
#include <time.h>
#include "wifi_connector/wifi_connector.h"
#include "common/imodule.h"

#define TIME_SYNC_GMT_OFFSET_SEC (7 * 3600) // GMT+7 (Việt Nam)
#define TIME_SYNC_DAYLIGHT_OFFSET_SEC 0
#define TIME_SYNC_NTP_SERVER_1 "pool.ntp.org"
#define TIME_SYNC_NTP_SERVER_2 "time.google.com"
#define TIME_SYNC_NTP_SERVER_3 "time.cloudflare.com"
#define TIME_SYNC_TOTAL_TIMEOUT_MS 30000
#define TIME_SYNC_MIN_VALID_EPOCH 1777234651L

class TimeSyncClass : public ModuleOnce
{
public:
    void loop_ui() override {}
    void loop() override
    {
        if (synced || !WifiConnector.is_connected())
        {
            return;
        }

        if (!ntp_configured || millis() - ntp_start_ms >= TIME_SYNC_TOTAL_TIMEOUT_MS)
        {
            configTime(TIME_SYNC_GMT_OFFSET_SEC, TIME_SYNC_DAYLIGHT_OFFSET_SEC, TIME_SYNC_NTP_SERVER_1, TIME_SYNC_NTP_SERVER_2, TIME_SYNC_NTP_SERVER_3);
            ntp_configured = true;
            ntp_start_ms = millis();
        }

        if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED && time(nullptr) >= (time_t)TIME_SYNC_MIN_VALID_EPOCH)
        {
            synced = true;
        }
    }

    bool is_synced() { return synced; }
    struct tm get_time()
    {
        struct tm ti{};
        getLocalTime(&ti, 0);
        return ti;
    }

protected:
    void setup_impl() override {}

private:
    bool synced = false;
    bool ntp_configured = false;
    unsigned long ntp_start_ms = 0;
};

extern TimeSyncClass TimeSync;
