#pragma once

#define SG_KEY_OPT ((char)0x01)

// Fn + - / Fn + = : handled by the firmware (global volume), never forwarded to apps.
#define SG_KEY_VOL_DOWN ((char)0x02)
#define SG_KEY_VOL_UP   ((char)0x03)

char input_read();