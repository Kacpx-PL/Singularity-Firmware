#include "lua_app.h"
#include "lua_core.h"
#include "menu.h"

static void lua_app_open(const char* script) {
    if (script) {
        lua_core_run_string(script);
    }
}

static void lua_app_open_from_file(const char* path) {
    if (path) {
        lua_core_run_file(path);
    }
}

static void lua_app_loop(const char* data) {
    lua_core_call_update();
}

static void lua_app_key(const char* data, char key) {
    lua_core_call_on_key(key);
}

static void lua_app_close(const char* data) {
    // Later: call a Lua "on_close()" function, and/or reset Lua state.
}

static void folder_open(const char* path) {
    menu_enter_folder(path);
}

static void folder_loop(const char* data) {}
static void folder_key(const char* data, char key) {}
static void folder_close(const char* data) {}

// -- make apps --
App make_lua_app(const char* name, const char* script, const Icon* icon) {
    return { name, script, icon, false, lua_app_open, lua_app_loop, lua_app_key, lua_app_close };
}

App make_lua_app_from_file(const char* name, const char* path, const Icon* icon) {
    return { name, path, icon, false, lua_app_open_from_file, lua_app_loop, lua_app_key, lua_app_close };
}

App make_folder_app(const char* name, const char* path, const Icon* icon) {
    return { name, path, icon, true, folder_open, folder_loop, folder_key, folder_close };
}