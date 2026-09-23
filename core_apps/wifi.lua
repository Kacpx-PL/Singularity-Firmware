-- Ensure default wifi config file exists
local WIFI_CONFIG_PATH = "/singularity/system/wifi_networks.lua"
if not storage_exists(WIFI_CONFIG_PATH) then
    local default_wifi_config = "return {\n    autoconnect = false,\n    networks = {},\n}\n"
    storage_write(WIFI_CONFIG_PATH, default_wifi_config)
end

local mode = "list"
local pending_ssid = ""

function refresh_list()
    gfx.listClr()
    gfx.listAddItem("Auto Connect: " .. (wifi.getAutoconnect() and "ON" or "OFF"))
    gfx.listAddItem("Add Network")
    gfx.listAddItem("Status")
    gfx.listAddItem("Disconnect")
    gfx.listAddItem("Manage Networks")
end

gfx.clearScreen()
gfx.drawText("WiFi Menu", 10, 40, 2)
refresh_list()
gfx.listDraw()

function on_key(key)
    if mode == "list" then
        local result = gfx.listHandleKey(key)
        --draw_rectF(0,20,240,47)
        gfx.clearScreen()
        gfx.drawText("WiFi Menu", 10, 40, 2)
        gfx.listDraw()

        if result == 0 then
            wifi.setAutoconnect(not wifi.getAutoconnect())
            refresh_list()
            gfx.listDraw()
        elseif result == 1 then
            gfx.clearScreen()
            mode = "input_ssid"
            gfx.textInputStart("SSID:", false)
            gfx.textInputDraw()
        elseif result == 2 then
            if wifi.connected() then
                gfx.clearScreen()
                gfx.drawText("Connected", 10, 30, 2)
                gfx.drawText("IP: " .. wifi.getIP(), 10, 50, 2)
                gfx.drawText("SSID: ".. wifi.getSSID(), 10, 70, 1)
            else
                gfx.clearScreen()
                gfx.drawText("Not connected", 10, 30)
            end
        elseif result == 3 then
            gfx.clearScreen()
            wifi.disconnect()
            gfx.drawText("Disconnected", 10, 30)
        elseif result == 4 then -- new "Manage Networks" entry
            mode = "manage"
            gfx.listClr()
            local count = wifi.getNetworkCount()
            for i = 0, count - 1 do
                gfx.listAddItem(wifi.getNetworkSSID(i))
            end
            gfx.listAddItem("< Back")
            gfx.clearScreen()
            gfx.drawText("Select to remove:", 10, 30, 1)
            gfx.listDraw()
        end

    elseif mode == "input_ssid" then
        local r = gfx.textInputHandleKey(key)
        if r == 2 then
            mode = "list"
            gfx.clearScreen()
            gfx.listDraw()
            gfx.drawText("WiFi Menu", 10, 40, 2)
        else
            gfx.textInputDraw()
            if r == 1 then
                gfx.clearScreen()
                pending_ssid = gfx.textInputGetVal()
                mode = "input_pass"
                gfx.textInputStart("Password:", false)
                gfx.textInputDraw()
            end
        end

    elseif mode == "input_pass" then
        local r = gfx.textInputHandleKey(key)
        if r == 2 then
            mode = "list"
            gfx.clearScreen()
            gfx.drawText("WiFi Menu", 10, 40, 2)
            gfx.listDraw()
        else
            gfx.textInputDraw()
            if r == 1 then
                wifi.addNetwork(pending_ssid, gfx.textInputGetVal())
                mode = "list"
                refresh_list()
                gfx.clearScreen()
                gfx.drawText("WiFi Menu", 10, 40, 2)
                gfx.listDraw()
            end
        end

    elseif mode == "manage" then
        local result = gfx.listHandleKey(key)
        gfx.clearScreen()
        gfx.drawText("Select to remove:", 10, 30, 2)
        gfx.listDraw()

        if result ~= nil and result >= 0 then
            local count = wifi.getNetworkCount()
            if result == count then
                mode = "list"
                refresh_list()
                gfx.clearScreen()
                gfx.drawText("WiFi Menu", 10, 40, 2)
                gfx.listDraw()
            else
                wifi.remNetwork(result)
                mode = "list"
                refresh_list()
                gfx.clearScreen()
                gfx.drawText("WiFi Menu", 10, 40, 2)
                gfx.listDraw()
            end
        end
        
    end
end

function update()
end