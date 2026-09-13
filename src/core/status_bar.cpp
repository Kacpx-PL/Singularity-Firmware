#include "status_bar.h"
#include "../modules/wifi/wifi.h"
#include "../modules/storage/storage_service.h"
#include "icons.h"
#include <M5Cardputer.h>
#include "config.h"

const int STATUS_BAR_HEIGHT = 20;

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

void status_bar_draw() {
    M5Cardputer.Display.drawRect(0, 0, M5Cardputer.Display.width(), STATUS_BAR_HEIGHT, THEME_COLOR);
    M5Cardputer.Display.fillRect(1, 1, M5Cardputer.Display.width() - 2, STATUS_BAR_HEIGHT - 2, BLACK);
    
    // Battery percentage
    int battery = M5Cardputer.Power.getBatteryLevel();
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(WHITE);

    char batteryText[8];
    snprintf(batteryText, sizeof(batteryText), "%d%%", battery);

    int textW = M5Cardputer.Display.textWidth(batteryText);
    M5Cardputer.Display.setCursor(M5Cardputer.Display.width() - textW - 5, 6);
    M5Cardputer.Display.print(batteryText);

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
}