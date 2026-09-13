#include "lua_bindings.h"
#include "status_bar.h"
#include "config.h"
#include "pins.h"
#include "../modules/ir/ir_service.h"
#include "M5Cardputer.h"
#include "../modules/wifi/wifi.h"
#include "../modules/ui/ui_list.h"
#include "../modules/ui/ui_text_input.h"
#include "../modules/ui/ui_file_browser.h"

extern "C" {
    #include "lua.h"
    #include "lauxlib.h"
}

//default draw color
static uint32_t g_draw_color = 0xFFFFFF;

// Hardware bindings

static int l_config_set_theme_color(lua_State* L) {
    int color = luaL_checkinteger(L, 1);
    config_set_theme_color((uint16_t)color);
    return 0;
}

static int l_config_set_boot_delay(lua_State* L) {
    int ms = luaL_checkinteger(L, 1);
    config_set_boot_delay_ms((uint32_t)ms);
    return 0;
}

static int l_gpio_write(lua_State* L) {
    int pin = luaL_checkinteger(L, 1);
    int value = luaL_checkinteger(L, 2);

    if (pin_is_reserved(pin)) {
        return luaL_error(L, "GPIO %d is reserved by the firmware", pin);
    }

    pinMode(pin, OUTPUT);
    digitalWrite(pin, value);
    return 0;
}

static int l_gpio_read(lua_State* L) {
    int pin = luaL_checkinteger(L, 1);

    if (pin_is_reserved(pin)) {
        return luaL_error(L, "GPIO %d is reserved by the firmware", pin);
    }

    pinMode(pin, INPUT);
    lua_pushinteger(L, digitalRead(pin));
    return 1;
}

static int l_ir_send_protocol(lua_State* L) {
    const char* protocol = luaL_checkstring(L, 1);
    uint32_t address = (uint32_t)luaL_checkinteger(L, 2);
    uint32_t command = (uint32_t)luaL_checkinteger(L, 3);
    ir_send_protocol(protocol, address, command);
    return 0;
}

static int l_ir_send_raw(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    int freq = luaL_checkinteger(L, 2);

    int len = lua_rawlen(L, 1);
    uint16_t* data = (uint16_t*)malloc(len * sizeof(uint16_t));
    if (!data) return luaL_error(L, "out of memory for raw IR data");

    for (int i = 0; i < len; i++) {
        lua_rawgeti(L, 1, i + 1); // Lua arrays are 1-indexed
        data[i] = (uint16_t)lua_tointeger(L, -1);
        lua_pop(L, 1);
    }

    ir_send_raw(data, len, (uint16_t)freq);
    free(data);

    return 0;
}

static int l_ir_receive_available(lua_State* L) {
    lua_pushboolean(L, ir_receive_available());
    return 1;
}

static int l_ir_receive_get_result(lua_State* L) {
    lua_pushstring(L, ir_receive_get_result().c_str());
    return 1;
}

static int l_ir_receive_resume(lua_State* L) {
    ir_receive_resume();
    return 0;
}

static int l_ir_receive_get_address(lua_State* L) {
    lua_pushinteger(L, ir_receive_get_address());
    return 1;
}

static int l_ir_receive_get_command(lua_State* L) {
    lua_pushinteger(L, ir_receive_get_command());
    return 1;
}

static int l_ir_receive_get_protocol(lua_State* L) {
    lua_pushstring(L, ir_receive_get_protocol().c_str());
    return 1;
}

static int l_serial_print(lua_State* L) {
    const char* str = luaL_checkstring(L, 1);
    Serial.print(str);
    return 0;
}

void register_hardware_bindings(lua_State* L) {
    lua_register(L, "config_set_theme_color", l_config_set_theme_color);
    lua_register(L, "config_set_boot_delay", l_config_set_boot_delay);
    lua_register(L, "gpio_read", l_gpio_read);
    lua_register(L, "gpio_write", l_gpio_write);
    lua_register(L, "ir_send_protocol",l_ir_send_protocol);
    lua_register(L, "ir_receive_available", l_ir_receive_available);
    lua_register(L, "ir_receive_get_result", l_ir_receive_get_result);
    lua_register(L, "ir_receive_resume",l_ir_receive_resume);
    lua_register(L, "serial_print",l_serial_print);
    lua_register(L, "ir_receive_get_address", l_ir_receive_get_address);
    lua_register(L, "ir_receive_get_command", l_ir_receive_get_command);
    lua_register(L, "ir_receive_get_protocol", l_ir_receive_get_protocol);
    lua_register(L, "ir_send_raw", l_ir_send_raw);
}

//Wifi Bindings
static int l_wifi_connect(lua_State* L) {
    const char* ssid = luaL_checkstring(L, 1);
    const char* pass = luaL_checkstring(L, 2);
    bool ok = wifi_connect(ssid, pass);
    lua_pushboolean(L, ok);
    return 1;
}

static int l_wifi_is_connected(lua_State* L) {
    lua_pushboolean(L, wifi_is_connected());
    return 1;
}

static int l_wifi_get_ip(lua_State* L) {
    lua_pushstring(L, wifi_get_ip());
    return 1;
}

static int l_wifi_disconnect(lua_State* L) {
    wifi_disconnect();
    return 0;
}

static int l_wifi_get_rssi(lua_State* L) {
    lua_pushinteger(L, wifi_get_rssi());
    return 1;
}

static int l_wifi_add_network(lua_State* L) {
    const char* ssid = luaL_checkstring(L, 1);
    const char* pass = luaL_checkstring(L, 2);
    wifi_add_network(ssid, pass);
    return 0;
}

static int l_wifi_set_autoconnect(lua_State* L) {
    bool enabled = lua_toboolean(L, 1);
    wifi_set_autoconnect(enabled);
    return 0;
}

static int l_wifi_get_autoconnect(lua_State* L) {
    lua_pushboolean(L, wifi_get_autoconnect());
    return 1;
}

static int l_wifi_get_ssid(lua_State* L) {
    lua_pushstring(L, wifi_get_ssid());
    return 1;
}

static int l_wifi_remove_network(lua_State* L) {
    int index = luaL_checkinteger(L, 1);
    wifi_remove_network(index);
    return 0;
}

static int l_wifi_get_network_count(lua_State* L) {
    lua_pushinteger(L, wifi_get_network_count());
    return 1;
}

static int l_wifi_get_network_ssid(lua_State* L) {
    int index = luaL_checkinteger(L, 1);
    lua_pushstring(L, wifi_get_network_ssid(index));
    return 1;
}

void register_wifi_bindings(lua_State* L) {
    lua_register(L, "wifi_connect", l_wifi_connect);
    lua_register(L, "wifi_is_connected", l_wifi_is_connected);
    lua_register(L, "wifi_get_ip", l_wifi_get_ip);
    lua_register(L, "wifi_disconnect", l_wifi_disconnect);
    lua_register(L, "wifi_get_rssi", l_wifi_get_rssi);
    lua_register(L, "wifi_add_network", l_wifi_add_network);
    lua_register(L, "wifi_set_autoconnect", l_wifi_set_autoconnect);
    lua_register(L, "wifi_get_autoconnect", l_wifi_get_autoconnect);
    lua_register(L, "wifi_get_ssid", l_wifi_get_ssid);
    lua_register(L, "wifi_remove_network", l_wifi_remove_network);
    lua_register(L, "wifi_get_network_count", l_wifi_get_network_count);
    lua_register(L, "wifi_get_network_ssid", l_wifi_get_network_ssid);
}

//UI Bindings
static int l_draw_status_bar(lua_State* L) {
    status_bar_draw();
    return 0;
}

static int l_list_clear(lua_State* L) {
    list_clear();
    return 0;
}

static int l_list_add_item(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    list_add_item(text);
    return 0;
}

static int l_list_draw(lua_State* L) {
    list_draw();
    return 0;
}

static int l_list_handle_key(lua_State* L) {
    int key = luaL_checkinteger(L, 1);
    int result = list_handle_key((char)key);
    lua_pushinteger(L, result);
    return 1;
}

static int l_list_get_selected_index(lua_State* L) {
    lua_pushinteger(L, list_get_selected_index());
    return 1;
}

static int l_list_get_selected_text(lua_State* L) {
    lua_pushstring(L, list_get_selected_text());
    return 1;
}

static int l_text_input_start(lua_State* L) {
    const char* prompt = luaL_checkstring(L, 1);
    bool mask = lua_toboolean(L, 2);
    text_input_start(prompt, mask);
    return 0;
}

static int l_text_input_draw(lua_State* L) {
    text_input_draw();
    return 0;
}

static int l_text_input_handle_key(lua_State* L) {
    int key = luaL_checkinteger(L, 1);
    lua_pushinteger(L, text_input_handle_key((char)key));
    return 1;
}

static int l_text_input_get_value(lua_State* L) {
    lua_pushstring(L, text_input_get_value());
    return 1;
}

static int l_clear_screen(lua_State* L) {
    M5Cardputer.Display.fillRect(0, 20, 240, 115, 0x0);
    return 1;
}

static int l_draw_text(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int size = luaL_optinteger(L, 4, 2);
    if(size < 1){size = 1;}
    uint32_t color = luaL_optinteger(L, 5, 0xFFFFFFFF);
    M5Cardputer.Display.setTextColor(color, BLACK);
    M5Cardputer.Display.setTextSize(size);
    M5Cardputer.Display.drawString(text, x, y);
    return 1;
}

static int l_set_color(lua_State* L) {
    g_draw_color = (uint32_t)luaL_checkinteger(L, 1);
    return 0;
}

static int l_draw_rect_full(lua_State* L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int w = luaL_checkinteger(L, 3);
    int h = luaL_checkinteger(L, 4);
    M5Cardputer.Display.fillRect(x, y, w, h,  g_draw_color);
    return 1;
}

static int l_draw_rect(lua_State* L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int w = luaL_checkinteger(L, 3);
    int h = luaL_checkinteger(L, 4);
    M5Cardputer.Display.drawRect(x, y, w, h, g_draw_color);
    return 1;
}

static int l_draw_line(lua_State* L) {
    int x0 = luaL_checkinteger(L, 1);
    int y0 = luaL_checkinteger(L, 2);
    int x1 = luaL_checkinteger(L, 3);
    int y1 = luaL_checkinteger(L, 4);
    M5Cardputer.Display.drawLine(x0, y0, x1, y1,  g_draw_color);
    return 1;
}

static int l_draw_triangle(lua_State* L) {
    int x0 = luaL_checkinteger(L, 1);
    int y0 = luaL_checkinteger(L, 2);
    int x1 = luaL_checkinteger(L, 3);
    int y1 = luaL_checkinteger(L, 4);
    int x2 = luaL_checkinteger(L, 5);
    int y2 = luaL_checkinteger(L, 6);
    M5Cardputer.Display.drawTriangle(x0,y0,x1,y1,x2,y2,  g_draw_color);
    return 1;
}

static int l_draw_triangle_full(lua_State* L) {
    int x0 = luaL_checkinteger(L, 1);
    int y0 = luaL_checkinteger(L, 2);
    int x1 = luaL_checkinteger(L, 3);
    int y1 = luaL_checkinteger(L, 4);
    int x2 = luaL_checkinteger(L, 5);
    int y2 = luaL_checkinteger(L, 6);
    M5Cardputer.Display.fillTriangle(x0,y0,x1,y1,x2,y2,  g_draw_color);
    return 1;
}

static int l_draw_circle(lua_State* L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int r = luaL_checkinteger(L, 3);
    M5Cardputer.Display.drawCircle(x,y,r, g_draw_color);
    return 1;
}

static int l_draw_circle_full(lua_State* L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int r = luaL_checkinteger(L, 3);
    M5Cardputer.Display.fillCircle(x,y,r, g_draw_color);
    return 1;
}

static int l_file_browser_start(lua_State* L) {
    const char* path = luaL_checkstring(L, 1);
    bool folder_select_mode = lua_toboolean(L, 2);
    file_browser_start(path ,folder_select_mode);
    return 0;
}
static int l_file_browser_draw(lua_State* L) {
    file_browser_draw();
    return 0;
}
static int l_file_browser_handle_key(lua_State* L) {
    int key = luaL_checkinteger(L, 1);
    lua_pushinteger(L, file_browser_handle_key((char)key));
    return 1;
}
static int l_file_browser_get_selected_path(lua_State* L) {
    lua_pushstring(L, file_browser_get_selected_path());
    return 1;
}

void register_ui_bindings(lua_State* L) {
    lua_register(L, "list_clear", l_list_clear);
    lua_register(L, "list_add_item", l_list_add_item);
    lua_register(L, "list_draw", l_list_draw);
    lua_register(L, "list_handle_key", l_list_handle_key);
    lua_register(L, "list_get_selected_index", l_list_get_selected_index);
    lua_register(L, "list_get_selected_text", l_list_get_selected_text);
    lua_register(L, "draw_status_bar", l_draw_status_bar);
    lua_register(L, "draw_text", l_draw_text);
    lua_register(L, "clear_screen", l_clear_screen);
    lua_register(L, "draw_rect_full", l_draw_rect_full);
    lua_register(L, "set_color", l_set_color);
    lua_register(L, "draw_rect", l_draw_rect);
    lua_register(L, "draw_line", l_draw_line);
    lua_register(L, "draw_triangle", l_draw_triangle);
    lua_register(L, "draw_triangle_full", l_draw_triangle_full);
    lua_register(L, "draw_circle", l_draw_circle);
    lua_register(L, "draw_circle_full", l_draw_circle_full);
    lua_register(L, "text_input_start", l_text_input_start);
    lua_register(L, "text_input_draw", l_text_input_draw);
    lua_register(L, "text_input_handle_key", l_text_input_handle_key);
    lua_register(L, "text_input_get_value", l_text_input_get_value);
    lua_register(L, "file_browser_start", l_file_browser_start);
    lua_register(L, "file_browser_draw", l_file_browser_draw);
    lua_register(L, "file_browser_handle_key", l_file_browser_handle_key);
    lua_register(L, "file_browser_get_selected_path", l_file_browser_get_selected_path);
}