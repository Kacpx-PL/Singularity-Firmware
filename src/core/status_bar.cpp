#include "status_bar.h"
#include "../modules/wifi/wifi.h"
#include "../modules/storage/storage_service.h"
#include "../modules/ntp/ntp_service.h"
#include "../modules/sound/sound_service.h"
#include "../modules/spi_bus/spi_bus.h"
#include "icons.h"
#include <M5Cardputer.h>
#include "config.h"

const int STATUS_BAR_HEIGHT = 20;

static const unsigned long VOLUME_POPUP_MS = 1500;
static bool g_volume_popup = false;
static unsigned long g_volume_popup_until = 0;

static void draw_icon(int x, int y, const Icon& icon, uint16_t color) {
    M5Canvas canvas(&M5Cardputer.Display);
    canvas.setColorDepth(1);
    canvas.createSprite(icon.width, icon.height);
    canvas.setPaletteColor(0, BLACK);
    canvas.setPaletteColor(1, color);
    canvas.fillSprite(0);
    canvas.drawBitmap(0, 0, icon.data, icon.width, icon.height, 1);
    canvas.pushSprite(x, y);
    canvas.deleteSprite();
}

// Volume view: "Vol" + 10 segments + number (or MUTE). Replaces the normal content while shown.
static void draw_volume_popup() {
    const int level = sound_get_level();
    const int segs = SG_VOLUME_MAX_LEVEL;

    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setCursor(8, 6);
    M5Cardputer.Display.print("Vol");

    for (int i = 0; i < segs; i++) {
        int x = 40 + i * 16;
        if (i < level) {
            M5Cardputer.Display.fillRect(x, 6, 13, 8, THEME_COLOR);
        } else {
            M5Cardputer.Display.drawRect(x, 6, 13, 8, THEME_COLOR);
        }
    }

    char label[8];
    if (level == 0) snprintf(label, sizeof(label), "MUTE");
    else snprintf(label, sizeof(label), "%d", level);
    int w = M5Cardputer.Display.textWidth(label);
    M5Cardputer.Display.setCursor(M5Cardputer.Display.width() - w - 5, 6);
    M5Cardputer.Display.print(label);
}

void status_bar_draw() {
    M5Cardputer.Display.drawRect(0, 0, M5Cardputer.Display.width(), STATUS_BAR_HEIGHT, THEME_COLOR);
    M5Cardputer.Display.fillRect(1, 1, M5Cardputer.Display.width() - 2, STATUS_BAR_HEIGHT - 2, BLACK);

    if (g_volume_popup) {
        draw_volume_popup();
        return;
    }
    
    // Battery percentage
    int battery = M5Cardputer.Power.getBatteryLevel();
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(WHITE);

    char batteryText[8];
    snprintf(batteryText, sizeof(batteryText), "%d%%", battery);

    int textW = M5Cardputer.Display.textWidth(batteryText);
    M5Cardputer.Display.setCursor(M5Cardputer.Display.width() - textW - 5, 6);
    M5Cardputer.Display.print(batteryText);

    // Clock (centered). Time comes from the firmware's NTP service; hidden until the clock is set.
    struct tm now;
    if (ntp_get_local_time(&now)) {
        char clockText[8];
        snprintf(clockText, sizeof(clockText), "%02d:%02d", now.tm_hour, now.tm_min);
        int clockW = M5Cardputer.Display.textWidth(clockText);
        M5Cardputer.Display.setCursor((M5Cardputer.Display.width() - clockW) / 2, 6);
        M5Cardputer.Display.print(clockText);
    }

    // WiFi icon
    const Icon* icon = &icon_wifi_none;
    if (wifi_is_connected()) {
        int rssi = wifi_get_rssi();
        if (rssi > -60) icon = &icon_wifi_strong;
        else if (rssi > -75) icon = &icon_wifi_medium;
        else icon = &icon_wifi_weak;
    } else {
        icon = &icon_wifi_disconnected;
    }

    draw_icon(5, 2, *icon, THEME_COLOR);

    // SD Card icon (only show if detected)
    if (storage_is_ready()) {
        draw_icon(25, 2, icon_msdicon, THEME_COLOR);
    }

    // Cap icon (CC1101/NFC or LoRa cap, detected once at boot)
    if (spi_bus_cap_present()) {
        draw_icon(45, 2, icon_module, THEME_COLOR);
    }
}

void status_bar_show_volume() {
    g_volume_popup = true;
    g_volume_popup_until = millis() + VOLUME_POPUP_MS;
    status_bar_draw();
}

void status_bar_tick() {
    // (long) cast keeps this correct when millis() wraps
    if (g_volume_popup && (long)(millis() - g_volume_popup_until) >= 0) {
        g_volume_popup = false;
        status_bar_draw();
    }
}