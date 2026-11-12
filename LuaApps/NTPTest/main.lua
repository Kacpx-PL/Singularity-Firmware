-- NTP test: shows Wi-Fi/NTP status, the local time, date and raw epoch.
-- Needs a saved Wi-Fi network (use the WiFi app first). Press S to restart the sync.

local TZ = "CET-1CEST,M3.5.0,M10.5.0/3"   -- Poland. UTC would be "UTC0"

local GREEN, RED, YELLOW, WHITE = 0x07E0, 0xF800, 0xFFE0, 0xFFFF

local started = false
local start_ms = 0
local synced_after = nil
local last = {}          -- last text drawn per slot, so we only redraw what changed

local function pad(text, width)
    return string.format("%-" .. width .. "s", text)
end

local function line(slot, text, x, y, size, color)
    local key = text .. "|" .. color
    if last[slot] == key then return end
    last[slot] = key
    gfx.drawText(text, x, y, size, color)
end

local function start_ntp()
    sys.ntpStart(TZ)
    started = true
    start_ms = sys.millis()
    synced_after = nil
end

gfx.clearScreen()
gfx.drawText(pad("TZ: " .. TZ, 40), 4, 118, 1, 0x7BEF)

function on_key(key)
    if key == 115 or key == 83 then   -- s / S
        start_ntp()
    end
end

function update()
    -- Wi-Fi
    local online = wifi.connected()
    if online then
        line("wifi", pad("WiFi: " .. wifi.getSSID(), 40), 4, 24, 1, GREEN)
    else
        line("wifi", pad("WiFi: not connected", 40), 4, 24, 1, RED)
    end

    -- start syncing as soon as we're online
    if online and not started then start_ntp() end

    -- NTP status
    if not started then
        line("ntp", pad("NTP: waiting for WiFi", 40), 4, 36, 1, YELLOW)
    elseif sys.ntpSynced() then
        if not synced_after then synced_after = sys.millis() - start_ms end
        line("ntp", pad(string.format("NTP: synced (%.1fs)", synced_after / 1000), 40), 4, 36, 1, GREEN)
    else
        line("ntp", pad("NTP: syncing...", 40), 4, 36, 1, YELLOW)
    end

    -- Time / date / epoch
    local t = sys.time()
    if t then
        line("time", string.format("%02d:%02d:%02d", t.hour, t.min, t.sec), 4, 52, 3, WHITE)
        line("date", string.format("%04d-%02d-%02d", t.year, t.month, t.day), 4, 82, 2, WHITE)
        line("epoch", pad("epoch: " .. tostring(sys.epoch()), 40), 4, 104, 1, 0x7BEF)
    else
        line("time", "--:--:--", 4, 52, 3, 0x7BEF)
        line("date", "----------", 4, 82, 2, 0x7BEF)
        line("epoch", pad("epoch: nil (clock not set)", 40), 4, 104, 1, 0x7BEF)
    end
end
