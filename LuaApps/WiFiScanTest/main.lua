local mode = "scan"
local networks = {}
local scan_started = false
local last_status = "idle"

local function show_header(text)
    clear_screen()
    draw_text("WiFi Scan Test", 8, 25, 2)
    draw_text(text, 8, 42, 1)
end

local function start_scan()
    networks = {}
    scan_started = wifi_scan_start(false)
    last_status = wifi_scan_status()
    mode = "scan"
    show_header(scan_started and "Scanning..." or "Scan busy/failed")
    list_clear()
    list_add_item("Rescan")
    list_add_item("Please wait...")
    list_draw()
end

local function show_results()
    list_clear()
    list_add_item("Rescan")
    for i = 0, #networks - 1 do
        local network = networks[i + 1]
        local name = network.ssid
        if name == "" then name = "<hidden>" end
        list_add_item(string.format("%s  %ddBm", name, network.rssi))
    end
    if #networks == 0 then list_add_item("No networks found") end
    show_header(string.format("Found %d network(s)", #networks))
    list_draw()
end

local function show_network(index)
    local network = networks[index]
    if not network then return end
    mode = "detail"
    show_header("Network details")
    draw_text("SSID: " .. (network.ssid ~= "" and network.ssid or "<hidden>"), 8, 58, 1)
    draw_text("RSSI: " .. network.rssi .. " dBm", 8, 72, 1)
    draw_text("Channel: " .. network.channel, 8, 86, 1)
    draw_text("BSSID: " .. network.bssid, 8, 100, 1)
    draw_text("Auth: " .. network.encryption, 8, 114, 1)
    draw_text("Enter/back to return", 8, 128, 1)
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
            show_network(result)
        end
    else
        list_draw()
    end
end

function update()
    if mode ~= "scan" then return end
    local status = wifi_scan_status()
    if status ~= last_status then
        last_status = status
        if status == "complete" then
            networks = {}
            for i = 0, wifi_scan_count() - 1 do
                table.insert(networks, wifi_scan_get(i))
            end
            show_results()
        elseif status == "idle" or status == "failed" then
            show_header("Scan unavailable")
            list_clear()
            list_add_item("Rescan")
            list_draw()
        else
            show_header("Scanning...")
        end
    end
end
