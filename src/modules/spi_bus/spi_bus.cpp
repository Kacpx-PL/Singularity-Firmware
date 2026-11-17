#include "spi_bus.h"
#include "../../core/pins.h"
#include <Arduino.h>
#include <SPI.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

static volatile SpiCapType g_cap_type = SPI_CAP_NONE;
static SemaphoreHandle_t g_mutex = nullptr;

// Drive a CS pin high BEFORE making it an output, so it never glitches low.
static void cs_hold_high(int pin) {
    digitalWrite(pin, HIGH);
    pinMode(pin, OUTPUT);
}

void spi_bus_early_init() {
#if SG_CAP_PROBE_ENABLED
    // G5 is a chip-select INPUT on both caps (NFC_CS / LoRa NSS): safe to drive.
    cs_hold_high(SG_PIN_NFC_CS);
    // G6 is the CC1101's CS but the SX1262's BUSY OUTPUT. Driving it could short against the
    // LoRa chip, so it only gets a weak pull-up (keeps a CC1101 CS high without fighting BUSY).
    pinMode(SG_PIN_CC_CS, INPUT_PULLUP);
#endif
}

#if SG_CAP_PROBE_ENABLED

// ---------------------------------------------------------------------------
// SX126x (LoRa cap): NSS = G5, BUSY = G6
// ---------------------------------------------------------------------------
static const uint8_t SX_CMD_READ_REGISTER = 0x1D;
static const uint8_t SX_CMD_SET_STANDBY   = 0x80;

static bool sx_wait_busy_low(uint32_t timeout_us) {
    uint32_t t0 = micros();
    while (digitalRead(SG_PIN_LORA_BUSY)) {
        if (micros() - t0 > timeout_us) return false;
    }
    return true;
}

// Reads the LoRa sync word registers 0x0740/0x0741. After a reset they hold 0x1424 (private) or
// 0x3444 (public), which makes a good identity check. The NSS falling edge also wakes the chip
// from sleep, after which BUSY drops once it is ready, so this works from any chip state.
static bool sx_read_sync_word(uint8_t& msb, uint8_t& lsb) {
    digitalWrite(SG_PIN_LORA_NSS, LOW);
    if (!sx_wait_busy_low(10000)) {        // nothing pulls BUSY low: no SX126x here
        digitalWrite(SG_PIN_LORA_NSS, HIGH);
        return false;
    }
    SPI.transfer(SX_CMD_READ_REGISTER);
    SPI.transfer(0x07);                    // address 0x0740
    SPI.transfer(0x40);
    SPI.transfer(0x00);                    // status byte
    msb = SPI.transfer(0x00);
    lsb = SPI.transfer(0x00);
    digitalWrite(SG_PIN_LORA_NSS, HIGH);
    return true;
}

static bool sx126x_probe() {
    uint8_t m1, l1, m2, l2;
    if (!sx_read_sync_word(m1, l1)) return false;
    if (!sx_read_sync_word(m2, l2)) return false;
    if (m1 != m2 || l1 != l2) return false;   // random/floating data doesn't repeat
    return (m1 == 0x14 && l1 == 0x24) || (m1 == 0x34 && l1 == 0x44);
}

// Stops anything the chip may still be doing (e.g. after a soft reset of the ESP32 while the
// cap stayed powered): STDBY_RC means no TX and no RX.
static void sx_set_standby() {
    digitalWrite(SG_PIN_LORA_NSS, LOW);
    if (sx_wait_busy_low(10000)) {
        SPI.transfer(SX_CMD_SET_STANDBY);
        SPI.transfer(0x00);                // STDBY_RC
    }
    digitalWrite(SG_PIN_LORA_NSS, HIGH);
}

// ---------------------------------------------------------------------------
// CC1101 (+ NFC cap): CC_CS = G6
// ---------------------------------------------------------------------------
static const uint8_t CC_SRES        = 0x30;          // reset
static const uint8_t CC_SPWD        = 0x39;          // power down once CS goes high
static const uint8_t CC_REG_PARTNUM = 0x30 | 0xC0;   // status registers need the burst bit
static const uint8_t CC_REG_VERSION = 0x31 | 0xC0;

// A CC1101 pulls SO low once its crystal is ready after CS goes low. Without a chip the line
// never does that on its own, so a timeout here also means "nothing there".
static bool cc_wait_ready(uint32_t timeout_us) {
    uint32_t t0 = micros();
    while (digitalRead(SG_PIN_SD_MISO)) {
        if (micros() - t0 > timeout_us) return false;
    }
    return true;
}

static bool cc_strobe(uint8_t cmd) {
    digitalWrite(SG_PIN_CC_CS, LOW);
    if (!cc_wait_ready(2000)) {
        digitalWrite(SG_PIN_CC_CS, HIGH);
        return false;
    }
    SPI.transfer(cmd);
    if (cmd == CC_SRES) cc_wait_ready(5000);   // SO goes low again when the reset is done
    digitalWrite(SG_PIN_CC_CS, HIGH);
    return true;
}

static bool cc_read_status(uint8_t reg, uint8_t& out) {
    digitalWrite(SG_PIN_CC_CS, LOW);
    if (!cc_wait_ready(2000)) {
        digitalWrite(SG_PIN_CC_CS, HIGH);
        return false;
    }
    SPI.transfer(reg);
    out = SPI.transfer(0);
    digitalWrite(SG_PIN_CC_CS, HIGH);
    return true;
}

static bool cc1101_probe() {
    uint8_t part1, ver1, part2, ver2;
    if (!cc_read_status(CC_REG_PARTNUM, part1) || !cc_read_status(CC_REG_VERSION, ver1)) return false;
    if (!cc_read_status(CC_REG_PARTNUM, part2) || !cc_read_status(CC_REG_VERSION, ver2)) return false;
    if (part1 != part2 || ver1 != ver2) return false;      // floating pins don't read the same twice
    return part1 == 0x00 && ver1 != 0x00 && ver1 != 0xFF; // PARTNUM is 0x00, VERSION is 0x14 (or 0x04)
}

#endif // SG_CAP_PROBE_ENABLED

void spi_bus_detect_cap() {
#if SG_CAP_PROBE_ENABLED
    // The SD card is deselected here (storage_init() finished) and SPI was begun by it.
    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
    SpiCapType type = SPI_CAP_NONE;

    // 1) LoRa first: it only needs G5 (driven) and G6 (read), so it can't hurt a CC1101 cap and
    //    it must be ruled in/out before G6 is ever driven as an output.
    if (sx126x_probe()) {
        type = SPI_CAP_LORA_SX126X;
        sx_set_standby();
    }
    // 2) CC1101, but only if G6 is not being held low by something (e.g. an SX126x BUSY pin we
    //    did not recognise). With nothing attached the pull-up keeps it high.
    else if (digitalRead(SG_PIN_CC_CS)) {
        cs_hold_high(SG_PIN_CC_CS);
        if (cc1101_probe()) {
            type = SPI_CAP_CC1101;
            cc_strobe(CC_SRES);   // known state
            cc_strobe(CC_SPWD);   // sleep: transmitter off, no stray RF
        }
    }
    SPI.endTransaction();

    if (type != SPI_CAP_NONE) {
        g_mutex = xSemaphoreCreateRecursiveMutex();
        if (g_mutex) g_cap_type = type;
    }

    switch (g_cap_type) {
        case SPI_CAP_LORA_SX126X:
            pinMode(SG_PIN_LORA_BUSY, INPUT);   // BUSY is driven by the chip, drop our pull-up
            break;
        case SPI_CAP_CC1101:
            break;                              // G5 and G6 stay high outputs (idle chip selects)
        default:
            pinMode(SG_PIN_NFC_CS, INPUT);      // nothing attached: release everything
            pinMode(SG_PIN_CC_CS, INPUT);
            break;
    }

    Serial.printf("[SPIBUS] cap: %s%s\n", spi_bus_cap_name(),
                  g_cap_type != SPI_CAP_NONE ? " (SD/radio bus locking enabled)" : "");
#endif
}

SpiCapType spi_bus_cap_type() {
    return g_cap_type;
}

const char* spi_bus_cap_name() {
    switch (g_cap_type) {
        case SPI_CAP_CC1101:      return "CC1101";
        case SPI_CAP_LORA_SX126X: return "SX126x";
        default:                  return "none";
    }
}

bool spi_bus_cap_present() {
    return g_cap_type != SPI_CAP_NONE;
}

bool spi_bus_lock() {
    if (g_cap_type == SPI_CAP_NONE) return false;
    return xSemaphoreTakeRecursive(g_mutex, portMAX_DELAY) == pdTRUE;
}

void spi_bus_unlock() {
    if (g_cap_type == SPI_CAP_NONE) return;
    xSemaphoreGiveRecursive(g_mutex);
}
