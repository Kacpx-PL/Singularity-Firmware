#include "lua_core.h"
#include "lua_bindings.h"
#include "../modules/ble/ble_service.h"
#include "../modules/wifi/wifi.h"

extern "C" {
    #include "lua.h"
    #include "lauxlib.h"
    #include "lualib.h"
}

#include <M5Cardputer.h>

#include "../modules/storage/storage_service.h"

static lua_State* L;
static const char* LUA_PROTECTED_GLOBALS = "singularity.protected_globals";


static int l_print_screen(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);

    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setCursor(10, 40);
    M5Cardputer.Display.print(text);

    return 0;
}

static void show_lua_error(const char* err) {
    Serial.println(err);
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.setCursor(5, 25);
    M5Cardputer.Display.print(err);
}

void lua_core_init() {
    L = luaL_newstate();
    luaL_openlibs(L); // for now, open everything

    lua_register(L, "print_screen", l_print_screen);
    register_wifi_bindings(L);
    register_ble_bindings(L);
    register_storage_bindings(L);
    register_ui_bindings(L);
    register_hardware_bindings(L);

    lua_newtable(L);
    int protected_globals = lua_gettop(L);
    lua_pushglobaltable(L);
    int globals = lua_gettop(L);
    lua_pushnil(L);
    while (lua_next(L, globals) != 0) {
        lua_pushvalue(L, -2);
        lua_pushboolean(L, 1);
        lua_rawset(L, protected_globals);
        lua_pop(L, 1);
    }
    lua_pushvalue(L, protected_globals);
    lua_setfield(L, LUA_REGISTRYINDEX, LUA_PROTECTED_GLOBALS);
    lua_settop(L, 0);
}

void lua_core_reset_app() {
    uint32_t before = ESP.getFreeHeap();
    wifi_scan_cancel();
    ble_scan_cancel();
    if (L) {
        lua_pushglobaltable(L);
        int globals = lua_gettop(L);
        lua_getfield(L, LUA_REGISTRYINDEX, LUA_PROTECTED_GLOBALS);
        int protected_globals = lua_gettop(L);
        lua_pushnil(L);
        while (lua_next(L, globals) != 0) {
            lua_pushvalue(L, -2);
            lua_rawget(L, protected_globals);
            bool protected_global = !lua_isnil(L, -1);
            lua_pop(L, 1);
            if (!protected_global) {
                lua_pushvalue(L, -2);
                lua_pushnil(L);
                lua_rawset(L, globals);
            }
            lua_pop(L, 1);
        }
        lua_settop(L, 0);
        lua_gc(L, LUA_GCCOLLECT, 0);
    }
    Serial.printf("[LUA] app state reset, free heap: %u -> %u bytes\n", before, ESP.getFreeHeap());
}

void lua_core_run_string(const char* script) {
    if (luaL_dostring(L, script) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        Serial.println(err);

        show_lua_error(err);
    }
}

void lua_core_run_file(const char* path) {
    char* buf = (char*)malloc(8192);
    if (!buf) {
        Serial.println("Failed to allocate buffer for script file");
        return;
    }

    if (!storage_read(path, buf, 8192)) {
        Serial.println("Failed to read script file");
        free(buf);
        return;
    }

    lua_core_run_string(buf);
    free(buf);
}

void lua_core_run_test() {
    const char* script = "print_screen('Hello from Lua!')";

    if (luaL_dostring(L, script) != LUA_OK) {
        const char* err = lua_tostring(L, -1);
        Serial.println(err);

        show_lua_error(err);
    }
}

void lua_core_call_update() {
    lua_getglobal(L, "update");
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            Serial.println(err);

            show_lua_error(err);
        }
    } else {
        lua_pop(L, 1); // clean up the non-function value pushed by lua_getglobal
    }
}

void lua_core_call_on_key(char key) {
    lua_getglobal(L, "on_key");
    if (lua_isfunction(L, -1)) {
        lua_pushinteger(L, key);
        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
            const char* err = lua_tostring(L, -1);
            Serial.println(err);
        }
    } else {
        lua_pop(L, 1);
    }
}

static int l_storage_read(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);

    size_t size = storage_get_file_size(path);
    if (size == 0) {
        lua_pushnil(L);
        return 1;
    }

    char* buf = (char*)malloc(size + 1);
    if (!buf) {
        lua_pushnil(L);
        return 1;
    }

    if (storage_read(path, buf, size + 1)) {
        lua_pushstring(L, buf);
    } else {
        lua_pushnil(L);
    }

    free(buf);
    return 1;
}

static int l_storage_write(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    const char* data = luaL_checkstring(L, 2);
    lua_pushboolean(L, storage_write(path, data));
    return 1;
}

static int l_storage_exists(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    lua_pushboolean(L, storage_exists(path));
    return 1;
}

void register_storage_bindings(lua_State* L) {
    lua_register(L, "storage_read", l_storage_read);
    lua_register(L, "storage_write", l_storage_write);
    lua_register(L, "storage_exists", l_storage_exists);
}

LuaManifest lua_core_load_manifest(const char* script) {
    LuaManifest m;
    m.valid = false;

    // Clear the stack before loading new manifest
    lua_settop(L, 0);

    if (luaL_loadstring(L, script) != LUA_OK) {
        lua_pop(L, 1);
        lua_settop(L, 0);
        return m;
    }

    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        lua_pop(L, 1);
        lua_settop(L, 0);
        return m;
    }

    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        lua_settop(L, 0);
        return m;
    }

    lua_getfield(L, -1, "type");
    if (lua_isstring(L, -1)) m.type = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "name");
    if (lua_isstring(L, -1)) {
        m.name = lua_tostring(L, -1);
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "icon");
    if (lua_isstring(L, -1)) m.icon = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "entry");
    if (lua_isstring(L, -1)) m.entry = lua_tostring(L, -1);
    lua_pop(L, 1);

    lua_pop(L, 1); // pop the table itself
    lua_settop(L, 0); // completely clear stack

    m.valid = true;
    return m;
}

std::vector<WifiNetwork> lua_core_load_wifi_networks(const char* script) {
    std::vector<WifiNetwork> networks;

    if (luaL_loadstring(L, script) != LUA_OK) {
        Serial.println(lua_tostring(L, -1));
        lua_pop(L, 1);
        return networks;
    }

    if (lua_pcall(L, 0, 1, 0) != LUA_OK) {
        Serial.println(lua_tostring(L, -1));
        lua_pop(L, 1);
        return networks;
    }

    if (!lua_istable(L, -1)) {
        lua_pop(L, 1);
        return networks;
    }

    int len = lua_rawlen(L, -1); // number of entries in the array

    for (int i = 1; i <= len; i++) {
        lua_rawgeti(L, -1, i); // push networks[i]

        if (lua_istable(L, -1)) {
            WifiNetwork net;

            lua_getfield(L, -1, "ssid");
            if (lua_isstring(L, -1)) net.ssid = lua_tostring(L, -1);
            lua_pop(L, 1);

            lua_getfield(L, -1, "password");
            if (lua_isstring(L, -1)) net.password = lua_tostring(L, -1);
            lua_pop(L, 1);

            networks.push_back(net);
        }

        lua_pop(L, 1); // pop networks[i]
    }

    lua_pop(L, 1); // pop the outer table
    return networks;
}

WifiConfig lua_core_load_wifi_config(const char* script) {
    WifiConfig cfg;
    cfg.autoconnect = true; // fallback per your request — missing field = assume true

    if (luaL_loadstring(L, script) != LUA_OK) { lua_pop(L, 1); return cfg; }
    if (lua_pcall(L, 0, 1, 0) != LUA_OK) { lua_pop(L, 1); return cfg; }
    if (!lua_istable(L, -1)) { lua_pop(L, 1); return cfg; }

    lua_getfield(L, -1, "autoconnect");
    if (lua_isboolean(L, -1)) cfg.autoconnect = lua_toboolean(L, -1);
    lua_pop(L, 1);

    lua_getfield(L, -1, "networks");
    if (lua_istable(L, -1)) {
        int len = lua_rawlen(L, -1);
        for (int i = 1; i <= len; i++) {
            lua_rawgeti(L, -1, i);
            if (lua_istable(L, -1)) {
                WifiNetwork net;
                lua_getfield(L, -1, "ssid");
                if (lua_isstring(L, -1)) net.ssid = lua_tostring(L, -1);
                lua_pop(L, 1);
                lua_getfield(L, -1, "password");
                if (lua_isstring(L, -1)) net.password = lua_tostring(L, -1);
                lua_pop(L, 1);
                cfg.networks.push_back(net);
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1); // networks field
    lua_pop(L, 1); // outer table

    return cfg;
}

LuaConfig lua_core_load_config(const char* script) {
    LuaConfig cfg = {};

    if (luaL_loadstring(L, script) != LUA_OK) { lua_pop(L, 1); return cfg; }
    if (lua_pcall(L, 0, 1, 0) != LUA_OK) { lua_pop(L, 1); return cfg; }
    if (!lua_istable(L, -1)) { lua_pop(L, 1); return cfg; }

    lua_getfield(L, -1, "theme_color");
    if (lua_isnumber(L, -1)) {
        cfg.theme_color = (uint32_t)lua_tointeger(L, -1);
        cfg.has_theme_color = true;
    }
    lua_pop(L, 1);

    lua_getfield(L, -1, "boot_delay_ms");
    if (lua_isnumber(L, -1)) {
        cfg.boot_delay_ms = (uint32_t)lua_tointeger(L, -1);
        cfg.has_boot_delay_ms = true;
    }
    lua_pop(L, 1);

    lua_pop(L, 1);
    return cfg;
}