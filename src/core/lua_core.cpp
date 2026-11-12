#include "lua_core.h"
#include "lua_bindings.h"
#include "../modules/ble/ble_service.h"
#include "../modules/wifi/wifi.h"
#include "../modules/http/http_service.h"
#include "../modules/i2c/i2c_service.h"

extern "C" {
    #include "lua.h"
    #include "lauxlib.h"
    #include "lualib.h"
}

#include <M5Cardputer.h>

#include "../modules/storage/storage_service.h"

static lua_State* L;
static const char* LUA_PROTECTED_GLOBALS = "singularity.protected_globals";
static constexpr size_t LUA_APP_MAX_FILE_SIZE = 32 * 1024;

// ---------------------------------------------------------------------------
// Resource limits: memory cap ("OOM killer"), low-heap guard and watchdog
// ---------------------------------------------------------------------------
#define SG_APP_MEM_BUDGET   (96 * 1024)  // Lua heap an app may use on top of the base state
#define SG_HEAP_RESERVE     (24 * 1024)  // Lua refuses to grow if free heap would drop below this
#define SG_HEAP_CRITICAL    (16 * 1024)  // free heap below this while an app runs => app is killed
#define SG_WATCHDOG_MS      3000         // max time one script call (open/update/on_key) may run
#define SG_WATCHDOG_STEP    2000         // VM instructions between watchdog checks

static size_t g_mem_used = 0;
static size_t g_mem_limit = 0;        // 0 = unlimited (while the base state is being built)
static bool g_app_faulted = false;    // true once an app was halted/killed; loop+keys are ignored
static bool g_wd_enabled = false;
static bool g_wd_tripped = false;
static unsigned long g_wd_start = 0;
static std::string g_app_dir;         // folder of the running SD app ("" for built-in apps)

// Lua allocator with accounting. Returning NULL on growth makes Lua run an
// emergency GC and then raise LUA_ERRMEM, which the pcall wrappers turn into a kill.
static void* sg_alloc(void*, void* ptr, size_t osize, size_t nsize) {
    size_t old = ptr ? osize : 0;   // when ptr is NULL, osize is a type tag, not a size
    if (nsize == 0) {
        if (ptr) {
            g_mem_used -= old;
            free(ptr);
        }
        return nullptr;
    }
    if (nsize > old && g_mem_limit) {
        size_t grow = nsize - old;
        if (g_mem_used + grow > g_mem_limit) return nullptr;
        if (grow >= 256 && ESP.getFreeHeap() < SG_HEAP_RESERVE + grow) return nullptr;
    }
    void* p = realloc(ptr, nsize);
    if (p) g_mem_used = g_mem_used - old + nsize;
    return p;
}

static int sg_panic(lua_State* La) {
    const char* msg = lua_tostring(La, -1);
    Serial.printf("[LUA] PANIC: %s - restarting\n", msg ? msg : "?");
    Serial.flush();
    delay(100);
    ESP.restart();
    return 0;
}

static void wd_hook(lua_State* La, lua_Debug*) {
    if (!g_wd_enabled) return;
    if (!g_wd_tripped && millis() - g_wd_start <= SG_WATCHDOG_MS) return;
    if (!g_wd_tripped) {
        g_wd_tripped = true;
        // From now on fail on EVERY instruction so a script can't swallow the
        // error with pcall and keep looping.
        lua_sethook(La, wd_hook, LUA_MASKCOUNT, 1);
    }
    luaL_error(La, "watchdog: script ran longer than %d ms", SG_WATCHDOG_MS);
}

static void wd_set_hook() {
    lua_sethook(L, wd_hook, LUA_MASKCOUNT, SG_WATCHDOG_STEP);
}

static void wd_begin() {
    g_wd_start = millis();
    g_wd_tripped = false;
    g_wd_enabled = true;
}

static void wd_end() {
    g_wd_enabled = false;
    if (g_wd_tripped) {
        g_wd_tripped = false;
        wd_set_hook(); // restore the normal check interval
    }
}

// RAII guard for the config/manifest loaders (many early returns).
struct WdScope {
    WdScope() { wd_begin(); }
    ~WdScope() { wd_end(); }
};

// Blocking bindings (http.fetch, wifi.connect) call this when they return so
// time spent waiting on the network doesn't count against the script.
void lua_core_watchdog_kick() {
    g_wd_start = millis();
}

bool lua_core_heap_critical() {
    return ESP.getFreeHeap() < SG_HEAP_CRITICAL;
}

bool lua_core_app_faulted() {
    return g_app_faulted;
}

size_t lua_core_mem_used() {
    return g_mem_used;
}

void lua_core_set_app_dir(const char* dir) {
    g_app_dir = dir ? dir : "";
}

const char* lua_core_get_app_dir() {
    return g_app_dir.c_str();
}


static int l_print_screen(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);

    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextSize(2);
    M5Cardputer.Display.setTextColor(WHITE);
    M5Cardputer.Display.setCursor(10, 40);
    M5Cardputer.Display.print(text);

    return 0;
}

static void show_lua_error(const char* err, bool halted = false) {
    Serial.println(err);
    M5Cardputer.Display.fillScreen(BLACK);
    M5Cardputer.Display.setTextSize(1);
    M5Cardputer.Display.setTextColor(RED);
    M5Cardputer.Display.setCursor(5, 25);
    M5Cardputer.Display.print(err);
    if (halted) {
        M5Cardputer.Display.setTextColor(WHITE);
        M5Cardputer.Display.print("\n\n[app stopped - press ESC]");
    }
}

// Call right after a guarded lua_pcall. Handles errors, OOM and watchdog trips.
//   halt_on_error: a plain runtime error also stops the app (used for open/update)
// OOM and watchdog trips ALWAYS kill the app: its memory is reclaimed immediately.
static void finish_call(int status, bool halt_on_error) {
    bool wd = g_wd_tripped;   // read before wd_end() clears it
    wd_end();

    if (status == LUA_OK && !wd) {
        lua_settop(L, 0);
        return;
    }

    std::string msg;
    if (status != LUA_OK) {
        const char* e = lua_tostring(L, -1);
        msg = e ? e : "unknown error";
    } else {
        msg = "watchdog: script ignored timeout"; // script swallowed the error with pcall
    }
    if (status == LUA_ERRMEM) {
        msg = "Out of memory: app killed";
    }
    lua_settop(L, 0);

    bool kill = wd || status == LUA_ERRMEM;
    if (kill) lua_core_reset_app();   // reclaim everything right now
    if (kill || halt_on_error) {
        g_app_faulted = true;
        show_lua_error(msg.c_str(), true);
    } else {
        Serial.println(msg.c_str());
    }
}

void lua_core_kill_app(const char* reason) {
    lua_core_reset_app();
    g_app_faulted = true;
    show_lua_error(reason, true);
}

void lua_core_init() {
    L = lua_newstate(sg_alloc, nullptr, luaL_makeseed(nullptr));   // accounted allocator instead of luaL_newstate()
    if (!L) {
        Serial.println("[LUA] failed to create Lua state");
        return;
    }
    lua_atpanic(L, sg_panic);
    luaL_openlibs(L); // for now, open everything

    lua_register(L, "print_screen", l_print_screen);
    register_config_bindings(L);
    register_gpio_bindings(L);
    register_ir_bindings(L);
    register_system_bindings(L);
    register_wifi_bindings(L);
    register_ble_bindings(L);
    register_espnow_bindings(L);
    register_http_bindings(L);
    register_i2c_bindings(L);
    registergfx_bindings(L);

    register_storage_bindings(L); // defined here

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

    // Base state is built: from here on apps are limited to a fixed budget.
    lua_gc(L, LUA_GCCOLLECT, 0);
    g_mem_limit = g_mem_used + SG_APP_MEM_BUDGET;
    wd_set_hook();
    Serial.printf("[LUA] base state: %u KB, app budget: %u KB (limit %u KB)\n",
                  (unsigned)(g_mem_used / 1024), (unsigned)(SG_APP_MEM_BUDGET / 1024),
                  (unsigned)(g_mem_limit / 1024));
}

void lua_core_reset_app() {
    g_app_faulted = false;
    uint32_t before = ESP.getFreeHeap();
    wifi_scan_cancel();
    ble_scan_cancel();
    http_cleanup();
    i2c_end();            // give the Grove port back (and re-enable IR receive)
    g_app_dir.clear();
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
    Serial.printf("[LUA] app state reset, free heap: %u -> %u bytes, lua mem: %u KB\n",
                  before, ESP.getFreeHeap(), (unsigned)(g_mem_used / 1024));
}

void lua_core_run_string(const char* script) {
    g_app_faulted = false; // a newly started script begins with a clean slate
    wd_begin();
    int st = luaL_loadstring(L, script);
    if (st == LUA_OK) st = lua_pcall(L, 0, LUA_MULTRET, 0);
    finish_call(st, true);
}

void lua_core_run_file(const char* path) {
    char* buf = (char*)malloc(LUA_APP_MAX_FILE_SIZE + 1);
    if (!buf) {
        Serial.println("Failed to allocate buffer for script file");
        return;
    }

    if (!storage_read(path, buf, LUA_APP_MAX_FILE_SIZE + 1)) {
        Serial.println("Failed to read script file");
        free(buf);
        return;
    }

    lua_core_run_string(buf);
    free(buf);
}

void lua_core_run_test() {
    lua_core_run_string("print_screen('Hello from Lua!')");
}

void lua_core_call_update() {
    if (g_app_faulted) return; // app was halted/killed, wait for ESC
    lua_getglobal(L, "update");
    if (lua_isfunction(L, -1)) {
        wd_begin();
        int st = lua_pcall(L, 0, 0, 0);
        finish_call(st, true);
    } else {
        lua_pop(L, 1); // clean up the non-function value pushed by lua_getglobal
    }
}

void lua_core_call_on_key(char key) {
    if (g_app_faulted) return;
    lua_getglobal(L, "on_key");
    if (lua_isfunction(L, -1)) {
        lua_pushinteger(L, key);
        wd_begin();
        int st = lua_pcall(L, 1, 0, 0);
        finish_call(st, false); // plain errors only log; OOM/watchdog still kill the app
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

    // Use a Lua-managed buffer instead of malloc/free: if Lua raises an error
    // (e.g. out of memory) it longjmps past this function, which would leak a
    // malloc'd buffer. luaL_Buffer memory is owned by the GC.
    luaL_Buffer b;
    char* buf = luaL_buffinitsize(L, &b, size + 1);

    if (storage_read(path, buf, size + 1)) {
        luaL_pushresultsize(&b, strlen(buf));
    } else {
        lua_pushnil(L);
    }
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
    WdScope wd_scope; // manifests are untrusted code too
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

    m.priority = 100;
    lua_getfield(L, -1, "priority");
    if (lua_isnumber(L, -1)) {
        m.priority = (int)lua_tointeger(L, -1);
    }
    lua_pop(L, 1);

    lua_pop(L, 1); // pop the table itself
    lua_settop(L, 0); // completely clear stack

    m.valid = true;
    return m;
}

std::vector<WifiNetwork> lua_core_load_wifi_networks(const char* script) {
    WdScope wd_scope;
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
    WdScope wd_scope;
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
    WdScope wd_scope;
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