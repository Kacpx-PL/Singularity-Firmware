-- Ensure default config file exists
local CONFIG_PATH = "/singularity/system/config.lua"
if not storage_exists(CONFIG_PATH) then
    local default_config = "return {\n    theme_color = 2016,\n    boot_delay_ms = 3000,\n    volume = 5,\n    timezone = \"UTC0\",\n}\n"
    storage_write(CONFIG_PATH, default_config)
end

local mode = "list"
local SG_BOOT_DELAY_MIN = 1000  --setting this below 1000 will not change anything.
local SG_BOOT_DELAY_MAX = 10000 --setting this above 10000 will not change anything.

function refresh_list()
    gfx.listClr()
    gfx.listAddItem("Set Theme Color (hex)")
    gfx.listAddItem("Set Boot Time (ms)")
    gfx.listAddItem("Set Timezone")
end

gfx.clearScreen()
gfx.drawText("System Settings", 10, 40, 2)
refresh_list()
gfx.listDraw()

function math.clamp(x, min, max)
    return math.min(math.max(x, min), max)
end

local function hex_to_rgb565(hex)
    hex = hex:gsub("#", "")
    if #hex ~= 6 then
        return nil
    end

    local r = tonumber(hex:sub(1, 2), 16)
    local g = tonumber(hex:sub(3, 4), 16)
    local b = tonumber(hex:sub(5, 6), 16)

    if r == nil or g == nil or b == nil then
        return nil
    end

    local r5 = r >> 3
    local g6 = g >> 2
    local b5 = b >> 3

    return (r5 << 11) | (g6 << 5) | b5
end

-- Timezones are POSIX TZ strings (note the inverted sign: UTC+1 is "-1").
-- Anything not listed here can be entered with "Custom".
local TIMEZONES = {
    {"UTC",                      "UTC0"},
    {"London",                   "GMT0BST,M3.5.0/1,M10.5.0"},
    {"Central Europe (CET)",     "CET-1CEST,M3.5.0,M10.5.0/3"},
    {"Eastern Europe (EET)",     "EET-2EEST,M3.5.0/3,M10.5.0/4"},
    {"US Eastern",               "EST5EDT,M3.2.0,M11.1.0"},
    {"US Central",               "CST6CDT,M3.2.0,M11.1.0"},
    {"US Mountain",              "MST7MDT,M3.2.0,M11.1.0"},
    {"US Pacific",               "PST8PDT,M3.2.0,M11.1.0"},
    {"Japan",                    "JST-9"},
    {"Sydney",                   "AEST-10AEDT,M10.1.0,M4.1.0/3"},
}
local SG_TIMEZONE_MAX_LEN = 47

local function show_main_list()
    gfx.clearScreen()
    gfx.drawText("System Settings", 10, 40, 2)
    refresh_list()
    gfx.listDraw()
end

local function show_tz_list()
    gfx.listClr()
    gfx.listAddItem("< Back")
    for _, tz in ipairs(TIMEZONES) do
        gfx.listAddItem(tz[1])
    end
    gfx.listAddItem("Custom (POSIX TZ)")
    gfx.clearScreen()
    gfx.drawText("Timezone", 10, 40, 2)
    gfx.listDraw()
end

function on_key(key)
    if mode == "list" then
        local result = gfx.listHandleKey(key)
        gfx.clearScreen()
        gfx.drawText("System Settings", 10, 40, 2)
        gfx.listDraw()

        if result == 0 then
            mode = "gfx.setColor"
            gfx.clearScreen()
            gfx.textInputStart("Hex color (e.g. FF6600):", false)
            gfx.textInputDraw()
        
        elseif result == 1 then
            mode = "set_delay"
            gfx.clearScreen()
            gfx.textInputStart("Boot Time (e.g. 3000):", false)
            gfx.textInputDraw()

        elseif result == 2 then
            mode = "tz_list"
            show_tz_list()
        end

    elseif mode == "gfx.setColor" then
        local r = gfx.textInputHandleKey(key)

        if r == 2 then
            -- cancelled (backspace on empty)
            mode = "list"
            gfx.clearScreen()
            gfx.drawText("System Settings", 10, 40, 2)
            refresh_list()
            gfx.listDraw()
        else
            gfx.textInputDraw()
            if r == 1 then
                local hex_value = gfx.textInputGetVal()
                local rgb565 = hex_to_rgb565(hex_value)

                gfx.clearScreen()
                if rgb565 == nil then
                    gfx.drawText("Invalid hex color!", 10, 40, 1)
                else
                    cnfig.setThemeColor(rgb565)
                    gfx.drawText("Theme updated!", 10, 30, 2)
                    gfx.drawText("Value: 0x" .. string.format("%04X", rgb565), 10, 55, 1)
                end

                mode = "list"
            end
        end

    elseif mode == "set_delay" then
        local r = gfx.textInputHandleKey(key)

        if r == 2 then
            -- cancelled (backspace on empty)
            mode = "list"
            gfx.clearScreen()
            gfx.drawText("System Settings", 10, 40, 2)
            refresh_list()
            gfx.listDraw()
        else
            gfx.textInputDraw()
            if r == 1 then
                local raw_value = tonumber(gfx.textInputGetVal())

                gfx.clearScreen()
                if raw_value == nil then
                    gfx.drawText("Invalid number!", 10, 40, 1)
                else
                    local boot_delay = math.clamp(raw_value, SG_BOOT_DELAY_MIN, SG_BOOT_DELAY_MAX)
                    cnfig.setBootDelay(boot_delay)
                    gfx.drawText("Boot time updated!", 10, 30, 2)
                    gfx.drawText("Time: " .. boot_delay .. "ms", 10, 55, 1)
                end

                mode = "list"
            end
        end

    elseif mode == "tz_list" then
        local result = gfx.listHandleKey(key)
        gfx.clearScreen()
        gfx.drawText("Timezone", 10, 40, 2)
        gfx.listDraw()

        if result == 0 then
            mode = "list"
            show_main_list()

        elseif result >= 1 and result <= #TIMEZONES then
            local tz = TIMEZONES[result]
            cnfig.setTimezone(tz[2])
            gfx.clearScreen()
            gfx.drawText("Timezone updated!", 10, 30, 2)
            gfx.drawText(tz[1], 10, 55, 1)
            mode = "list"
            refresh_list()   -- list widget still holds the timezones; restore the main items

        elseif result == #TIMEZONES + 1 then
            mode = "tz_custom"
            gfx.clearScreen()
            gfx.textInputStart("POSIX TZ string:", false)
            gfx.textInputDraw()
        end

    elseif mode == "tz_custom" then
        local r = gfx.textInputHandleKey(key)

        if r == 2 then
            -- cancelled (backspace on empty): back to the timezone list
            mode = "tz_list"
            show_tz_list()
        else
            gfx.textInputDraw()
            if r == 1 then
                local value = gfx.textInputGetVal()

                gfx.clearScreen()
                if #value < 1 or #value > SG_TIMEZONE_MAX_LEN then
                    gfx.drawText("Invalid timezone!", 10, 40, 1)
                else
                    cnfig.setTimezone(value)
                    gfx.drawText("Timezone updated!", 10, 30, 2)
                    gfx.drawText(value, 10, 55, 1)
                end

                mode = "list"
                refresh_list()   -- list widget still holds the timezones; restore the main items
            end
        end
    end
end

function update()
end