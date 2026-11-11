#include "wifi.h"
#include "../../core/paths.h"
#include "../storage/storage_service.h"
#include <WiFi.h>
#include <vector>

#define DISCONNECT_GRACE_MS 5000  // connection must be gone this long before we treat it as dropped

static std::vector<WifiNetwork> known_networks;
static unsigned long disconnected_since = 0;
static bool suppress_reconnect = false; // set by wifi_disconnect(): stay offline until the user connects again
static bool save_blocked = false;       // set when wifi_networks.lua exists but could not be parsed

static WifiState state = WIFI_STATE_IDLE;
static unsigned long state_started = 0;
static const WifiNetwork* pending_network = nullptr;
static bool autoconnect_enabled = true;
static bool lua_scan_active = false;
static bool lua_scan_complete = false;
static std::vector<WifiScanResult> lua_scan_results;

bool wifi_connect(const char* ssid, const char* password) {
    suppress_reconnect = false; // the user explicitly asked to connect
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        attempts++;
    }

    bool ok = (WiFi.status() == WL_CONNECTED);
    // keep the state machine in sync with what just happened
    state = ok ? WIFI_STATE_CONNECTED : WIFI_STATE_IDLE;
    pending_network = nullptr;
    disconnected_since = 0;
    return ok;
}

void wifi_set_known_networks(const std::vector<WifiNetwork>& networks) {
    known_networks = networks;
}

// Used at boot: applies the loaded settings WITHOUT writing them back to the SD card.
void wifi_apply_config(const std::vector<WifiNetwork>& networks, bool autoconnect) {
    known_networks = networks;
    autoconnect_enabled = autoconnect;
}

// Called when wifi_networks.lua exists but couldn't be read/parsed, so that a later
// addNetwork()/setAutoconnect() can't overwrite the user's saved networks with an empty list.
void wifi_block_saving(bool blocked) {
    save_blocked = blocked;
}

void wifi_start_auto_connect() {
    if (lua_scan_active) return; // don't fight an app's own scan
    suppress_reconnect = false;
    WiFi.scanNetworks(true); // true = async scan, returns immediately
    state = WIFI_STATE_SCANNING;
    state_started = millis();
}

void wifi_tick() {
    if (lua_scan_active) {
        int n = WiFi.scanComplete();
        if (n >= 0) {
            lua_scan_results.clear();
            lua_scan_results.reserve(n);
            for (int i = 0; i < n; i++) {
                WifiScanResult result;
                result.ssid = WiFi.SSID(i).c_str();
                result.bssid = WiFi.BSSIDstr(i).c_str();
                result.rssi = WiFi.RSSI(i);
                result.channel = WiFi.channel(i);
                result.encryption = (uint8_t)WiFi.encryptionType(i);
                lua_scan_results.push_back(result);
            }
            WiFi.scanDelete();
            lua_scan_active = false;
            lua_scan_complete = true;
        }
        return;
    }

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
                disconnected_since = 0;
            } else if (millis() - state_started > 10000) {
                state = WIFI_STATE_FAILED;
            }
            break;
        }

        case WIFI_STATE_CONNECTED: {
            // Notice a dropped connection (AP reboot, out of range...) after a short grace
            // period, then hand over to IDLE which reconnects if auto-connect is on.
            if (WiFi.status() == WL_CONNECTED) {
                disconnected_since = 0;
            } else if (disconnected_since == 0) {
                disconnected_since = millis();
            } else if (millis() - disconnected_since > DISCONNECT_GRACE_MS) {
                disconnected_since = 0;
                state = WIFI_STATE_IDLE;
            }
            break;
        }

        case WIFI_STATE_IDLE: {
            if (WiFi.status() == WL_CONNECTED) {
                state = WIFI_STATE_CONNECTED; // something else connected us
            } else if (autoconnect_enabled && !suppress_reconnect && !known_networks.empty()) {
                // Auto-reconnect: idle, not connected, allowed to auto-connect -> scan
                wifi_start_auto_connect();
            }
            break;
        }

        case WIFI_STATE_FAILED: {
            // Retry after a delay, but only if auto-connect is still allowed
            if (millis() - state_started > 5000) {
                if (autoconnect_enabled && !suppress_reconnect && !known_networks.empty()) {
                    wifi_start_auto_connect();
                } else {
                    state = WIFI_STATE_IDLE;
                }
            }
            break;
        }

        default:
            break;
    }
}

static void save_wifi_config() {
    if (save_blocked) {
        Serial.println("[WIFI] saving blocked: wifi_networks.lua could not be parsed at boot, not overwriting it");
        return;
    }
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
    if (enabled) suppress_reconnect = false; // turning it on again lifts a previous manual disconnect
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
    suppress_reconnect = true; // stay disconnected until the user connects again
    disconnected_since = 0;
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

bool wifi_scan_start(bool show_hidden) {
    if (lua_scan_active || state == WIFI_STATE_SCANNING) return false;
    int result = WiFi.scanNetworks(true, show_hidden);
    if (result == WIFI_SCAN_FAILED) return false;
    lua_scan_results.clear();
    lua_scan_active = true;
    lua_scan_complete = false;
    return true;
}

void wifi_scan_cancel() {
    // Always release results, even if the scan already completed; otherwise
    // the vector's memory stays allocated after the app that scanned has closed.
    if (lua_scan_active) WiFi.scanDelete();
    lua_scan_active = false;
    lua_scan_complete = false;
    lua_scan_results.clear();
    lua_scan_results.shrink_to_fit();
}

const char* wifi_scan_status() {
    if (lua_scan_active) return "scanning";
    if (lua_scan_complete) return "complete";
    return "idle";
}

int wifi_scan_get_count() {
    return lua_scan_complete ? (int)lua_scan_results.size() : 0;
}

bool wifi_scan_get_result(int index, WifiScanResult& result) {
    if (!lua_scan_complete || index < 0 || index >= (int)lua_scan_results.size()) return false;
    result = lua_scan_results[index];
    return true;
}