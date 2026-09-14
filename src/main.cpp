#include <M5Cardputer.h>
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

void setup() {
    Serial.begin(115200);

    auto cfg = M5.config();
    M5Cardputer.begin(cfg);

    bool sd_ok = storage_init();
    if (!sd_ok) {
        Serial.println("SD card init failed — continuing without SD");
    } else {
        // Wait for SD filesystem to be ready before attempting to access it
        if (!storage_wait_ready(3000)) {
            Serial.println("SD card filesystem timeout");
        }
    }
    lua_core_init();
    config_load();
    ir_init();
    lua_core_run_string("math.randomseed(millis())");

    boot_screen();

    menu_init();
    status_bar_draw();


    char buf[1024];
    if (storage_read(SG_SYSTEM_DIR "/wifi_networks.lua", buf, sizeof(buf))) {
        WifiConfig cfg = lua_core_load_wifi_config(buf);
        wifi_set_known_networks(cfg.networks);
        if (cfg.autoconnect) {
            wifi_start_auto_connect();
        }
    } else {
        Serial.println("No wifi_networks.lua found — skipping auto-connect");
}
}


void loop() {
    M5Cardputer.update();
    char key = input_read();
    menu_update(key);
}