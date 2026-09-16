#pragma once
#include <string>
#include <vector>

struct WifiScanResult {
    std::string ssid;
    std::string bssid;
    int32_t rssi;
    int32_t channel;
    uint8_t encryption;
};

struct WifiNetwork {
    std::string ssid;
    std::string password;
};

enum WifiState {
    WIFI_STATE_IDLE,
    WIFI_STATE_SCANNING,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_FAILED
};


void wifi_set_known_networks(const std::vector<WifiNetwork>& networks);
bool wifi_connect(const char* ssid, const char* password);
void wifi_set_autoconnect(bool enabled); // saves to file
bool wifi_get_autoconnect();
void wifi_add_network(const char* ssid, const char* password); // saves to file

void wifi_tick();
bool wifi_is_connected();
void wifi_disconnect();
int wifi_get_rssi();
void wifi_start_auto_connect(); // non-blocking, kicks off the process
void wifi_remove_network(int index);
int wifi_get_network_count();
const char* wifi_get_network_ssid(int index);
const char* wifi_get_ip(); // returns "" if not connected
const char* wifi_get_ssid(); // returns "" if not connected
WifiState wifi_get_state();

bool wifi_scan_start(bool show_hidden);
void wifi_scan_cancel();
const char* wifi_scan_status();
int wifi_scan_get_count();
bool wifi_scan_get_result(int index, WifiScanResult& result);

