#pragma once
#include <string>
#include <cstdint>

// A generic icon: raw 1-bit-per-pixel bitmap data plus its own dimensions.
// Row-major, MSB-first, each row padded to a whole byte boundary
struct Icon {
    const uint8_t* data;
    uint8_t width;
    uint8_t height;
};

// --- Status bar icons (small, 16x16) ---
extern const Icon icon_wifi_none;
extern const Icon icon_wifi_disconnected;
extern const Icon icon_wifi_weak;
extern const Icon icon_wifi_medium;
extern const Icon icon_wifi_strong;
extern const Icon icon_globe;
extern const Icon icon_cog;
extern const Icon icon_msdicon;
extern const Icon icon_iricon;
extern const Icon icon_command;
extern const Icon icon_code;
extern const Icon icon_file;
extern const Icon icon_bluetooth;
extern const Icon icon_tool;
extern const Icon icon_wifi;
extern const Icon icon_folder8;


const Icon* icon_by_name(const std::string& name);

void draw_icon_scaled(int centerX, int centerY, const Icon& icon, float scale, uint16_t color);