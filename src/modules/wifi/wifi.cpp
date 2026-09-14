#include "wifi.h"
#include "../../core/paths.h"
#include "../storage/storage_service.h"
#include <WiFi.h>
#include <vector>

#define RSSI_ROAM_THRESHOLD -75  // if current connection drops below this, consider switching
#define MONITOR_INTERVAL_MS 15000

static std::vector<WifiNetwork> known_networks;
static unsigned long last_monitor_check = 0;

static WifiState state = WIFI_STATE_IDLE;
static unsigned long state_started = 0;
static const WifiNetwork* pending_network = nullptr;
static bool autoconnect_enabled = true;

bool wifi_connect(const char* ssid, const char* password) {
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }

    return WiFi.status() == WL_CONNECTED;
}

void wifi_set_known_networks(const std::vector<WifiNetwork>& networks) {
    known_networks = networks;
}

void wifi_start_auto_connect() {
    WiFi.scanNetworks(true); // true = async scan, returns immediately
    state = WIFI_STATE_SCANNING;
    state_started = millis();
}

void wifi_tick() {
    switch (state) {
        case WIFI_STATE_SCANNING: {
            int n = WiFi.scanComplete(); // -1 = still scanning, -2 = not started, >=0 = done
            if (n >= 0) {
                int best_rssi = -1000;
                const WifiNetwork* best_match = nullptr;

                for (int i = 0; i < n; i++) {
                    String found_ssid = WiFi.SSID(i);
                    for (auto& known : known_networks) {
                        if (found_ssid == known.ssid.c_str() && WiFi.RSSI(i) > best_rssi) {
                            best_rssi = WiFi.RSSI(i);
                            best_match = &known;
                        }
                    }
                }
                WiFi.scanDelete();

                if (best_match) {
                    WiFi.begin(best_match->ssid.c_str(), best_match->password.c_str());
                    pending_network = best_match;
                    state = WIFI_STATE_CONNECTING;
                    state_started = millis();
                } else {
                    state = WIFI_STATE_FAILED;
                }
            } else if (millis() - state_started > 10000) {
                state = WIFI_STATE_FAILED; // scan timeout safety net
            }
            break;
        }

        case WIFI_STATE_CONNECTING: {
            if (WiFi.status() == WL_CONNECTED) {
                state = WIFI_STATE_CONNECTED;
            } else if (millis() - state_started > 10000) {
                state = WIFI_STATE_FAILED;
            }
            break;
        }

        case WIFI_STATE_IDLE: {
            // Auto-reconnect: if idle and not connected, start scanning
            if (WiFi.status() != WL_CONNECTED && !known_networks.empty()) {
                wifi_start_auto_connect();
            }
            break;
        }

        case WIFI_STATE_FAILED: {
            // Retry after a delay
            if (millis() - state_started > 5000) {
                if (!known_networks.empty()) {
                    wifi_start_auto_connect();
                } else {
                    state = WIFI_STATE_IDLE;
                }
            }
            break;
        }

        default:
            break; // CONNECTED — nothing to advance
    }
}

static void save_wifi_config() {
    std::string out = "return {\n    autoconnect = ";
    out += autoconnect_enabled ? "true" : "false";
    out += ",\n    networks = {\n";
    for (auto& n : known_networks) {
        std::string safe_ssid = storage_escape_lua_string(n.ssid);
        std::string safe_password = storage_escape_lua_string(n.password);
        out += "        { ssid = \"" + safe_ssid + "\", password = \"" + safe_password + "\" },\n";
    }
    out += "    }\n}\n";
    storage_write(SG_SYSTEM_DIR "/wifi_networks.lua", out.c_str());
}

void wifi_set_autoconnect(bool enabled) {
    autoconnect_enabled = enabled;
    save_wifi_config();
}

bool wifi_get_autoconnect() {
    return autoconnect_enabled;
}

void wifi_add_network(const char* ssid, const char* password) {
    known_networks.push_back({ssid, password});
    save_wifi_config();
}

WifiState wifi_get_state() {
    return state;
}

bool wifi_is_connected() {
    return WiFi.status() == WL_CONNECTED;
}

void wifi_disconnect() {
    WiFi.disconnect();
    state = WIFI_STATE_IDLE;
    pending_network = nullptr;
}

int wifi_get_rssi() {
    return WiFi.RSSI();
}

const char* wifi_get_ip() {
    static String ip; // static so the pointer stays valid after return
    ip = WiFi.localIP().toString();
    return ip.c_str();
}

const char* wifi_get_ssid() {
    static String ssid; // static so the pointer stays valid after return
    ssid = WiFi.SSID();
    return ssid.c_str();
}

void wifi_remove_network(int index) {
    if (index < 0 || index >= (int)known_networks.size()) return;
    known_networks.erase(known_networks.begin() + index);
    save_wifi_config();
}

int wifi_get_network_count() {
    return known_networks.size();
}

const char* wifi_get_network_ssid(int index) {
    if (index < 0 || index >= (int)known_networks.size()) return "";
    return known_networks[index].ssid.c_str();
}