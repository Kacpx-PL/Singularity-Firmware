#include "lua_bindings.h"
#include "status_bar.h"
#include "config.h"
#include "pins.h"
#include "../modules/ir/ir_service.h"
#include "M5Cardputer.h"
#include "../modules/wifi/wifi.h"
#include "../modules/ble/ble_service.h"
#include "../modules/ui/ui_list.h"
#include "../modules/ui/ui_text_input.h"
#include "../modules/ui/ui_file_browser.h"
#include "../modules/http/http_service.h"

extern "C" {
    #include "lua.h"
    #include "lauxlib.h"
}

#include <ArduinoJson.h>
#include <string>
#include <vector>

//default draw color
static uint16_t g_draw_color = 0xFFFF;

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

static int l_millis(lua_State* L) {
    lua_pushinteger(L, (lua_Integer)millis());
    return 1;
}

static int l_key_is_pressed(lua_State* L) {
    int key = luaL_checkinteger(L, 1);
    lua_pushboolean(L, M5Cardputer.Keyboard.isKeyPressed((char)key));
    return 1;
}

static int l_get_imu(lua_State* L) {

    M5.Imu.update();

    float ax = 0.0f, ay = 0.0f, az = 0.0f;
    float gx = 0.0f, gy = 0.0f, gz = 0.0f;

    M5.Imu.getAccel(&ax, &ay, &az);
    M5.Imu.getGyro(&gx, &gy, &gz);

    lua_createtable(L, 6, 0);

    float values[6] = { ax, ay, az, gx, gy, gz };

    for (int i = 0; i < 6; i++) {
        lua_pushnumber(L, values[i]);
        lua_rawseti(L, -2, i + 1);
    }

    return 1;
}

    


// -----Wifi Bindings-------
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

static int l_wifi_scan_start(lua_State* L) {
    bool show_hidden = lua_toboolean(L, 1);
    lua_pushboolean(L, wifi_scan_start(show_hidden));
    return 1;
}

static int l_wifi_scan_status(lua_State* L) {
    lua_pushstring(L, wifi_scan_status());
    return 1;
}

static int l_wifi_scan_count(lua_State* L) {
    lua_pushinteger(L, wifi_scan_get_count());
    return 1;
}

static int l_wifi_scan_get(lua_State* L) {
    int index = luaL_checkinteger(L, 1);
    WifiScanResult result;
    if (!wifi_scan_get_result(index, result)) {
        lua_pushnil(L);
        return 1;
    }

    lua_createtable(L, 0, 5);
    lua_pushstring(L, result.ssid.c_str());
    lua_setfield(L, -2, "ssid");
    lua_pushstring(L, result.bssid.c_str());
    lua_setfield(L, -2, "bssid");
    lua_pushinteger(L, result.rssi);
    lua_setfield(L, -2, "rssi");
    lua_pushinteger(L, result.channel);
    lua_setfield(L, -2, "channel");
    lua_pushinteger(L, result.encryption);
    lua_setfield(L, -2, "encryption");
    return 1;
}


static void push_byte_table(lua_State* L, const uint8_t* data, size_t length) {
    lua_createtable(L, (int)length, 0);
    for (size_t i = 0; i < length; i++) {
        lua_pushinteger(L, data[i]);
        lua_rawseti(L, -2, (int)i + 1);
    }
}

static void push_string_byte_table(lua_State* L, const std::string& value) {
    push_byte_table(L, (const uint8_t*)value.data(), value.size());
}

static void push_string_array(lua_State* L, const std::vector<std::string>& values) {
    lua_createtable(L, (int)values.size(), 0);
    for (size_t i = 0; i < values.size(); i++) {
        lua_pushstring(L, values[i].c_str());
        lua_rawseti(L, -2, (int)i + 1);
    }
}

static int l_ble_scan_start(lua_State* L) {
    uint32_t duration = (uint32_t)luaL_optinteger(L, 1, 5);
    bool active = lua_isnoneornil(L, 2) ? true : lua_toboolean(L, 2);
    if (duration == 0) duration = 1;
    lua_pushboolean(L, ble_scan_start(duration, active));
    return 1;
}

static int l_ble_scan_status(lua_State* L) {
    lua_pushstring(L, ble_scan_status());
    return 1;
}

static int l_ble_scan_count(lua_State* L) {
    lua_pushinteger(L, ble_scan_get_count());
    return 1;
}

static int l_ble_scan_get(lua_State* L) {
    int index = luaL_checkinteger(L, 1);
    BleScanResult result;
    if (!ble_scan_get_result(index, result)) {
        lua_pushnil(L);
        return 1;
    }

    lua_createtable(L, 0, 13);
    lua_pushstring(L, result.address.c_str());
    lua_setfield(L, -2, "address");
    lua_pushinteger(L, result.address_type);
    lua_setfield(L, -2, "address_type");
    lua_pushinteger(L, result.rssi);
    lua_setfield(L, -2, "rssi");
    lua_pushstring(L, result.name.c_str());
    lua_setfield(L, -2, "name");
    if (result.has_appearance) {
        lua_pushinteger(L, result.appearance);
        lua_setfield(L, -2, "appearance");
    }
    if (result.has_tx_power) {
        lua_pushinteger(L, result.tx_power);
        lua_setfield(L, -2, "tx_power");
    }
    push_string_array(L, result.service_uuids);
    lua_setfield(L, -2, "service_uuids");
    push_string_array(L, result.service_data_uuids);
    lua_setfield(L, -2, "service_data_uuids");
    lua_createtable(L, (int)result.service_data.size(), 0);
    for (size_t i = 0; i < result.service_data.size(); i++) {
        push_string_byte_table(L, result.service_data[i]);
        lua_rawseti(L, -2, (int)i + 1);
    }
    lua_setfield(L, -2, "service_data");
    push_string_byte_table(L, result.manufacturer_data);
    lua_setfield(L, -2, "manufacturer_data");
    push_byte_table(L, result.payload.data(), result.payload.size());
    lua_setfield(L, -2, "payload");
    return 1;
}

static int l_ble_parse_advertisement(lua_State* L) {
    luaL_checktype(L, 1, LUA_TTABLE);
    size_t length = lua_rawlen(L, 1);
    if (length > 255) return luaL_error(L, "advertisement payload is limited to 255 bytes");

    std::vector<uint8_t> data(length);
    for (size_t i = 0; i < length; i++) {
        lua_rawgeti(L, 1, (int)i + 1);
        lua_Integer byte = luaL_checkinteger(L, -1);
        lua_pop(L, 1);
        if (byte < 0 || byte > 255) return luaL_error(L, "advertisement bytes must be in range 0..255");
        data[i] = (uint8_t)byte;
    }

    std::vector<BleAdField> fields = ble_parse_advertisement(data.data(), data.size());
    lua_createtable(L, (int)fields.size(), 0);
    for (size_t i = 0; i < fields.size(); i++) {
        const BleAdField& field = fields[i];
        lua_createtable(L, 0, 8);
        lua_pushinteger(L, field.type);
        lua_setfield(L, -2, "type");
        lua_pushinteger(L, (lua_Integer)field.data.size() + 1);
        lua_setfield(L, -2, "length");
        push_byte_table(L, field.data.data(), field.data.size());
        lua_setfield(L, -2, "data");
        if (!field.name.empty()) {
            lua_pushstring(L, field.name.c_str());
            lua_setfield(L, -2, "name");
        }
        if (!field.uuid.empty()) {
            lua_pushstring(L, field.uuid.c_str());
            lua_setfield(L, -2, "uuid");
        }
        if (field.has_flags) {
            lua_pushinteger(L, field.flags);
            lua_setfield(L, -2, "flags");
        }
        if (field.has_tx_power) {
            lua_pushinteger(L, field.tx_power);
            lua_setfield(L, -2, "tx_power");
        }
        if (field.has_company_id) {
            lua_pushinteger(L, field.company_id);
            lua_setfield(L, -2, "company_id");
        }
        lua_rawseti(L, -2, (int)i + 1);
    }
    return 1;
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
    return 0;
}

static int l_draw_text(lua_State* L) {
    const char* text = luaL_checkstring(L, 1);
    int x = luaL_checkinteger(L, 2);
    int y = luaL_checkinteger(L, 3);
    int size = luaL_optinteger(L, 4, 2);
    if(size < 1){size = 1;}
    uint16_t color = luaL_optinteger(L, 5, 0xFFFF);
    M5Cardputer.Display.setTextColor(color, BLACK);
    M5Cardputer.Display.setTextSize(size);
    M5Cardputer.Display.drawString(text, x, y);
    return 0;
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
    return 0;
}

static int l_draw_rect(lua_State* L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int w = luaL_checkinteger(L, 3);
    int h = luaL_checkinteger(L, 4);
    M5Cardputer.Display.drawRect(x, y, w, h, g_draw_color);
    return 0;
}

static int l_draw_line(lua_State* L) {
    int x0 = luaL_checkinteger(L, 1);
    int y0 = luaL_checkinteger(L, 2);
    int x1 = luaL_checkinteger(L, 3);
    int y1 = luaL_checkinteger(L, 4);
    M5Cardputer.Display.drawLine(x0, y0, x1, y1,  g_draw_color);
    return 0;
}

static int l_draw_triangle(lua_State* L) {
    int x0 = luaL_checkinteger(L, 1);
    int y0 = luaL_checkinteger(L, 2);
    int x1 = luaL_checkinteger(L, 3);
    int y1 = luaL_checkinteger(L, 4);
    int x2 = luaL_checkinteger(L, 5);
    int y2 = luaL_checkinteger(L, 6);
    M5Cardputer.Display.drawTriangle(x0,y0,x1,y1,x2,y2,  g_draw_color);
    return 0;
}

static int l_draw_triangle_full(lua_State* L) {
    int x0 = luaL_checkinteger(L, 1);
    int y0 = luaL_checkinteger(L, 2);
    int x1 = luaL_checkinteger(L, 3);
    int y1 = luaL_checkinteger(L, 4);
    int x2 = luaL_checkinteger(L, 5);
    int y2 = luaL_checkinteger(L, 6);
    M5Cardputer.Display.fillTriangle(x0,y0,x1,y1,x2,y2,  g_draw_color);
    return 0;
}

static int l_draw_circle(lua_State* L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int r = luaL_checkinteger(L, 3);
    M5Cardputer.Display.drawCircle(x,y,r, g_draw_color);
    return 0;
}

static int l_draw_circle_full(lua_State* L) {
    int x = luaL_checkinteger(L, 1);
    int y = luaL_checkinteger(L, 2);
    int r = luaL_checkinteger(L, 3);
    M5Cardputer.Display.fillCircle(x,y,r, g_draw_color);
    return 0;
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

void lua_push_json_value(lua_State* L, JsonVariantConst v) {
    if (v.is<JsonObjectConst>()) {
        lua_newtable(L);
        for (JsonPairConst kv : v.as<JsonObjectConst>()) {
            lua_pushstring(L, kv.key().c_str());
            lua_push_json_value(L, kv.value());
            lua_settable(L, -3);
        }
    } else if (v.is<JsonArrayConst>()) {
        lua_newtable(L);
        int i = 1;
        for (JsonVariantConst elem : v.as<JsonArrayConst>()) {
            lua_push_json_value(L, elem);
            lua_rawseti(L, -2, i++);
        }
    } else if (v.is<bool>()) {
        lua_pushboolean(L, v.as<bool>());
    } else if (v.is<lua_Number>()) {
        lua_pushnumber(L, v.as<lua_Number>());
    } else if (v.is<const char*>()) {
        lua_pushstring(L, v.as<const char*>());
    } else {
        lua_pushnil(L);
    }
}

static int l_http_fetch(lua_State* L) {
    const char* url = luaL_checkstring(L, 1);
    int timeout_ms = luaL_optinteger(L, 2, 10000);

    HttpResponse response = http_fetch(url, timeout_ms);
    lua_createtable(L, 0, 4);

    lua_pushinteger(L, response.status_code);
    lua_setfield(L, -2, "status_code");

    lua_pushboolean(L, response.ok);
    lua_setfield(L, -2, "ok");

    if (!response.body.empty()) {
        lua_pushstring(L, response.body.c_str());
        lua_setfield(L, -2, "body");
    } else {
        lua_pushstring(L, "");
        lua_setfield(L, -2, "body");
    }

    if (!response.error.empty()) {
        lua_pushstring(L, response.error.c_str());
        lua_setfield(L, -2, "error");
    } else {
        lua_pushstring(L, "");
        lua_setfield(L, -2, "error");
    }

    return 1;
}

static int l_json_parse(lua_State* L) {
    const char* text = nullptr;
    bool popped_body = false;

    if (lua_isstring(L, 1)) {
        text = lua_tostring(L, 1);
    } else if (lua_istable(L, 1)) {
        lua_getfield(L, 1, "body");
        if (!lua_isstring(L, -1)) {
            lua_pop(L, 1);
            lua_pushnil(L);
            lua_pushstring(L, "json_parse expected a JSON string or an http_fetch result table");
            return 2;
        }
        text = lua_tostring(L, -1);
        popped_body = true;
    } else {
        return luaL_argerror(L, 1, "string or http_fetch result table expected");
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, text);

    // Clean up the pushed "body" string from stack after parsing finishes
    if (popped_body) {
        lua_remove(L, -1);
    }

    if (err) {
        lua_pushnil(L);
        lua_pushstring(L, err.c_str());
        return 2;
    }

    lua_push_json_value(L, doc.as<JsonVariantConst>());
    return 1;
}

static const luaL_Reg cnfig_lib[] = {
    {"setThemeColor", l_config_set_theme_color},
    {"setBootDelay", l_config_set_boot_delay},
    {NULL, NULL}
};

static const luaL_Reg gpio_lib[] = {
    {"read", l_gpio_read},
    {"write", l_gpio_write},
    {NULL, NULL}
};

static const luaL_Reg ir_lib[] = {
    {"sendProtocol", l_ir_send_protocol},
    {"sendRaw", l_ir_send_raw},
    {"receiveAvbl", l_ir_receive_available},
    {"receiveGetRes", l_ir_receive_get_result},
    {"receiveResume", l_ir_receive_resume},
    {"receiveGetAddress", l_ir_receive_get_address},
    {"receiveGetCmd", l_ir_receive_get_command},
    {"receiveGetProtocol", l_ir_receive_get_protocol},
    {NULL, NULL}
};

static const luaL_Reg sys_lib[] = {
    {"serialPrint", l_serial_print},
    {"millis", l_millis},
    {"keyPressed", l_key_is_pressed},
    {"getImu", l_get_imu},
    {NULL, NULL}
};

static const luaL_Reg wifi_lib[] = {
    {"connect", l_wifi_connect},
    {"connected", l_wifi_is_connected},
    {"getIP", l_wifi_get_ip},
    {"disconnect", l_wifi_disconnect},
    {"getRssi", l_wifi_get_rssi},
    {"addNetwork", l_wifi_add_network},
    {"setAutoconnect", l_wifi_set_autoconnect},
    {"getAutoconnect", l_wifi_get_autoconnect},
    {"getSSID", l_wifi_get_ssid},
    {"remNetwork", l_wifi_remove_network},
    {"getNetworkCount", l_wifi_get_network_count},
    {"getNetworkSSID", l_wifi_get_network_ssid},
    {"scanStart", l_wifi_scan_start},
    {"scanStatus", l_wifi_scan_status},
    {"scanCount", l_wifi_scan_count},
    {"scanGet", l_wifi_scan_get},
    {NULL, NULL}
};

static const luaL_Reg ble_lib[] = {
    {"scanStart", l_ble_scan_start},
    {"scanStatus", l_ble_scan_status},
    {"scanCount", l_ble_scan_count},
    {"scanGet", l_ble_scan_get},
    {"parseAdvert", l_ble_parse_advertisement},
    {NULL, NULL}
};

static const luaL_Reg http_lib[] = {
    {"fetch", l_http_fetch},
    {"jsonParse", l_json_parse},
    {NULL, NULL}
};

static const luaL_Reg gfx_lib[] = {
    {"listClr", l_list_clear},
    {"listAddItem", l_list_add_item},
    {"listDraw", l_list_draw},
    {"listHandleKey", l_list_handle_key},
    {"listGetSelIndex", l_list_get_selected_index},
    {"listGetSelText", l_list_get_selected_text},
    {"drawStatusBar", l_draw_status_bar},
    {"drawText", l_draw_text},
    {"clearScreen", l_clear_screen},
    {"setColor", l_set_color},
    {"drawRect", l_draw_rect},
    {"drawRectF", l_draw_rect_full},
    {"drawLine", l_draw_line},
    {"drawTri", l_draw_triangle},
    {"drawTriF", l_draw_triangle_full},
    {"drawCircle", l_draw_circle},
    {"drawCircleF", l_draw_circle_full},
    {"textInputStart", l_text_input_start},
    {"textInputDraw", l_text_input_draw},
    {"textInputHandleKey", l_text_input_handle_key},
    {"textInputGetVal", l_text_input_get_value},
    {"fileBrowserStart", l_file_browser_start},
    {"fileBrowserDraw", l_file_browser_draw},
    {"fileBrowserHandleKey", l_file_browser_handle_key},
    {"fileBrowserGetSelectedPath", l_file_browser_get_selected_path},
    {NULL, NULL}
};

void register_config_bindings(lua_State* L) {
    luaL_newlib(L, cnfig_lib);
    lua_setglobal(L, "cnfig");
}

void register_gpio_bindings(lua_State* L) {
    luaL_newlib(L, gpio_lib);
    lua_setglobal(L, "gpio");
}

void register_ir_bindings(lua_State* L) {
    luaL_newlib(L, ir_lib);
    lua_setglobal(L, "ir");
}

void register_system_bindings(lua_State* L) {
    luaL_newlib(L, sys_lib);
    lua_setglobal(L, "sys");
}

void register_wifi_bindings(lua_State* L) {
    luaL_newlib(L, wifi_lib);
    lua_setglobal(L, "wifi");
}

void register_ble_bindings(lua_State* L) {
    luaL_newlib(L, ble_lib);
    lua_setglobal(L, "ble");
}

void register_http_bindings(lua_State* L) {
    luaL_newlib(L, http_lib);
    lua_setglobal(L, "http");
}

void registergfx_bindings(lua_State* L) {
    luaL_newlib(L, gfx_lib);
    lua_setglobal(L, "gfx");
}