#pragma once
#include <cstdint>

// Shared SPI bus: SCK 40 / MISO 39 / MOSI 14 is used by the SD card AND an optional cap on the
// expansion header: the CC1101 + NFC cap, or the LoRa (SX1262) cap. Devices are told apart only
// by their chip-select pin, so:
//
//   * every unused CS must sit HIGH, always            -> spi_bus_early_init()
//   * only one task may use the bus at a time           -> SpiBusGuard
//   * each device uses its own SPISettings              -> SPI.beginTransaction() by the driver
//
// Without a cap NOTHING here does anything: the pins are released again and the guard is a no-op.
//
// Header pin roles differ per cap:
//   G5: NFC_CS (CC1101 cap)  | NSS  (LoRa cap)    both are inputs on the cap side -> safe to drive
//   G6: CC_CS  (CC1101 cap)  | BUSY (LoRa cap)    BUSY is an OUTPUT of the chip -> only driven once
//                                                  the cap is known to be a CC1101
//
// Radio/NFC drivers must do, for every transfer sequence:
//     SpiBusGuard bus;                                   // waits for a running SD operation
//     SPI.beginTransaction(SPISettings(hz, MSBFIRST, mode));
//     digitalWrite(cs, LOW); ... transfer ...; digitalWrite(cs, HIGH);
//     SPI.endTransaction();
// Never call SPI.begin() again and never touch the bus from an ISR (set a flag instead).
// Keep the CC1101's GDO1/SO pin at its default (IOCFG1 = 0x2E, high impedance while CS is high),
// it shares MISO with the SD card.

// Set to 0 to compile the whole cap support out (the cap pins are then never touched).
#ifndef SG_CAP_PROBE_ENABLED
#define SG_CAP_PROBE_ENABLED 1
#endif

enum SpiCapType : uint8_t {
    SPI_CAP_NONE = 0,
    SPI_CAP_CC1101,        // CC1101 + NFC cap
    SPI_CAP_LORA_SX126X,   // LoRa cap (SX1262 / compatible)
};

// Call in setup() BEFORE storage_init(): drives G5 high (and leaves G6 as a pulled-up input) so a
// floating CS can't answer the SD card's traffic. Never drives G6, so it is safe with every cap.
void spi_bus_early_init();

// Call in setup() AFTER storage_init(): identifies the cap.
//   LoRa cap found   -> SX126x put in standby (no TX/RX), bus lock enabled
//   CC1101 found     -> reset and put to sleep (no RF emission), bus lock enabled
//   nothing found    -> the pins are released
void spi_bus_detect_cap();

SpiCapType spi_bus_cap_type();
const char* spi_bus_cap_name();   // "none", "CC1101", "SX126x"
bool spi_bus_cap_present();

// Recursive lock. Returns true if it really took the lock (always false while no cap is present).
bool spi_bus_lock();
void spi_bus_unlock();

// RAII helper: hold it for the duration of any SD or radio/NFC bus use.
struct SpiBusGuard {
    SpiBusGuard() : held_(spi_bus_lock()) {}
    ~SpiBusGuard() { if (held_) spi_bus_unlock(); }
    SpiBusGuard(const SpiBusGuard&) = delete;
    SpiBusGuard& operator=(const SpiBusGuard&) = delete;
private:
    bool held_;
};
