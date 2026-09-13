local mode = "list"
local pending_ssid = ""

function refresh_list()
    list_clear()
    list_add_item("Auto Connect: " .. (wifi_get_autoconnect() and "ON" or "OFF"))
    list_add_item("Add Network")
    list_add_item("Status")
    list_add_item("Disconnect")
    list_add_item("Manage Networks")
end

clear_screen()
draw_text("WiFi Menu", 10, 40, 2)
refresh_list()
list_draw()

function on_key(key)
    if mode == "list" then
        local result = list_handle_key(key)
        draw_rectF(0,20,240,47)
        draw_text("WiFi Menu", 10, 40, 2)
        list_draw()

        if result == 0 then
            wifi_set_autoconnect(not wifi_get_autoconnect())
            refresh_list()
            list_draw()
        elseif result == 1 then
            clear_screen()
            mode = "input_ssid"
            text_input_start("SSID:", false)
            text_input_draw()
        elseif result == 2 then
            if wifi_is_connected() then
                clear_screen()
                draw_text("Connected", 10, 30, 2)
                draw_text("IP: " .. wifi_get_ip(), 10, 50, 2)
                draw_text("SSID: ".. wifi_get_ssid(), 10, 70, 1)
            else
                clear_screen()
                draw_text("Not connected", 10, 30)
            end
        elseif result == 3 then
            clear_screen()
            wifi_disconnect()
            draw_text("Disconnected", 10, 30)
        elseif result == 4 then -- new "Manage Networks" entry
            mode = "manage"
            list_clear()
            local count = wifi_get_network_count()
            for i = 0, count - 1 do
                list_add_item(wifi_get_network_ssid(i))
            end
            list_add_item("< Back")
            clear_screen()
            draw_text("Select to remove:", 10, 30, 1)
            list_draw()
        end

    elseif mode == "input_ssid" then
        local r = text_input_handle_key(key)
        if r == 2 then
            mode = "list"
            clear_screen()
            list_draw()
            draw_text("WiFi Menu", 10, 40, 2)
        else
            text_input_draw()
            if r == 1 then
                clear_screen()
                pending_ssid = text_input_get_value()
                mode = "input_pass"
                text_input_start("Password:", false)
                text_input_draw()
            end
        end

    elseif mode == "input_pass" then
        local r = text_input_handle_key(key)
        if r == 2 then
            mode = "list"
            clear_screen()
            draw_text("WiFi Menu", 10, 40, 2)
            list_draw()
        else
            text_input_draw()
            if r == 1 then
                wifi_add_network(pending_ssid, text_input_get_value())
                mode = "list"
                refresh_list()
                clear_screen()
                draw_text("WiFi Menu", 10, 40, 2)
                list_draw()
            end
        end

    elseif mode == "manage" then
        local result = list_handle_key(key)
        clear_screen()
        draw_text("Select to remove:", 10, 30, 2)
        list_draw()

        if result ~= nil and result >= 0 then
            local count = wifi_get_network_count()
            if result == count then
                mode = "list"
                refresh_list()
                clear_screen()
                draw_text("WiFi Menu", 10, 40, 2)
                list_draw()
            else
                wifi_remove_network(result)
                mode = "list"
                refresh_list()
                clear_screen()
                draw_text("WiFi Menu", 10, 40, 2)
                list_draw()
            end
        end
        
    end
end

function update()
end