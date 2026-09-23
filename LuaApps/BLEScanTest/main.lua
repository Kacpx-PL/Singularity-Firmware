local mode = "scan"
local devices = {}
local last_status = "idle"

local function show_header(text)
    gfx.clearScreen()
    gfx.drawText("BLE Scan Test", 8, 25, 2)
    gfx.drawText(text, 8, 42, 1)
end

local function start_scan()
    devices = {}
    last_status = ble.scanStatus()
    ble.scanStart(5, true)
    mode = "scan"
    show_header("Scanning for 5 seconds...")
    gfx.listClr()
    gfx.listAddItem("Rescan")
    gfx.listAddItem("Please wait...")
    gfx.listDraw()
end

local function show_results()
    gfx.listClr()
    gfx.listAddItem("Rescan")
    for i = 1, #devices do
        local device = devices[i]
        local name = device.name
        if name == "" then name = "<unnamed>" end
        gfx.listAddItem(string.format("%s  %ddBm", name, device.rssi))
    end
    if #devices == 0 then gfx.listAddItem("No BLE devices found") end
    show_header(string.format("Found %d device(s)", #devices))
    gfx.listDraw()
end

local function byte_count(bytes)
    if bytes == nil then return 0 end
    return #bytes
end

local function show_device(index)
    local device = devices[index]
    if not device then return end
    mode = "detail"
    show_header("Device details")
    local name = device.name
    if name == "" then name = "<unnamed>" end
    gfx.drawText("Name: " .. name, 8, 58, 1)
    gfx.drawText("RSSI: " .. device.rssi .. " dBm", 8, 72, 1)
    gfx.drawText("Addr: " .. device.address, 8, 86, 1)
    gfx.drawText("Services: " .. #device.service_uuids, 8, 100, 1)
    gfx.drawText("Mfr bytes: " .. byte_count(device.manufacturer_data), 8, 114, 1)
    gfx.drawText("Payload: " .. byte_count(device.payload) .. " bytes", 8, 128, 1)
end

start_scan()

function on_key(key)
    if mode == "detail" then
        if key == 8 or key == '\r' then show_results() end
        return
    end

    local result = gfx.listHandleKey(key)
    if key == 8 then
        return
    end
    if result ~= nil and result >= 0 then
        if result == 0 then
            start_scan()
        elseif last_status == "complete" and result > 0 then
            show_device(result)
        end
    else
        gfx.listDraw()
    end
end

function update()
    if mode ~= "scan" then return end
    local status = ble.scanStatus()
    if status ~= last_status then
        last_status = status
        if status == "complete" then
            devices = {}
            for i = 0, ble.scanCount() - 1 do
                table.insert(devices, ble.scanGet(i))
            end
            show_results()
        elseif status == "unavailable" then
            show_header("BLE unavailable")
            gfx.listClr()
            gfx.listAddItem("Rescan")
            gfx.listDraw()
        end
    end
end
