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

gfx.fileBrowserStart("/singularity/system/ir_db", false)

gfx.clearScreen()
gfx.fileBrowserDraw()

function on_key(key)
    if mode == "browse" then
        local result = gfx.fileBrowserHandleKey(key)
        gfx.clearScreen()
        gfx.fileBrowserDraw()

        if result == 1 then
            local path = gfx.fileBrowserGetSelectedPath()
            local content = storage_read(path)

            if content == nil then
                gfx.clearScreen()
                gfx.drawText("Failed to read file", 10, 25, 1)
            else
                signals = parse_ir_file(content)
				sys.serialPrint("Parsed signal count: " .. #signals)
				
                gfx.listClr()
                for _, sig in ipairs(signals) do
                    gfx.listAddItem(sig.name)
                end

                mode = "signals"
                gfx.clearScreen()
                gfx.drawText("Select signal:", 10, 25, 2)
                gfx.listDraw()
            end
        end

    elseif mode == "signals" then
        local result = gfx.listHandleKey(key)
        gfx.clearScreen()
        gfx.drawText("Select signal:", 10, 25, 2)
        gfx.listDraw()

        if key == 8 then -- \b is 8
            -- back out to file browser
            mode = "browse"
            gfx.clearScreen()
            gfx.fileBrowserDraw()
        elseif result ~= nil and result >= 0 then
            local sig = signals[result + 1]

            if sig.type == "raw" then
                local data = parse_raw_data(sig.data)
                local freq = tonumber(sig.frequency) or 38000
                ir.sendRaw(data, freq / 1000) -- convert Hz to kHz for sendRaw's expected units
            else
                local address = hex_bytes_to_int(sig.address)
                local command = hex_bytes_to_int(sig.command)
                ir.sendProtocol(sig.protocol, address, command)
            end

            gfx.clearScreen()
            gfx.drawText("Select signal:", 10, 25, 2)
            gfx.listDraw()
            gfx.drawText("Sent: " .. sig.name, 15, 45, 1)
        end
    end
end

function update()
end