#pragma once
#include <cstdint>

// Sound: tones only for now.
//
// Volume model (same idea as a phone):
//  * GLOBAL volume: a user-facing step 0..SG_VOLUME_MAX_LEVEL (0 = mute), changed with Fn + -/=,
//    saved in config.lua. It is the speaker's master volume. Apps can read it but never change it.
//  * APP volume: 0.0..1.0 passed per tone by the app; it scales *within* the global volume,
//    so an app can be quieter than the user's setting but never louder.
//
// Both go through a perceptual curve (see sound_master_from_level) because loudness is
// roughly logarithmic: equal steps in amplitude do not sound like equal steps in volume.

#define SG_VOLUME_MAX_LEVEL 10

void sound_init();                         // apply the saved volume (call after config_load)
void sound_tick();                         // saves a changed volume once it has settled
void sound_stop();                         // stop everything an app started (called when an app closes)

int  sound_get_level();                    // global volume step 0..SG_VOLUME_MAX_LEVEL
void sound_change_volume(int delta);       // hotkey: +1 / -1, clamps, beeps for feedback, saves later

// App tone on the app channel; a new tone replaces the previous one.
// Returns false if the global volume is muted (nothing is played) or the speaker refused it.
bool sound_app_tone(float freq_hz, uint32_t duration_ms, float app_volume);
bool sound_app_is_playing();

// Volume step -> speaker master volume (0..255). The single place to tune the curve.
uint8_t sound_master_from_level(int level);
