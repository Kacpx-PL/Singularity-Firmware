#include "i2c_service.h"
#include "../../core/pins.h"
#include "../ir/ir_service.h"
#include <M5Cardputer.h>

static bool g_active = false;
static uint32_t g_freq = 100000;

bool i2c_begin(uint32_t freq_hz) {
    if (g_active) {          // already running: just change the clock
        g_freq = freq_hz;
        return true;
    }

    ir_receive_suspend();    // GPIO1 is the IR RX pin
    if (!M5.Ex_I2C.begin(I2C_NUM_0, SG_PIN_GROVE_SDA, SG_PIN_GROVE_SCL)) {
        ir_receive_restore();
        return false;
    }

    g_freq = freq_hz;
    g_active = true;
    return true;
}

void i2c_end() {
    if (!g_active) return;
    M5.Ex_I2C.release();
    g_active = false;
    ir_receive_restore();
}

bool i2c_is_active() {
    return g_active;
}

// The chains below deliberately mirror M5Unified's own writeRegister/readRegister:
// stop() is only reached when every earlier step succeeded.

bool i2c_write(uint8_t addr, const uint8_t* data, size_t len) {
    if (!g_active) return false;
    return M5.Ex_I2C.start(addr, false, g_freq)
        && (len == 0 || M5.Ex_I2C.write(data, len))   // len == 0 is an "is anyone there" ping
        && M5.Ex_I2C.stop();
}

bool i2c_read(uint8_t addr, uint8_t* out, size_t len) {
    if (!g_active || len == 0) return false;
    return M5.Ex_I2C.start(addr, true, g_freq)
        && M5.Ex_I2C.read(out, len, true)             // NACK the last byte, as the I2C spec wants
        && M5.Ex_I2C.stop();
}

bool i2c_write_read(uint8_t addr, const uint8_t* data, size_t wlen, uint8_t* out, size_t rlen) {
    if (!g_active || rlen == 0) return false;
    return M5.Ex_I2C.start(addr, false, g_freq)
        && (wlen == 0 || M5.Ex_I2C.write(data, wlen))
        && M5.Ex_I2C.restart(addr, true, g_freq)
        && M5.Ex_I2C.read(out, rlen, true)
        && M5.Ex_I2C.stop();
}

int i2c_scan(uint8_t* found, int max) {
    if (!g_active) return 0;
    int count = 0;
    // 0x00..0x07 are reserved, and M5Unified notes that probing them hangs the ESP32-S3.
    for (int addr = 0x08; addr < 0x78 && count < max; addr++) {
        if (M5.Ex_I2C.scanID((uint8_t)addr, g_freq)) {
            found[count++] = (uint8_t)addr;
        }
    }
    return count;
}
