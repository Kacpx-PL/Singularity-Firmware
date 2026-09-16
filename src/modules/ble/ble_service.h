#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct BleScanResult {
    std::string address;
    int address_type;
    int rssi;
    std::string name;
    int appearance;
    int tx_power;
    bool has_appearance;
    bool has_tx_power;
    std::vector<std::string> service_uuids;
    std::vector<std::string> service_data_uuids;
    std::vector<std::string> service_data;
    std::string manufacturer_data;
    std::vector<uint8_t> payload;
};

struct BleAdField {
    uint8_t type;
    std::vector<uint8_t> data;
    std::string name;
    std::string uuid;
    int flags;
    int tx_power;
    int company_id;
    bool has_flags;
    bool has_tx_power;
    bool has_company_id;
};

bool ble_scan_start(uint32_t duration_seconds, bool active);
void ble_scan_cancel();
const char* ble_scan_status();
int ble_scan_get_count();
bool ble_scan_get_result(int index, BleScanResult& result);
std::vector<BleAdField> ble_parse_advertisement(const uint8_t* data, size_t length);