#include "ble_service.h"

#include <cstdio>
#include <sdkconfig.h>

#if defined(CONFIG_BLUEDROID_ENABLED)
#include <BLEDevice.h>
#include <BLEAdvertisedDevice.h>
#include <BLEScan.h>
#endif

static bool scan_active = false;
static bool scan_complete = false;
static std::vector<BleScanResult> scan_results;

#if defined(CONFIG_BLUEDROID_ENABLED)
static bool ble_initialized = false;

static BleScanResult copy_device(BLEAdvertisedDevice device) {
    BleScanResult result;
    result.address = device.getAddress().toString();
    result.address_type = (int)device.getAddressType();
    result.rssi = device.haveRSSI() ? device.getRSSI() : 0;
    result.name = device.haveName() ? device.getName() : "";
    result.has_appearance = device.haveAppearance();
    result.appearance = result.has_appearance ? device.getAppearance() : 0;
    result.has_tx_power = device.haveTXPower();
    result.tx_power = result.has_tx_power ? device.getTXPower() : 0;

    for (int i = 0; i < device.getServiceUUIDCount(); i++) {
        result.service_uuids.push_back(device.getServiceUUID(i).toString());
    }
    for (int i = 0; i < device.getServiceDataUUIDCount(); i++) {
        result.service_data_uuids.push_back(device.getServiceDataUUID(i).toString());
    }
    for (int i = 0; i < device.getServiceDataCount(); i++) {
        result.service_data.push_back(device.getServiceData(i));
    }
    if (device.haveManufacturerData()) result.manufacturer_data = device.getManufacturerData();

    const uint8_t* payload = device.getPayload();
    if (payload && device.getPayloadLength() > 0) {
        result.payload.assign(payload, payload + device.getPayloadLength());
    }
    return result;
}

static void on_scan_complete(BLEScanResults results) {
    scan_results.clear();
    scan_results.reserve(results.getCount());
    for (int i = 0; i < results.getCount(); i++) {
        scan_results.push_back(copy_device(results.getDevice(i)));
    }
    scan_active = false;
    scan_complete = true;
}
#endif

bool ble_scan_start(uint32_t duration_seconds, bool active) {
    if (scan_active) return false;
#if defined(CONFIG_BLUEDROID_ENABLED)
    if (!ble_initialized) {
        BLEDevice::init("");
        ble_initialized = true;
    }
    BLEScan* scan = BLEDevice::getScan();
    scan->setActiveScan(active);
    scan->clearResults();
    scan_results.clear();
    scan_complete = false;
    scan_active = scan->start(duration_seconds, on_scan_complete, false);
    return scan_active;
#else
    (void)duration_seconds;
    (void)active;
    return false;
#endif
}

void ble_scan_cancel() {
#if defined(CONFIG_BLUEDROID_ENABLED)
    if (scan_active) BLEDevice::getScan()->stop();
    if (ble_initialized) {
        BLEDevice::getScan()->clearResults();
        BLEDevice::deinit(true);
        ble_initialized = false;
    }
#endif
    scan_active = false;
    scan_complete = false;
    scan_results.clear();
    scan_results.shrink_to_fit();
}

const char* ble_scan_status() {
    if (scan_active) return "scanning";
    if (scan_complete) return "complete";
#if defined(CONFIG_BLUEDROID_ENABLED)
    return "idle";
#else
    return "unavailable";
#endif
}

int ble_scan_get_count() {
    return scan_complete ? (int)scan_results.size() : 0;
}

bool ble_scan_get_result(int index, BleScanResult& result) {
    if (!scan_complete || index < 0 || index >= (int)scan_results.size()) return false;
    result = scan_results[index];
    return true;
}

std::vector<BleAdField> ble_parse_advertisement(const uint8_t* data, size_t length) {
    std::vector<BleAdField> fields;
    size_t offset = 0;
    while (offset < length) {
        uint8_t field_length = data[offset++];
        if (field_length == 0) break;
        if (offset + field_length > length) break;

        BleAdField field{};
        field.type = data[offset];
        if (field_length > 1) {
            field.data.assign(data + offset + 1, data + offset + field_length);
        }
        size_t value_length = field_length - 1;
        switch (field.type) {
            case 0x01:
                if (value_length > 0) {
                    field.flags = field.data[0];
                    field.has_flags = true;
                }
                break;
            case 0x08:
            case 0x09:
                field.name.assign((const char*)field.data.data(), field.data.size());
                break;
            case 0x0A:
                if (value_length > 0) {
                    field.tx_power = (int8_t)field.data[0];
                    field.has_tx_power = true;
                }
                break;
            case 0x02:
            case 0x03:
                if (value_length >= 2) {
                    char uuid[5];
                    snprintf(uuid, sizeof(uuid), "%02X%02X", field.data[1], field.data[0]);
                    field.uuid = uuid;
                }
                break;
            case 0x06:
            case 0x07:
                if (value_length >= 16) {
                    char uuid[37];
                    snprintf(uuid, sizeof(uuid), "%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                        field.data[15], field.data[14], field.data[13], field.data[12], field.data[11], field.data[10],
                        field.data[9], field.data[8], field.data[7], field.data[6], field.data[5], field.data[4],
                        field.data[3], field.data[2], field.data[1], field.data[0]);
                    field.uuid = uuid;
                }
                break;
            case 0xFF:
                if (value_length >= 2) {
                    field.company_id = field.data[0] | (field.data[1] << 8);
                    field.has_company_id = true;
                }
                break;
            default:
                break;
        }
        fields.push_back(field);
        offset += field_length;
    }
    return fields;
}