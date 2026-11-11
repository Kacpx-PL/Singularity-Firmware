#include "espnow_service.h"

#include <WiFi.h>
#include <esp_idf_version.h>
#include <esp_now.h>
#include <cstring>

static EspNowReceiveCallback receive_callback = nullptr;
static bool initialized = false;

#if ESP_IDF_VERSION_MAJOR >= 5
static void on_receive(const esp_now_recv_info_t* info, const uint8_t* data, int length) {
    if (receive_callback) receive_callback(info->src_addr, data, length);
}
#else
static void on_receive(const uint8_t* mac, const uint8_t* data, int length) {
    if (receive_callback) receive_callback(mac, data, length);
}
#endif

bool espnow_init() {
    if (initialized) return true;
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) return false;
    if (esp_now_register_recv_cb(on_receive) != ESP_OK) {
        esp_now_deinit();
        return false;
    }
    initialized = true;
    return true;
}

bool espnow_add_peer(const uint8_t* mac) {
    if (!mac) return false;

    esp_now_peer_info_t peer{};
    std::memcpy(peer.peer_addr, mac, ESP_NOW_ETH_ALEN);
    peer.ifidx = WIFI_IF_STA;
    peer.channel = 0;
    peer.encrypt = false;
    return esp_now_add_peer(&peer) == ESP_OK;
}

bool espnow_send(const uint8_t* mac, const uint8_t* data, size_t length) {
    if (!mac || (!data && length > 0) || length > ESP_NOW_MAX_DATA_LEN) return false;
    return esp_now_send(mac, data, length) == ESP_OK;
}

void espnow_set_receive_callback(EspNowReceiveCallback callback) {
    receive_callback = callback;
}
