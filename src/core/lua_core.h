#pragma once
#include <cstddef>
#include <string>
#include "../modules/wifi/wifi.h"

extern "C" {
    #include "lua.h"
}

struct LuaManifest {
    std::string type;   // "app" or "folder"
    std::string name;
    std::string icon;
    std::string entry;  // only meaningful when type == "app"
    int priority;       // default 100
    bool valid;
};

struct WifiConfig {
    bool autoconnect;
    std::vector<WifiNetwork> networks;
};

struct LuaConfig {
    bool has_theme_color;
    uint32_t theme_color;
    bool has_boot_delay_ms;
    uint32_t boot_delay_ms;
};

void lua_core_init();
void lua_core_reset_app();
void lua_core_run_string(const char* script);
void lua_core_run_file(const char* path);
void lua_core_call_update();
void lua_core_call_on_key(char key);

// Resource protection (memory cap, low-heap kill, watchdog)
void lua_core_watchdog_kick();               // call after blocking C calls (http, wifi.connect)
bool lua_core_heap_critical();               // free heap below the kill threshold
void lua_core_kill_app(const char* reason);  // reset app state now and halt it until ESC
bool lua_core_app_faulted();
size_t lua_core_mem_used();                  // bytes currently allocated by the Lua state

// Folder of the app being launched/running (e.g. "/singularity/apps/weather"), no trailing slash.
// nullptr/"" for built-in apps. Cleared when the app is reset.
void lua_core_set_app_dir(const char* dir);
const char* lua_core_get_app_dir();
LuaConfig lua_core_load_config(const char* script);
LuaManifest lua_core_load_manifest(const char* script);
WifiConfig lua_core_load_wifi_config(const char* script);
std::vector<WifiNetwork> lua_core_load_wifi_networks(const char* script);