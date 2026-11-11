local mode = "menu"
local locked_protocol = nil
local captured_signals = {}
local pending_signal = nil
local target_path = nil

local function show_menu()
    gfx.clearScreen()
    gfx.drawText("IR Capture", 10, 25, 2)
    gfx.listClr()
    gfx.listAddItem("Load existing remote")
    gfx.listAddItem("Create new remote")
    gfx.listDraw()
end

show_menu()

local function show_status()
    gfx.clearScreen()
    gfx.drawText("IR Capture", 10, 25, 1)
    if locked_protocol then
        gfx.drawText("Protocol: " .. locked_protocol, 5, 40, 1)
    end
    gfx.drawText("Signals: " .. #captured_signals, 5, 55, 1)
    gfx.drawText("Press remote button...", 5, 70, 1)
end

local function parse_ir_file(content)
    local result = {}
    local current = nil
    for line in content:gmatch("[^\r\n]+") do
        if line:match("^#") then
            if current and current.name then table.insert(result, current) end
            current = {}
        elseif current then
            local key, value = line:match("^([%w_]+):%s*(.+)$") -- [%w_] so keys like duty_cycle survive a re-save
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

-- Flipper .ir files store numbers as little-endian bytes: 0x1A2B -> "2B 1A 00 00"
local function int_to_hex_bytes(n)
    return string.format("%02X %02X %02X %02X",
        n & 0xFF, (n >> 8) & 0xFF, (n >> 16) & 0xFF, (n >> 24) & 0xFF)
end

-- Every signal (freshly captured OR loaded from an existing file) keeps its fields as the
-- strings used by the file format, so loaded remotes can be re-saved unchanged.
local function save_ir_file()
    local content = "Filetype: IR signals file\nVersion: 1\n"
    for _, sig in ipairs(captured_signals) do
        content = content .. "#\n"
        content = content .. "name: " .. tostring(sig.name or "unnamed") .. "\n"
        content = content .. "type: " .. tostring(sig.type or "parsed") .. "\n"
        for _, k in ipairs({ "protocol", "address", "command", "frequency", "duty_cycle", "data" }) do
            if sig[k] ~= nil then
                content = content .. k .. ": " .. tostring(sig[k]) .. "\n"
            end
        end
    end
    storage_write(target_path, content)
end

function on_key(key)
    if mode == "menu" then
        local result = gfx.listHandleKey(key)
        gfx.clearScreen()
        gfx.drawText("IR Capture", 10, 25, 2)
        gfx.listDraw()

        if result == 0 then
            mode = "browse_load"
            gfx.fileBrowserStart("/singularity/system/ir_db", false)
            gfx.clearScreen()
            gfx.fileBrowserDraw()
        elseif result == 1 then
            mode = "browse_folder"
            gfx.fileBrowserStart("/singularity/system/ir_db", true)
            gfx.clearScreen()
            gfx.fileBrowserDraw()
        end

    elseif mode == "browse_load" then
        local result = gfx.fileBrowserHandleKey(key)
        gfx.clearScreen()
        gfx.fileBrowserDraw()

        if result == 1 then
            target_path = gfx.fileBrowserGetSelectedPath()
            local content = storage_read(target_path)
            captured_signals = content and parse_ir_file(content) or {}
            if #captured_signals > 0 then
                locked_protocol = captured_signals[1].protocol
            end
            mode = "capture"
            show_status()
        end

    elseif mode == "browse_folder" then
        local result = gfx.fileBrowserHandleKey(key)
        gfx.clearScreen()
        gfx.fileBrowserDraw()

        if result == 1 then
            local folder = gfx.fileBrowserGetSelectedPath()
            mode = "naming_file"
            gfx.textInputStart("Remote name:", false)
            gfx.textInputDraw()
            target_path = folder -- store folder temporarily, finalize after naming
        end

    elseif mode == "naming_file" then
        local r = gfx.textInputHandleKey(key)
        if r == 2 then
            mode = "menu"
            show_menu()
        else
            gfx.textInputDraw()
            if r == 1 then
                local name = gfx.textInputGetVal():gsub("[^%w%-_ ]", "_") -- no path characters in file names
                if name == "" then name = "remote" end
                target_path = target_path .. "/" .. name .. ".ir"
                captured_signals = {}
                locked_protocol = nil
                mode = "capture"
                show_status()
            end
        end

    elseif mode == "naming_signal" then
        local r = gfx.textInputHandleKey(key)
        if r == 2 then
            pending_signal = nil
            mode = "capture"
            show_status()
        else
            gfx.textInputDraw()
            if r == 1 then
                pending_signal.name = gfx.textInputGetVal()
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
    if mode == "capture" and ir.receiveAvbl() then
        local protocol = ir.receiveGetProtocol()
        local address = ir.receiveGetAddress()
        local command = ir.receiveGetCmd()
        ir.receiveResume()

        if command == 0 and address == 0 then return end

        if locked_protocol ~= nil and protocol ~= locked_protocol then
            gfx.clearScreen()
            gfx.drawText("Protocol mismatch!", 5, 25, 1)
            gfx.drawText("Expected: " .. locked_protocol, 5, 40, 1)
            gfx.drawText("Got: " .. protocol, 5, 55, 1)
            return
        end

        pending_signal = {
            type = "parsed",
            protocol = protocol,
            address = int_to_hex_bytes(address),
            command = int_to_hex_bytes(command),
        }
        mode = "naming_signal"

        gfx.clearScreen()
        gfx.drawText("Captured!", 10, 25, 1)
        gfx.drawText(protocol .. " 0x" .. string.format("%04X", address) .. " 0x" .. string.format("%02X", command), 5, 30, 1)
        gfx.textInputStart("Name this button:", false)
        gfx.textInputDraw()
    end
end