#include "msc_mode.h"
#include <M5Cardputer.h>
#include <USB.h>
#include <USBMSC.h>
#include <SD.h>
#include <SPI.h>
#include "config.h"

#define SD_SPI_SCK_PIN  40
#define SD_SPI_MISO_PIN 39
#define SD_SPI_MOSI_PIN 14
#define SD_SPI_CS_PIN   12

static USBMSC msc;
static volatile bool should_stop = false;

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize) {
    const uint32_t secSize = SD.sectorSize();
    if (secSize == 0) return -1;

    for (uint32_t x = 0; x < bufsize / secSize; ++x) {
        if (!SD.writeRAW(buffer + secSize * x, lba + x)) {
            return -1;
        }
    }
    return bufsize;
}

static int32_t onRead(uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize) {
    const uint32_t secSize = SD.sectorSize();
    if (secSize == 0) return -1;

    for (uint32_t x = 0; x < bufsize / secSize; ++x) {
        if (!SD.readRAW(reinterpret_cast<uint8_t*>(buffer) + (x * secSize), lba + x)) {
            return -1;
        }
    }
    return bufsize;
}

static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
    if (!start && load_eject) {
        should_stop = true;
        return false;
    }
    return true;
}

static void draw_status(const char* line1, const char* line2 = nullptr, uint16_t color = THEME_COLOR) {
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setTextColor(color);
    M5Cardputer.Display.setCursor(10, 40);
    M5Cardputer.Display.print(line1);

    if (line2 != nullptr) {
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setCursor(10, 70);
        M5Cardputer.Display.print(line2);
    }
}

void enter_msc_mode() {
    draw_status("USB Storage Mode", "Initializing SD...");

    SPI.begin(SD_SPI_SCK_PIN, SD_SPI_MISO_PIN, SD_SPI_MOSI_PIN, SD_SPI_CS_PIN);

    if (!SD.begin(SD_SPI_CS_PIN, SPI, 25000000)) {
        draw_status("USB Storage Mode", "SD init failed!", RED);
        delay(3000);
        return;
    }

    uint32_t sectorCount = SD.numSectors();
    uint32_t sectorSize = SD.sectorSize();

    should_stop = false;

    msc.vendorID("SG0MSC");
    msc.productID("SD Card");
    msc.productRevision("1.0");
    msc.onRead(onRead);
    msc.onWrite(onWrite);
    msc.onStartStop(onStartStop);
    msc.mediaPresent(true);
    msc.begin(sectorCount, sectorSize);

    USB.begin();

    draw_status("USB Storage Mode", "Plug into a computer.\n Press any key to exit.");

    // Wait for the triggering key to actually be released before
    // we start listening for an exit press — otherwise the same
    // held-down key that got us here instantly exits us too.
    while (M5Cardputer.Keyboard.isPressed()) {
        M5Cardputer.update();
        delay(20);
    }

    bool key_was_pressed = false;
    while (!should_stop) {
        M5Cardputer.update();

        bool key_is_pressed = M5Cardputer.Keyboard.isPressed();
        if (key_is_pressed && !key_was_pressed) {
            should_stop = true;
        }
        key_was_pressed = key_is_pressed;

        delay(50);
    }

    draw_status("Exiting USB mode...", nullptr);
    delay(1000);

    ESP.restart();
}