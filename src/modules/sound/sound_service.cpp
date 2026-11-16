#include "sound_service.h"
#include "../../core/config.h"
#include <M5Cardputer.h>
#include <math.h>

// Virtual speaker channels (the speaker has 8). Apps own channel 0; the firmware's own
// feedback beep uses channel 7 so it never cuts an app's tone.
static const int APP_CHANNEL = 0;
static const int SYSTEM_CHANNEL = 7;

static bool g_dirty = false;               // volume changed but not saved yet
static unsigned long g_last_change = 0;
static const unsigned long SAVE_DELAY_MS = 2000;   // don't hit the SD card on every key press

// IMPORTANT: M5Unified squares the master volume AND the channel volume itself
// (amplitude ~ (master/255)^2 * (channel/255)^2). So a byte value b means an amplitude of
// (b/255)^2, and to get a wanted amplitude `a` (0..1) we must send 255 * sqrt(a).
// An earlier version squared again on our side, which made amplitude ~ level^4: the first
// step was inaudible and every middle step was a +6 dB jump.
//
// Now the volume steps are evenly spaced in dB: step MAX is full scale (0 dB), step 1 is
// SG_VOLUME_FLOOR_DB below it, and the steps in between each change by the same number of
// dB, which is what the ear hears as equal steps. Tune SG_VOLUME_FLOOR_DB (more negative =
// quieter first step but bigger jumps; closer to 0 = audible first step, smaller range).
// Config stores the step, not the 0..255 value, so saved settings stay valid.
#define SG_VOLUME_FLOOR_DB (-40.0f)

uint8_t sound_master_from_level(int level) {
    if (level <= 0) return 0;
    if (level >= SG_VOLUME_MAX_LEVEL) return 255;
    float db = SG_VOLUME_FLOOR_DB * (float)(SG_VOLUME_MAX_LEVEL - level) / (float)(SG_VOLUME_MAX_LEVEL - 1);
    float byte_val = 255.0f * powf(10.0f, db / 40.0f);   // /40 = /20 (dB -> amplitude) then sqrt
    int v = (int)(byte_val + 0.5f);
    return (uint8_t)(v < 1 ? 1 : v);
}

static void apply_master() {
    M5.Speaker.setVolume(sound_master_from_level(VOLUME_LEVEL));
}

void sound_init() {
    if (VOLUME_LEVEL > SG_VOLUME_MAX_LEVEL) VOLUME_LEVEL = SG_VOLUME_MAX_LEVEL;
    apply_master();
}

int sound_get_level() {
    return VOLUME_LEVEL;
}

void sound_change_volume(int delta) {
    int level = (int)VOLUME_LEVEL + delta;
    if (level < 0) level = 0;
    if (level > SG_VOLUME_MAX_LEVEL) level = SG_VOLUME_MAX_LEVEL;
    VOLUME_LEVEL = (uint8_t)level;
    apply_master();

    g_dirty = true;
    g_last_change = millis();

    // Short beep at the new volume so the change can be heard (silent when muted).
    if (VOLUME_LEVEL > 0) {
        M5.Speaker.setChannelVolume(SYSTEM_CHANNEL, 255);
        M5.Speaker.tone(1200.0f, 40, SYSTEM_CHANNEL, true);
    }
}

void sound_tick() {
    if (g_dirty && millis() - g_last_change > SAVE_DELAY_MS) {
        g_dirty = false;
        config_save();
    }
}

void sound_stop() {
    M5.Speaker.stop(APP_CHANNEL);
}

bool sound_app_tone(float freq_hz, uint32_t duration_ms, float app_volume) {
    if (VOLUME_LEVEL == 0) return false;   // global mute

    if (app_volume < 0.0f) app_volume = 0.0f;
    if (app_volume > 1.0f) app_volume = 1.0f;
    // The speaker squares the channel byte itself, so sending v * 255 gives an amplitude of v^2:
    // 0.5 -> a quarter of the amplitude (-12 dB), which sounds roughly half as loud.
    M5.Speaker.setChannelVolume(APP_CHANNEL, (uint8_t)(app_volume * 255.0f + 0.5f));
    return M5.Speaker.tone(freq_hz, duration_ms, APP_CHANNEL, true);
}

bool sound_app_is_playing() {
    return M5.Speaker.isPlaying(APP_CHANNEL) > 0;
}
