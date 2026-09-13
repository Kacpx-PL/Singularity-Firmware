local mode = "list"
local SG_BOOT_DELAY_MIN = 1000  --setting this below 1000 will not change anything.
local SG_BOOT_DELAY_MAX = 10000 --setting this above 10000 will not change anything.

function refresh_list()
    list_clear()
    list_add_item("Set Theme Color (hex)")
    list_add_item("Set Boot Time (ms)")
end

clear_screen()
draw_text("System Settings", 10, 40, 2)
refresh_list()
list_draw()

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
        local result = list_handle_key(key)
        clear_screen()
        draw_text("System Settings", 10, 40, 2)
        list_draw()

        if result == 0 then
            mode = "set_color"
            clear_screen()
            text_input_start("Hex color (e.g. FF6600):", false)
            text_input_draw()
        
        elseif result == 1 then
            mode = "set_delay"
            clear_screen()
            text_input_start("Boot Time (e.g. 3000):", false)
            text_input_draw()
        end

    elseif mode == "set_color" then
        local r = text_input_handle_key(key)

        if r == 2 then
            -- cancelled (backspace on empty)
            mode = "list"
            clear_screen()
            draw_text("System Settings", 10, 40, 2)
            refresh_list()
            list_draw()
        else
            text_input_draw()
            if r == 1 then
                local hex_value = text_input_get_value()
                local rgb565 = hex_to_rgb565(hex_value)

                clear_screen()
                if rgb565 == nil then
                    draw_text("Invalid hex color!", 10, 40, 1)
                else
                    config_set_theme_color(rgb565)
                    draw_text("Theme updated!", 10, 30, 2)
                    draw_text("Value: 0x" .. string.format("%04X", rgb565), 10, 55, 1)
                end

                mode = "list"
            end
        end

    elseif mode == "set_delay" then
        local r = text_input_handle_key(key)

        if r == 2 then
            -- cancelled (backspace on empty)
            mode = "list"
            clear_screen()
            draw_text("System Settings", 10, 40, 2)
            refresh_list()
            list_draw()
        else
            text_input_draw()
            if r == 1 then
                local raw_value = tonumber(text_input_get_value())

                clear_screen()
                if raw_value == nil then
                    draw_text("Invalid number!", 10, 40, 1)
                else
                    local boot_delay = math.clamp(raw_value, SG_BOOT_DELAY_MIN, SG_BOOT_DELAY_MAX)
                    config_set_boot_delay(boot_delay)
                    draw_text("Boot time updated!", 10, 30, 2)
                    draw_text("Time: " .. boot_delay .. "ms", 10, 55, 1)
                end

                mode = "list"
            end
        end
    end
end

function update()
end