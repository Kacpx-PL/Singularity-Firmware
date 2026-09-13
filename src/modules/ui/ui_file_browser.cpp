#include "ui_file_browser.h"
#include "core/config.h"
#include "core/status_bar.h"
#include "core/icons.h"
#include "modules/storage/storage_service.h"
#include <M5Cardputer.h>
#include <vector>

struct Entry {
    std::string name;
    bool is_dir;
};

static std::string root;
static std::string current_rel_path; // relative to root, e.g. "" or "subfolder"
static std::vector<Entry> entries;
static int selected = 0;
static int scroll_offset = 0;
static bool folder_mode = false;

#define BROWSER_VISIBLE_COUNT 8

static std::string full_path() {
    if (current_rel_path.empty()) return root;
    return root + "/" + current_rel_path;
}

static void refresh_entries() {
    entries.clear();
    selected = 0;
    scroll_offset = 0;

    std::string path = full_path();

    if (!current_rel_path.empty()) {
        entries.push_back({"..", true});
    }

    if (folder_mode) {
        entries.push_back({"[Select This Folder]", false});
    }

    auto raw_entries = storage_list_dir_entries(path.c_str());
    for (auto& e : raw_entries) {
        if (folder_mode && !e.is_dir) continue; // skip files entirely in folder-select mode
        if (!folder_mode && !e.is_dir && e.name.size() > 3 &&
            e.name.compare(e.name.size() - 3, 3, ".ir") != 0) continue; // keep only .ir files when browsing files
        entries.push_back({e.name, e.is_dir});
    }
}


void file_browser_start(const char* root_path, bool folder_select_mode) {
    root = root_path;
    current_rel_path = "";
    folder_mode = folder_select_mode;
    refresh_entries();
}

int file_browser_handle_key(char key) {
    if (entries.empty()) return 0;

    if (key == ';') {
        if (selected > 0) selected--;
        if (selected < scroll_offset) scroll_offset = selected;
    } else if (key == '.') {
        if (selected < (int)entries.size() - 1) selected++;
        if (selected >= scroll_offset + BROWSER_VISIBLE_COUNT) {
            scroll_offset = selected - BROWSER_VISIBLE_COUNT + 1;
        }
    } else if (key == '\r') {
        Entry& e = entries[selected];

        if (e.is_dir) {
            if (e.name == "..") {
                size_t pos = current_rel_path.find_last_of('/');
                if (pos == std::string::npos) {
                    current_rel_path = "";
                } else {
                    current_rel_path = current_rel_path.substr(0, pos);
                }
            } else {
                if (current_rel_path.empty()) current_rel_path = e.name;
                else current_rel_path += "/" + e.name;
            }
            refresh_entries();
        } else {
            return 1; // file selected
        }
    }

    return 0;
}

const char* file_browser_get_selected_path() {
    static std::string result;
    if (folder_mode && entries[selected].name == "[Select This Folder]") {
        result = full_path();
    } else {
        result = full_path() + "/" + entries[selected].name;
    }
    return result.c_str();
}

bool file_browser_selected_is_dir() {
    if (entries.empty()) return false;
    return entries[selected].is_dir;
}

void file_browser_draw() {
    int screenW = M5Cardputer.Display.width();
    int screenH = M5Cardputer.Display.height();

    int top = STATUS_BAR_HEIGHT;
    int pathBarHeight = 14;

    // Path breadcrumb bar
    M5Cardputer.Display.fillRect(0, top, screenW, pathBarHeight, BLACK);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(THEME_COLOR, BLACK);
    M5Cardputer.Display.setCursor(4, top + 3);

    std::string display_path = "/" + current_rel_path;
    M5Cardputer.Display.print(display_path.c_str());

    int listTop = top + pathBarHeight;
    int listHeight = screenH - listTop;
    int itemHeight = listHeight / BROWSER_VISIBLE_COUNT;

    int visible_end = min((int)entries.size(), scroll_offset + BROWSER_VISIBLE_COUNT);

    for (int i = scroll_offset; i < visible_end; i++) {
        int row = i - scroll_offset;
        int itemY = listTop + (row * itemHeight);

        if (i == selected) {
            M5Cardputer.Display.fillRect(0, itemY, screenW - 6, itemHeight, THEME_COLOR);
            M5Cardputer.Display.setTextColor(BLACK, THEME_COLOR);
        } else {
            M5Cardputer.Display.setTextColor(THEME_COLOR, BLACK);
        }

        int textX = 4;

        if (entries[i].is_dir) {
            uint16_t icon_color = (i == selected) ? BLACK : THEME_COLOR;
            draw_icon_scaled(4 + 4, itemY + (itemHeight / 2), icon_folder8, 1.0f, icon_color);
            textX = 16; // shift text right to make room for the icon
        }

        M5Cardputer.Display.setCursor(textX, itemY + 1);
        M5Cardputer.Display.print(entries[i].name.c_str());
    }
}