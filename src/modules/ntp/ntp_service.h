#pragma once
#include <ctime>

// Starts SNTP in the background and returns immediately (nothing blocks).
// tz is a POSIX TZ string, e.g. "UTC0" or "CET-1CEST,M3.5.0,M10.5.0/3".
void ntp_start(const char* tz, const char* server);

// true once an NTP response has been applied since the last ntp_start()
bool ntp_synced();

// true if the system clock holds a plausible date (2024 or later). This can already be
// the case without NTP if the clock was seeded from the RTC at boot.
bool ntp_time_valid();
