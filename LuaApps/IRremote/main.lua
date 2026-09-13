local mode = "browse"
local signals = {}

local function parse_ir_file(content)
    local result = {}
    local current = nil

    for line in content:gmatch("[^\r\n]+") do
        if line:match("^#") then
            if current and current.name then
                table.insert(result, current)
            end
            current = {}
        elseif current then
            local key, value = line:match("^(%w+):%s*(.+)$")
            if key then
                current[key] = value
            end
        end
    end

    if current and current.name then
        table.insert(result, current)
    end

    return result
end

local function parse_raw_data(datastr)
    local values = {}
    for num in datastr:gmatch("%d+") do
        table.insert(values, tonumber(num))
    end
    return values
end

local function hex_bytes_to_int(hexstr)
    local bytes = {}
    for b in hexstr:gmatch("%x%x") do
        table.insert(bytes, tonumber(b, 16))
    end
    local value = 0
    for i = #bytes, 1, -1 do
        value = (value << 8) | bytes[i]
    end
    return value
end

file_browser_start("/singularity/system/ir_db", false)

clear_screen()
file_browser_draw()

function on_key(key)
    if mode == "browse" then
        local result = file_browser_handle_key(key)
        clear_screen()
        file_browser_draw()

        if result == 1 then
            local path = file_browser_get_selected_path()
            local content = storage_read(path)

            if content == nil then
                clear_screen()
                draw_text("Failed to read file", 10, 25, 1)
            else
                signals = parse_ir_file(content)
				serial_print("Parsed signal count: " .. #signals)
				
                list_clear()
                for _, sig in ipairs(signals) do
                    list_add_item(sig.name)
                end

                mode = "signals"
                clear_screen()
                draw_text("Select signal:", 10, 25, 2)
                list_draw()
            end
        end

    elseif mode == "signals" then
        local result = list_handle_key(key)
        clear_screen()
        draw_text("Select signal:", 10, 25, 2)
        list_draw()

        if key == 8 then -- \b is 8
            -- back out to file browser
            mode = "browse"
            clear_screen()
            file_browser_draw()
        elseif result ~= nil and result >= 0 then
            local sig = signals[result + 1]

            if sig.type == "raw" then
                local data = parse_raw_data(sig.data)
                local freq = tonumber(sig.frequency) or 38000
                ir_send_raw(data, freq / 1000) -- convert Hz to kHz for sendRaw's expected units
            else
                local address = hex_bytes_to_int(sig.address)
                local command = hex_bytes_to_int(sig.command)
                ir_send_protocol(sig.protocol, address, command)
            end

            clear_screen()
            draw_text("Select signal:", 10, 25, 2)
            list_draw()
            draw_text("Sent: " .. sig.name, 15, 45, 1)
        end
    end
end

function update()
end