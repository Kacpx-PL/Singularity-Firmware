#include "ui_list.h"
#include <M5Cardputer.h>
#include "core/config.h"

#define LIST_VISIBLE_COUNT 3
#define LIST_MARGIN 8
#define LIST_SCROLLBAR_WIDTH 6

static std::vector<std::string> items;
static int selected = 0;
static int scroll_offset = 0;

void list_clear() {
    items.clear();
    selected = 0;
    scroll_offset = 0;
}

void list_add_item(const char* text) {
    items.push_back(text);
}

int list_get_selected_index() {
    return selected;
}

const char* list_get_selected_text() {
    if (selected < 0 || selected >= (int)items.size()) return "";
    return items[selected].c_str();
}

int list_handle_key(char key) {
    if (items.empty()) return -1;

    if (key == ';') { // up
        if (selected > 0) selected--;
        if (selected < scroll_offset) scroll_offset = selected;
    } else if (key == '.') { // down
        if (selected < (int)items.size() - 1) selected++;
        if (selected >= scroll_offset + LIST_VISIBLE_COUNT) {
            scroll_offset = selected - LIST_VISIBLE_COUNT + 1;
        }
    } else if (key == '\r') { // enter/select
        return selected;
    }

    return -1;
}

void list_draw() {
    int screenW = M5Cardputer.Display.width();
    int screenH = M5Cardputer.Display.height();

    int listTop = screenH / 2;
    int listHeight = screenH - listTop - LIST_MARGIN;
    int listLeft = LIST_MARGIN;
    int listWidth = screenW - (LIST_MARGIN * 2) - LIST_SCROLLBAR_WIDTH - 4;

    int itemHeight = listHeight / LIST_VISIBLE_COUNT;

    // Clear the list area background first
    M5Cardputer.Display.fillRect(listLeft, listTop, listWidth + LIST_SCROLLBAR_WIDTH + 4, listHeight, BLACK);

    // Background for the whole list area
    M5Cardputer.Display.drawRect(listLeft, listTop, listWidth + LIST_SCROLLBAR_WIDTH + 4, listHeight, THEME_COLOR);

    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setFont(&fonts::DejaVu12);

    int visible_end = min((int)items.size(), scroll_offset + LIST_VISIBLE_COUNT);

    for (int i = scroll_offset; i < visible_end; i++) {
        int row = i - scroll_offset;
        int itemY = listTop + (row * itemHeight);

        if (i == selected) {
            M5Cardputer.Display.fillRect(listLeft, itemY, listWidth, itemHeight, THEME_COLOR);
            M5Cardputer.Display.setTextColor(BLACK, THEME_COLOR);
        } else {
            M5Cardputer.Display.setTextColor(THEME_COLOR, BLACK);
        }

        M5Cardputer.Display.setCursor(listLeft + 4, itemY + (itemHeight / 2) - 6);
        M5Cardputer.Display.print(items[i].c_str());
    }

    M5Cardputer.Display.setFont(&fonts::Font0); // reset font, per your earlier rule

    // --- Scrollbar ---
    if ((int)items.size() > LIST_VISIBLE_COUNT) {
        int barX = listLeft + listWidth + 4;
        int barHeight = listHeight;

        // track
        M5Cardputer.Display.drawRect(barX, listTop, LIST_SCROLLBAR_WIDTH, barHeight, THEME_COLOR);

        // thumb — proportional to visible/total, positioned by scroll_offset
        int thumbHeight = max(4, (barHeight * LIST_VISIBLE_COUNT) / (int)items.size());
        int maxScroll = (int)items.size() - LIST_VISIBLE_COUNT;
        int thumbY = listTop + ((barHeight - thumbHeight) * scroll_offset) / maxScroll;

        M5Cardputer.Display.fillRect(barX, thumbY, LIST_SCROLLBAR_WIDTH, thumbHeight, THEME_COLOR);
    }
}