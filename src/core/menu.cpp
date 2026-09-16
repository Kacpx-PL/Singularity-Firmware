#include "menu.h"
#include "status_bar.h"
#include "app.h"
#include "lua_app.h"
#include "lua_core.h"
#include "icons.h"
#include "paths.h"
#include <M5Cardputer.h>
#include <vector>
#include <string>
#include <deque>
#include "../core_apps/generated/core_apps_bundle.h"
#include "../modules/storage/storage_service.h"
#include "../modules/wifi/wifi.h"
#include "config.h"

static std::vector<std::string> path_stack = { SG_APPS_DIR };
static std::vector<App> apps;
static std::deque<std::string> app_string_storage;

// Stack of folder paths, so ESC/back can pop up one level.
// Starts at root "/singularity/apps".

enum MenuState { STATE_MENU, STATE_APP };
static MenuState state = STATE_MENU;

static int selected = 0;
static bool needs_redraw = true;
static unsigned long last_status_refresh = 0;

static void scan_sd_apps(const char* base_path) {
    // Check if directory exists first
    if (!storage_exists(base_path)) {
        Serial.printf("[MENU] Directory not found: %s\n", base_path);
        return;
    }

    auto dirs = storage_list_dirs(base_path);

    for (auto& dirname : dirs) {
        std::string full_dir = std::string(base_path) + "/" + dirname;
        std::string manifest_path = full_dir + "/manifest.lua";

        if (!storage_exists(manifest_path.c_str())) {
            continue; // no manifest = not a valid app or folder, skip
        }

        char* buf = (char*)malloc(512);
        if (!buf) continue;
        
        if (!storage_read(manifest_path.c_str(), buf, 512)) {
            free(buf);
            continue;
        }

        LuaManifest m = lua_core_load_manifest(buf);
        free(buf);
        if (!m.valid) {continue;}
        

        if (m.type == "app") {
            app_string_storage.emplace_back(m.name);
            const char* name = app_string_storage.back().c_str();
            app_string_storage.emplace_back(full_dir + "/" + m.entry);
            const char* entry_path = app_string_storage.back().c_str();
            apps.push_back(make_lua_app_from_file(name, entry_path, icon_by_name(m.icon)));
        } else if (m.type == "folder") {
            app_string_storage.emplace_back(m.name);
            const char* name = app_string_storage.back().c_str();
            app_string_storage.emplace_back(full_dir);
            const char* folder_path = app_string_storage.back().c_str();
            apps.push_back(make_folder_app(name, folder_path, icon_by_name(m.icon)));
        }
    }
}

static void rebuild_menu() {

    apps.clear();
    app_string_storage.clear();

    if (path_stack.size() == 1) {
        // only show core apps at the true root
        apps.push_back(make_lua_app("WiFi", script_wifi, &icon_wifi));
        apps.push_back(make_lua_app("Config", script_config, &icon_cog));
    }

    scan_sd_apps(path_stack.back().c_str());
}

void menu_enter_folder(const char* path) {
    path_stack.push_back(path);
    rebuild_menu();
    selected = 0;
    needs_redraw = true;
}

void menu_go_back() {
    if (path_stack.size() > 1) {
        path_stack.pop_back();
        rebuild_menu();
        selected = 0;
        needs_redraw = true;
    }
}



// --- Drawing ---

/*static void draw_icon_placeholder(int x, int y) {
    M5Cardputer.Display.drawRect(x, y, 40, 40, THEME_COLOR);
    M5Cardputer.Display.drawLine(x, y, x + 40, y + 40, THEME_COLOR);
    M5Cardputer.Display.drawLine(x + 40, y, x, y + 40, THEME_COLOR);
}*/

static void draw_arrow_left(int x, int y) {
    M5Cardputer.Display.fillTriangle(x, y, x + 15, y - 20, x + 15, y + 20, THEME_COLOR);
}

static void draw_arrow_right(int x, int y) {
    M5Cardputer.Display.fillTriangle(x, y, x - 15, y - 20, x - 15, y + 20, THEME_COLOR);
}

static void draw_menu() {
    

    M5Cardputer.Display.fillScreen(BLACK);
    status_bar_draw();

    int screenW = M5Cardputer.Display.width();
    int screenH = M5Cardputer.Display.height();
    int centerY = STATUS_BAR_HEIGHT + (screenH - STATUS_BAR_HEIGHT) / 2;

    if (apps.empty()) {
        M5Cardputer.Display.setTextSize(1);
        M5Cardputer.Display.setTextColor(THEME_COLOR);
        M5Cardputer.Display.setCursor(10, centerY);
        M5Cardputer.Display.print("(empty)");
        return;
    }

    if (selected > 0) {
        draw_arrow_left(10, centerY);
    }
    if (selected < (int)apps.size() - 1) {
        draw_arrow_right(screenW - 10, centerY);
    }

    //int iconX = (screenW - 40) / 2;
    //int iconY = centerY - 20;
    
    int iconSize = 16; // your source icon size
    float scale = 4.0f; // 16 * 4 = 64
    int iconCenterX = screenW / 2;
    int iconCenterY = centerY;

    draw_icon_scaled(iconCenterX, iconCenterY, *apps[selected].icon, scale, THEME_COLOR);


    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setTextColor(THEME_COLOR);
    int textW = M5Cardputer.Display.textWidth(apps[selected].name);
    M5Cardputer.Display.setCursor((screenW - textW) / 2, iconCenterY + 30);
    M5Cardputer.Display.print(apps[selected].name);
}

// --- Public API ---

void menu_init() {
    path_stack = { SG_APPS_DIR };
    rebuild_menu();

    selected = 0;
    state = STATE_MENU;
    needs_redraw = true;
}

void menu_update(char key) {

    wifi_tick();
    
    if (millis() - last_status_refresh > 1250) {
        status_bar_draw();
        last_status_refresh = millis();
    }

    if (state == STATE_MENU) {
        if (key != 0 && !apps.empty()) {
            if (key == ',') {
                if (selected > 0) selected--;
                needs_redraw = true;
            } else if (key == '/') {
                if (selected < (int)apps.size() - 1) selected++;
                needs_redraw = true;
            } else if (key == '\r') {
                
                // SAVE is_folder flag BEFORE calling on_open(),
                // because on_open() might call rebuild_menu() which clears the apps vector!
                bool is_folder = apps[selected].is_folder;
                
                apps[selected].on_open(apps[selected].data);
                status_bar_draw();
                last_status_refresh = millis();
                
                // Only transition to STATE_APP if it's not a folder
                if (!is_folder) {
                    state = STATE_APP;
                    needs_redraw = false;
                } else {
                    // For folders, stay in STATE_MENU and redraw
                    needs_redraw = true;
                }
            } else if (key == '\n') {
                menu_go_back();
            }
        }

        if (needs_redraw) {
            draw_menu();
            needs_redraw = false;
        }
    } else if (state == STATE_APP) {
        apps[selected].on_loop(apps[selected].data);

        if (key != 0) {
            if (key == '\n') {
                apps[selected].on_close(apps[selected].data);
                state = STATE_MENU;
                needs_redraw = true;
            } else {
                apps[selected].on_key(apps[selected].data, key);
            }
        }
    }
}