-- Ensure default config file exists
local CONFIG_PATH = "/singularity/system/config.lua"
if not storage_exists(CONFIG_PATH) then
    local default_config = "return {\n    theme_color = 2016,\n    boot_delay_ms = 3000,\n}\n"
    storage_write(CONFIG_PATH, default_config)
end

local mode = "list"
local SG_BOOT_DELAY_MIN = 1000  --setting this below 1000 will not change anything.
local SG_BOOT_DELAY_MAX = 10000 --setting this above 10000 will not change anything.

function refresh_list()
    gfx.listClr()
    gfx.listAddItem("Set Theme Color (hex)")
    gfx.listAddItem("Set Boot Time (ms)")
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
    end
end

function update()
end