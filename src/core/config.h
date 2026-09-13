#pragma once
#include <cstdint>

// Runtime-configurable firmware settings, loaded from
// /singularity/system/config.lua at boot, saved back on change.

extern uint16_t THEME_COLOR;
extern uint32_t BOOT_DELAY_MS;

void config_load();
void config_save();
void config_set_theme_color(uint16_t color);
void config_set_boot_delay_ms(uint32_t ms);