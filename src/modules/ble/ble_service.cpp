#include "ble_service.h"

#include <cstdio>
#include <sdkconfig.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#if defined(CONFIG_BLUEDROID_ENABLED)
#include <BLEDevice.h>
#include <BLEAdvertisedDevice.h>
#include <BLEScan.h>
#endif

// scan_* are shared between the main task and the BLE task (scan-complete callback),
// so they're guarded by a mutex.
static volatile bool scan_active = false;
static volatile bool scan_complete = false;
static std::vector<BleScanResult> scan_results;
static SemaphoreHandle_t scan_mutex = nullptr; // created on first scan, before any callback can run

static void scan_lock() {
    if (scan_mutex) xSemaphoreTake(scan_mutex, portMAX_DELAY);
}
static void scan_unlock() {
    if (scan_mutex) xSemaphoreGive(scan_mutex);
}

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
    scan_lock();
    if (!scan_active) {
        // The scan was cancelled (app closed) before this callback arrived: ignore it,
        // otherwise we'd write into results that were already cleared/freed.
        scan_unlock();
        return;
    }
    scan_results.clear();
    scan_results.reserve(results.getCount());
    for (int i = 0; i < results.getCount(); i++) {
        scan_results.push_back(copy_device(results.getDevice(i)));
    }
    scan_active = false;
    scan_complete = true;
    scan_unlock();
}
#endif

bool ble_scan_start(uint32_t duration_seconds, bool active) {
    if (!scan_mutex) scan_mutex = xSemaphoreCreateMutex();
    if (scan_active) return false;
#if defined(CONFIG_BLUEDROID_ENABLED)
    if (!ble_initialized) {
        BLEDevice::init("");
        ble_initialized = true;
    }
    BLEScan* scan = BLEDevice::getScan();
    scan->setActiveScan(active);
    scan->clearResults();

    scan_lock();
    scan_results.clear();
    scan_complete = false;
    scan_active = true; // set BEFORE start() so a fast callback can't be mistaken for a cancelled scan
    scan_unlock();

    bool ok = scan->start(duration_seconds, on_scan_complete, false);
    if (!ok) {
        scan_lock();
        scan_active = false;
        scan_unlock();
    }
    return ok;
#else
    (void)duration_seconds;
    (void)active;
    return false;
#endif
}

void ble_scan_cancel() {
    // NOTE: we deliberately do NOT call BLEDevice::deinit(true). deinit(true) releases the
    // Bluetooth controller memory, which ESP-IDF allows only ONCE per boot and which makes
    // BLE unusable until reboot (re-init fails with ESP_ERR_INVALID_STATE, a second release
    // aborts the firmware). BLE therefore stays initialised after its first use.
#if defined(CONFIG_BLUEDROID_ENABLED)
    if (ble_initialized && scan_active) BLEDevice::getScan()->stop(); // not under the lock: the callback takes it
#endif
    scan_lock();
#if defined(CONFIG_BLUEDROID_ENABLED)
    if (ble_initialized) BLEDevice::getScan()->clearResults();
#endif
    scan_active = false;
    scan_complete = false;
    scan_results.clear();
    scan_results.shrink_to_fit();
    scan_unlock();
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
    bool found = false;
    scan_lock();
    if (scan_complete && index >= 0 && index < (int)scan_results.size()) {
        result = scan_results[index];
        found = true;
    }
    scan_unlock();
    return found;
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