#pragma once

// Pins the firmware itself depends on - never allow Lua apps direct access.
// (Cardputer / Cardputer-Adv, ESP32-S3.)

// SD card (SPI)
#define SG_PIN_SD_SCK   40
#define SG_PIN_SD_MISO  39
#define SG_PIN_SD_MOSI  14
#define SG_PIN_SD_CS    12

// IR: TX LED on the board, RX on the Grove port (G1) used by the IR capture module
#define SG_PIN_IR_TX    44
#define SG_PIN_IR_RX     1

// Grove port (HY2.0-4P): G1 = SCL (shared with the IR RX pin above), G2 = SDA.
// The i2c.* Lua API borrows these two pins and pauses IR receive while it is active.
#define SG_PIN_GROVE_SCL   1
#define SG_PIN_GROVE_SDA   2

// Expansion header
#define SG_PIN_EXT_RESET   3
#define SG_PIN_EXT_INT     4
#define SG_PIN_EXT_BUSY    6
#define SG_PIN_EXT_SCK    40
#define SG_PIN_EXT_MOSI   14
#define SG_PIN_EXT_MISO   39
#define SG_PIN_EXT_CS      5
#define SG_PIN_EXT_SDA     8
#define SG_PIN_EXT_SCL     9
#define SG_PIN_EXT_TX     13
#define SG_PIN_EXT_RX     15

// Keyboard controller (TCA8418) interrupt line; its I2C bus is SDA 8 / SCL 9 (see above)
#define SG_PIN_KB_INT     11

// Native USB (serial console / mass storage mode): driving these kills USB
#define SG_PIN_USB_DM     19
#define SG_PIN_USB_DP     20

// Display (ST7789): 33 RST, 34 DC, 35 MOSI, 36 SCK, 37 CS, 38 backlight
#define SG_PIN_LCD_FIRST  33
#define SG_PIN_LCD_LAST   38

// GPIO 26..32 are wired to the module's SPI flash on the ESP32-S3
#define SG_PIN_FLASH_FIRST 26
#define SG_PIN_FLASH_LAST  32

// Audio (speaker / microphone I2S)
#define SG_PIN_I2S_BCLK   41
#define SG_PIN_I2S_DOUT   42
#define SG_PIN_I2S_WS     43
#define SG_PIN_I2S_DIN    46

// The ESP32-S3 only has GPIO 0..21 and 26..48
inline bool pin_is_valid(int pin) {
    return (pin >= 0 && pin <= 21) || (pin >= 26 && pin <= 48);
}

inline bool pin_is_reserved(int pin) {
    // ranges first (also covers the display and flash pins)
    if (pin >= SG_PIN_FLASH_FIRST && pin <= SG_PIN_FLASH_LAST) return true;
    if (pin >= SG_PIN_LCD_FIRST && pin <= SG_PIN_LCD_LAST) return true;

    switch (pin) {
        case SG_PIN_SD_SCK: case SG_PIN_SD_MISO: case SG_PIN_SD_MOSI: case SG_PIN_SD_CS:
        case SG_PIN_IR_TX: case SG_PIN_IR_RX:
        case SG_PIN_EXT_RESET: case SG_PIN_EXT_INT: case SG_PIN_EXT_BUSY:
        case SG_PIN_EXT_CS:
        case SG_PIN_EXT_SDA: case SG_PIN_EXT_SCL: case SG_PIN_EXT_TX: case SG_PIN_EXT_RX:
        case SG_PIN_KB_INT:
        case SG_PIN_USB_DM: case SG_PIN_USB_DP:
        case SG_PIN_I2S_BCLK: case SG_PIN_I2S_DOUT: case SG_PIN_I2S_WS: case SG_PIN_I2S_DIN:
            return true;
        default:
            return false;
    }
}
