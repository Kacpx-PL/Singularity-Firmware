#pragma once
#include <ctime>

// System-owned clock. Apps never start it; they only read the time.
//
// ntp_tick() is called from the main loop. The first time Wi-Fi is connected it starts SNTP
// (non-blocking); lwIP then re-syncs by itself every hour and keeps retrying if a request
// fails, so it is only ever started once per boot.

void ntp_tick();

// POSIX TZ string, e.g. "UTC0" or "CET-1CEST,M3.5.0,M10.5.0/3". Applies immediately
// (only changes how local time is computed, no resync needed).
void ntp_set_timezone(const char* tz);

// true once an NTP response has been applied
bool ntp_synced();

// true if the system clock holds a plausible date (2024 or later). This can already be
// the case without NTP if the clock was seeded from the RTC at boot.
bool ntp_time_valid();

// Local time (in the configured timezone). Returns false if the clock isn't valid yet.
bool ntp_get_local_time(struct tm* out);
