#pragma once
#include <cstddef>
#include <cstdint>

// Crude I2C master on the Grove port (SDA = GPIO2, SCL = GPIO1) using M5Unified's Ex_I2C.
// The internal bus (keyboard / IMU, GPIO 8/9) is separate and never touched.
//
// GPIO1 is also the IR receiver pin, so i2c_begin() suspends IR receive and i2c_end()
// restores it.

#define SG_I2C_MAX_BYTES 255

bool i2c_begin(uint32_t freq_hz);
void i2c_end();                 // safe to call when not active
bool i2c_is_active();

// All return false on NACK / bus error. Addresses must be 0x08..0x77 (the caller checks).
bool i2c_write(uint8_t addr, const uint8_t* data, size_t len);
bool i2c_read(uint8_t addr, uint8_t* out, size_t len);
bool i2c_write_read(uint8_t addr, const uint8_t* data, size_t wlen, uint8_t* out, size_t rlen); // repeated start

// Fills `found` with responding addresses (up to max), returns how many.
int i2c_scan(uint8_t* found, int max);
