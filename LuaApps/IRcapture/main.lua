local mode = "menu"
local locked_protocol = nil
local captured_signals = {}
local pending_signal = nil
local target_path = nil

local function show_menu()
    clear_screen()
    draw_text("IR Capture", 10, 25, 2)
    list_clear()
    list_add_item("Load existing remote")
    list_add_item("Create new remote")
    list_draw()
end

show_menu()

local function show_status()
    clear_screen()
    draw_text("IR Capture", 10, 25, 1)
    if locked_protocol then
        draw_text("Protocol: " .. locked_protocol, 5, 40, 1)
    end
    draw_text("Signals: " .. #captured_signals, 5, 55, 1)
    draw_text("Press remote button...", 5, 70, 1)
end

local function parse_ir_file(content)
    local result = {}
    local current = nil
    for line in content:gmatch("[^\r\n]+") do
        if line:match("^#") then
            if current and current.name then table.insert(result, current) end
            current = {}
        elseif current then
            local key, value = line:match("^(%w+):%s*(.+)$")
            if key then current[key] = value end
        end
    end
    if current and current.name then table.insert(result, current) end
    return result
end

local function hex_bytes_to_int(hexstr)
    local bytes = {}
    for b in hexstr:gmatch("%x%x") do table.insert(bytes, tonumber(b, 16)) end
    local value = 0
    for i = #bytes, 1, -1 do value = (value << 8) | bytes[i] end
    return value
end

local function save_ir_file()
    local content = "Filetype: IR signals file\nVersion: 1\n"
    for _, sig in ipairs(captured_signals) do
        content = content .. "#\n"
        content = content .. "name: " .. sig.name .. "\n"
        content = content .. "type: parsed\n"
        content = content .. "protocol: " .. sig.protocol .. "\n"
        content = content .. string.format("address: %02X 00 00 00\n", sig.address)
        content = content .. string.format("command: %02X 00 00 00\n", sig.command)
    end
    storage_write(target_path, content)
end

function on_key(key)
    if mode == "menu" then
        local result = list_handle_key(key)
        clear_screen()
        draw_text("IR Capture", 10, 25, 2)
        list_draw()

        if result == 0 then
            mode = "browse_load"
            file_browser_start("/singularity/system/ir_db", false)
            clear_screen()
            file_browser_draw()
        elseif result == 1 then
            mode = "browse_folder"
            file_browser_start("/singularity/system/ir_db", true)
            clear_screen()
            file_browser_draw()
        end

    elseif mode == "browse_load" then
        local result = file_browser_handle_key(key)
        clear_screen()
        file_browser_draw()

        if result == 1 then
            target_path = file_browser_get_selected_path()
            local content = storage_read(target_path)
            captured_signals = content and parse_ir_file(content) or {}
            if #captured_signals > 0 then
                locked_protocol = captured_signals[1].protocol
            end
            mode = "capture"
            show_status()
        end

    elseif mode == "browse_folder" then
        local result = file_browser_handle_key(key)
        clear_screen()
        file_browser_draw()

        if result == 1 then
            local folder = file_browser_get_selected_path()
            mode = "naming_file"
            text_input_start("Remote name:", false)
            text_input_draw()
            target_path = folder -- store folder temporarily, finalize after naming
        end

    elseif mode == "naming_file" then
        local r = text_input_handle_key(key)
        if r == 2 then
            mode = "menu"
            show_menu()
        else
            text_input_draw()
            if r == 1 then
                local name = text_input_get_value()
                target_path = target_path .. "/" .. name .. ".ir"
                captured_signals = {}
                locked_protocol = nil
                mode = "capture"
                show_status()
            end
        end

    elseif mode == "naming_signal" then
        local r = text_input_handle_key(key)
        if r == 2 then
            pending_signal = nil
            mode = "capture"
            show_status()
        else
            text_input_draw()
            if r == 1 then
                pending_signal.name = text_input_get_value()
                table.insert(captured_signals, pending_signal)
                if locked_protocol == nil then
                    locked_protocol = pending_signal.protocol
                end
                pending_signal = nil
                save_ir_file()
                mode = "capture"
                show_status()
            end
        end
    end
end

function update()
    if mode == "capture" and ir_receive_available() then
        local protocol = ir_receive_get_protocol()
        local address = ir_receive_get_address()
        local command = ir_receive_get_command()
        ir_receive_resume()

        if command == 0 and address == 0 then return end

        if locked_protocol ~= nil and protocol ~= locked_protocol then
            clear_screen()
            draw_text("Protocol mismatch!", 5, 25, 1)
            draw_text("Expected: " .. locked_protocol, 5, 40, 1)
            draw_text("Got: " .. protocol, 5, 55, 1)
            return
        end

        pending_signal = { protocol = protocol, address = address, command = command }
        mode = "naming_signal"

        clear_screen()
        draw_text("Captured!", 10, 25, 1)
        draw_text(protocol .. " 0x" .. string.format("%04X", address) .. " 0x" .. string.format("%02X", command), 5, 30, 1)
        text_input_start("Name this button:", false)
        text_input_draw()
    end
end