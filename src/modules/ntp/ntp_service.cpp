#include "ntp_service.h"
#include "../wifi/wifi.h"
#include <Arduino.h>
#include <esp_sntp.h>
#include <sys/time.h>
#include <string>
#include <stdlib.h>

static volatile bool g_synced = false;
static bool g_started = false;
static std::string g_tz = "UTC0";
static const time_t MIN_VALID_EPOCH = 1704067200; // 2024-01-01 00:00:00 UTC

// Runs in the lwIP task, so it only sets a flag.
static void on_sync(struct timeval*) {
    g_synced = true;
}

void ntp_set_timezone(const char* tz) {
    g_tz = (tz && *tz) ? tz : "UTC0";
    setenv("TZ", g_tz.c_str(), 1);
    tzset();
}

void ntp_tick() {
    if (g_started) return;
    if (!wifi_is_connected()) return;

    sntp_set_time_sync_notification_cb(on_sync);
    // Sets TZ and starts SNTP; does not wait for a response.
    configTzTime(g_tz.c_str(), "pool.ntp.org", "time.nist.gov");
    g_started = true;
}

bool ntp_synced() {
    return g_synced;
}

bool ntp_time_valid() {
    return time(nullptr) >= MIN_VALID_EPOCH;
}

bool ntp_get_local_time(struct tm* out) {
    time_t now = time(nullptr);
    if (now < MIN_VALID_EPOCH) return false;
    localtime_r(&now, out);
    return true;
}
