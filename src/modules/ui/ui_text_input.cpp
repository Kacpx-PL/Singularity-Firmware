#include "ui_text_input.h"
#include <M5Cardputer.h>
#include <string>
#include "core/config.h"

static std::string prompt_text;
static std::string value;
static bool mask_input = false;

void text_input_start(const char* prompt, bool mask) {
    prompt_text = prompt;
    value.clear();
    mask_input = mask;
}

const char* text_input_get_value() {
    return value.c_str();
}

int text_input_handle_key(char key) {
    if (key == '\r') {
        return 1; // submitted
    } else if (key == '\b') {
        if (!value.empty()) {
            value.pop_back();
        } else {
            return 2; // backspace on empty = cancel
        }
    } else if (key >= 32 && key <= 126) {
        value += key;
    }
    return 0;
}
void text_input_draw() {
    int screenW = M5Cardputer.Display.width();
    int screenH = M5Cardputer.Display.height();

    M5Cardputer.Display.fillRect(0, screenH / 2, screenW, screenH / 2, BLACK);
    M5Cardputer.Display.drawRect(8, screenH / 2, screenW - 16, screenH / 2 - 8, THEME_COLOR);

    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(THEME_COLOR, BLACK);
    M5Cardputer.Display.setCursor(12, screenH / 2 + 6);
    M5Cardputer.Display.print(prompt_text.c_str());

    std::string display_value = mask_input ? std::string(value.size(), '*') : value;
    M5Cardputer.Display.setCursor(12, screenH / 2 + 24);
    M5Cardputer.Display.print(display_value.c_str());
    M5Cardputer.Display.print("_"); // cursor indicator
}

