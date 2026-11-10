#include <M5Cardputer.h>
#include <cstdlib>
#include <cstring>
#include "core/launch_app.h"
#include "core/input.h"
#include "core/menu.h"
#include "core/status_bar.h"
#include "core/lua_core.h"
#include "modules/storage/storage_service.h"
#include "modules/wifi/wifi.h"
#include "core/paths.h"
#include "core/config.h"
#include "modules/ir/ir_service.h"
#include "modules/espnow/espnow_service.h"

// Loads /singularity/system/wifi_networks.lua.
// - The buffer is sized to the file (a fixed 1 KB buffer used to truncate files with
//   ~a dozen networks, which then failed to parse and got overwritten by the next save).
// - If the file exists but can't be understood, saving is blocked so the user's
//   networks are never replaced by an empty list.
static void load_wifi_settings() {
    const char* path = SG_SYSTEM_DIR "/wifi_networks.lua";
    const size_t max_file_size = 16 * 1024;

    size_t size = storage_get_file_size(path);
    if (size == 0) {
        Serial.println("No wifi_networks.lua found - skipping auto-connect");
        return;
    }
    if (size > max_file_size) {
        Serial.println("[WIFI] wifi_networks.lua is too large to load - saving disabled");
        wifi_block_saving(true);
        return;
    }

    char* buf = (char*)malloc(size + 1);
    if (!buf) {
        Serial.println("[WIFI] out of memory loading wifi_networks.lua - saving disabled");
        wifi_block_saving(true);
        return;
    }

    if (storage_read(path, buf, size + 1)) {
        WifiConfig wcfg = lua_core_load_wifi_config(buf);

        // The file mentions networks but none were parsed => it's broken, don't clobber it.
        if (wcfg.networks.empty() && strstr(buf, "ssid")) {
            Serial.println("[WIFI] wifi_networks.lua could not be parsed - saving disabled");
            wifi_block_saving(true);
        }

        // Apply the file's settings (known networks AND the autoconnect flag) without saving.
        wifi_apply_config(wcfg.networks, wcfg.autoconnect);
        if (wcfg.autoconnect) {
            wifi_start_auto_connect();
        }
    } else {
        Serial.println("[WIFI] could not read wifi_networks.lua - saving disabled");
        wifi_block_saving(true);
    }
    free(buf);
}

void setup() {
    Serial.begin(115200);
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);

    bool sd_ok = storage_init();
    if (!sd_ok) {
        Serial.println("SD card init failed - continuing without SD");
    } else {
        // Wait for SD filesystem to be ready before attempting to access it
        if (!storage_wait_ready(3000)) {
            Serial.println("SD card filesystem timeout");
        }
    }

    lua_core_init();

    if (!espnow_init()) {
        Serial.println("ESP-NOW init failed");
    }

    config_load();

    ir_init();

    lua_core_run_string("math.randomseed(sys.millis())");

    boot_screen();

    menu_init();
    status_bar_draw();

    load_wifi_settings();
}


void loop() {
    M5Cardputer.update();
    char key = input_read();
    menu_update(key);
}
