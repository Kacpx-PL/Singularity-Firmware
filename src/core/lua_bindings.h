#pragma once

extern "C" {
    #include "lua.h"
}

void register_wifi_bindings(lua_State* L);
void register_storage_bindings(lua_State* L);
void register_ui_bindings(lua_State* L);
void register_hardware_bindings(lua_State* L);