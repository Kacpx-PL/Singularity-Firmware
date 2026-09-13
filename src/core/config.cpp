#include "config.h"
#include "paths.h"
#include "lua_core.h"
#include "../modules/storage/storage_service.h"

#define SG_BOOT_DELAY_MIN 1000  //1s min
#define SG_BOOT_DELAY_MAX 10000 //10s max

uint16_t THEME_COLOR = 0x07E0;   // default green (565 format), used if no config exists
uint32_t BOOT_DELAY_MS = 3000;

static uint32_t clamp_boot_delay(uint32_t value) {
    if (value < SG_BOOT_DELAY_MIN) return SG_BOOT_DELAY_MIN;
    if (value > SG_BOOT_DELAY_MAX) return SG_BOOT_DELAY_MAX;
    return value;
}

void config_load() {
    char buf[512];
    if (!storage_read(SG_SYSTEM_DIR "/config.lua", buf, sizeof(buf))) {
        return; // no config yet — keep defaults, don't error
    }

    LuaConfig cfg = lua_core_load_config(buf);

    if (cfg.has_theme_color) THEME_COLOR = cfg.theme_color;
    if (cfg.has_boot_delay_ms) BOOT_DELAY_MS = clamp_boot_delay(cfg.boot_delay_ms);
}

void config_save() {
    std::string out = "return {\n";
    out += "    theme_color = " + std::to_string(THEME_COLOR) + ",\n";
    out += "    boot_delay_ms = " + std::to_string(BOOT_DELAY_MS) + ",\n";
    out += "}\n";
    storage_write(SG_SYSTEM_DIR "/config.lua", out.c_str());
}

void config_set_theme_color(uint16_t color) {
    THEME_COLOR = color;
    config_save();
}

void config_set_boot_delay_ms(uint32_t ms) {
    BOOT_DELAY_MS = clamp_boot_delay(ms);
    config_save();
}