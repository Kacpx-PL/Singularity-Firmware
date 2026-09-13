#pragma once

// Pins the firmware itself depends on — never allow Lua apps direct access.
#define SG_PIN_SD_SCK   40
#define SG_PIN_SD_MISO  39
#define SG_PIN_SD_MOSI  14
#define SG_PIN_SD_CS    12
#define SG_PIN_IR_TX    44
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

// Keyboard uses an I2C controller (TCA8418) on Cardputer-Adv, not raw GPIO
// pins in the same sense — but worth blocking its I2C pins too once confirmed.

inline bool pin_is_reserved(int pin) {
    switch (pin) {
        case SG_PIN_SD_SCK: case SG_PIN_SD_MISO: case SG_PIN_SD_MOSI: case SG_PIN_SD_CS:
        case SG_PIN_IR_TX:
        case SG_PIN_EXT_RESET: case SG_PIN_EXT_INT: case SG_PIN_EXT_BUSY:
        case SG_PIN_EXT_CS:
        case SG_PIN_EXT_SDA: case SG_PIN_EXT_SCL: case SG_PIN_EXT_TX: case SG_PIN_EXT_RX:
            return true;
        default:
            return false;
    }
}