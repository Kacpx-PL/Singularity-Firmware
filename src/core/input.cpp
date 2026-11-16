#include "input.h"
#include <M5Cardputer.h>

char input_read() {

    if (!M5Cardputer.Keyboard.isChange()) {
        return 0;
    }

    if (!M5Cardputer.Keyboard.isPressed()) {
        return 0;
    }

    auto status = M5Cardputer.Keyboard.keysState();

    // Enter
    if (status.enter) {
        return '\r';
    }

    if (status.opt) {
        return SG_KEY_OPT;
    }

    if (status.del) {
        return '\b';
    }
    // Escape: Fn + `
    // In this keyboard library, Fn is reported separately,
    // while ` is the printable key.
    if (status.fn && !status.word.empty() && status.word[0] == '`') {
        return '\n';
    }

    // Volume: Fn + - / Fn + =  (the + sign lives on the = key, so accept both)
    if (status.fn && !status.word.empty()) {
        char c = status.word[0];
        if (c == '=' || c == '+') return SG_KEY_VOL_UP;
        if (c == '-' || c == '_') return SG_KEY_VOL_DOWN;
    }

    // Normal printable key
    if (status.word.empty()) {
        return 0;
    }

    return status.word[0];
}