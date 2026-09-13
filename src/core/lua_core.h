#pragma once
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
void lua_core_run_string(const char* script);
void lua_core_run_file(const char* path);
void lua_core_call_update();
void lua_core_call_on_key(char key);
LuaConfig lua_core_load_config(const char* script);
LuaManifest lua_core_load_manifest(const char* script);
WifiConfig lua_core_load_wifi_config(const char* script);
std::vector<WifiNetwork> lua_core_load_wifi_networks(const char* script);