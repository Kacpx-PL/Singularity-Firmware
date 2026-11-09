#pragma once

#include <cstddef>
#include <cstdint>

using EspNowReceiveCallback = void (*)(const uint8_t* mac, const uint8_t* data, int length);

bool espnow_init();
bool espnow_add_peer(const uint8_t* mac);
bool espnow_send(const uint8_t* mac, const uint8_t* data, size_t length);
void espnow_set_receive_callback(EspNowReceiveCallback callback);
