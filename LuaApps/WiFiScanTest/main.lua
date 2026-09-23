local mode = "scan"
local networks = {}
local scan_started = false
local last_status = "idle"

local function show_header(text)
    gfx.clearScreen()
    gfx.drawText("WiFi Scan Test", 8, 25, 2)
    gfx.drawText(text, 8, 42, 1)
end

local function start_scan()
    networks = {}
    scan_started = wifi.scanStart(false)
    last_status = wifi.scanStatus()
    mode = "scan"
    show_header(scan_started and "Scanning..." or "Scan busy/failed")
    gfx.listClr()
    gfx.listAddItem("Rescan")
    gfx.listAddItem("Please wait...")
    gfx.listDraw()
end

local function show_results()
    gfx.listClr()
    gfx.listAddItem("Rescan")
    for i = 0, #networks - 1 do
        local network = networks[i + 1]
        local name = network.ssid
        if name == "" then name = "<hidden>" end
        gfx.listAddItem(string.format("%s  %ddBm", name, network.rssi))
    end
    if #networks == 0 then gfx.listAddItem("No networks found") end
    show_header(string.format("Found %d network(s)", #networks))
    gfx.listDraw()
end

local function show_network(index)
    local network = networks[index]
    if not network then return end
    mode = "detail"
    show_header("Network details")
    gfx.drawText("SSID: " .. (network.ssid ~= "" and network.ssid or "<hidden>"), 8, 58, 1)
    gfx.drawText("RSSI: " .. network.rssi .. " dBm", 8, 72, 1)
    gfx.drawText("Channel: " .. network.channel, 8, 86, 1)
    gfx.drawText("BSSID: " .. network.bssid, 8, 100, 1)
    gfx.drawText("Auth: " .. network.encryption, 8, 114, 1)
    gfx.drawText("Enter/back to return", 8, 128, 1)
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
            show_network(result)
        end
    else
        gfx.listDraw()
    end
end

function update()
    if mode ~= "scan" then return end
    local status = wifi.scanStatus()
    if status ~= last_status then
        last_status = status
        if status == "complete" then
            networks = {}
            for i = 0, wifi.scanCount() - 1 do
                table.insert(networks, wifi.scanGet(i))
            end
            show_results()
        elseif status == "idle" or status == "failed" then
            show_header("Scan unavailable")
            gfx.listClr()
            gfx.listAddItem("Rescan")
            gfx.listDraw()
        else
            show_header("Scanning...")
        end
    end
end
