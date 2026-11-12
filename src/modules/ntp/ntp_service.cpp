#include "ntp_service.h"
#include <Arduino.h>
#include <esp_sntp.h>
#include <sys/time.h>

static volatile bool g_synced = false;
static const time_t MIN_VALID_EPOCH = 1704067200; // 2024-01-01 00:00:00 UTC

// Runs in the lwIP task, so it only sets a flag.
static void on_sync(struct timeval*) {
    g_synced = true;
}

void ntp_start(const char* tz, const char* server) {
    g_synced = false;
    sntp_set_time_sync_notification_cb(on_sync);
    // configTzTime sets TZ and (re)starts SNTP; it does not wait for a response.
    // If Wi-Fi isn't up yet SNTP keeps retrying in the background.
    configTzTime((tz && *tz) ? tz : "UTC0",
                 (server && *server) ? server : "pool.ntp.org",
                 "time.nist.gov");
}

bool ntp_synced() {
    return g_synced;
}

bool ntp_time_valid() {
    return time(nullptr) >= MIN_VALID_EPOCH;
}
