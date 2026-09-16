local mode = "scan"
local devices = {}
local last_status = "idle"

local function show_header(text)
    clear_screen()
    draw_text("BLE Scan Test", 8, 25, 2)
    draw_text(text, 8, 42, 1)
end

local function start_scan()
    devices = {}
    last_status = ble_scan_status()
    ble_scan_start(5, true)
    mode = "scan"
    show_header("Scanning for 5 seconds...")
    list_clear()
    list_add_item("Rescan")
    list_add_item("Please wait...")
    list_draw()
end

local function show_results()
    list_clear()
    list_add_item("Rescan")
    for i = 1, #devices do
        local device = devices[i]
        local name = device.name
        if name == "" then name = "<unnamed>" end
        list_add_item(string.format("%s  %ddBm", name, device.rssi))
    end
    if #devices == 0 then list_add_item("No BLE devices found") end
    show_header(string.format("Found %d device(s)", #devices))
    list_draw()
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
    draw_text("Name: " .. name, 8, 58, 1)
    draw_text("RSSI: " .. device.rssi .. " dBm", 8, 72, 1)
    draw_text("Addr: " .. device.address, 8, 86, 1)
    draw_text("Services: " .. #device.service_uuids, 8, 100, 1)
    draw_text("Mfr bytes: " .. byte_count(device.manufacturer_data), 8, 114, 1)
    draw_text("Payload: " .. byte_count(device.payload) .. " bytes", 8, 128, 1)
end

start_scan()

function on_key(key)
    if mode == "detail" then
        if key == 8 or key == '\r' then show_results() end
        return
    end

    local result = list_handle_key(key)
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
        list_draw()
    end
end

function update()
    if mode ~= "scan" then return end
    local status = ble_scan_status()
    if status ~= last_status then
        last_status = status
        if status == "complete" then
            devices = {}
            for i = 0, ble_scan_count() - 1 do
                table.insert(devices, ble_scan_get(i))
            end
            show_results()
        elseif status == "unavailable" then
            show_header("BLE unavailable")
            list_clear()
            list_add_item("Rescan")
            list_draw()
        end
    end
end
