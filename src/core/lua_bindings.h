#pragma once

extern "C" {
    #include "lua.h"
}

void register_config_bindings(lua_State* L);
void register_gpio_bindings(lua_State* L);
void register_ir_bindings(lua_State* L);
void register_system_bindings(lua_State* L);
void register_wifi_bindings(lua_State* L);
void register_ble_bindings(lua_State* L);
void register_http_bindings(lua_State* L);
void registergfx_bindings(lua_State* L);

void register_storage_bindings(lua_State* L); // in lua_core.cpp