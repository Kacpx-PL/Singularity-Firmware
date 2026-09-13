# Apps

See indvidual README files under each app for instructions and/or documentation

# Lua API Reference

Every app has access to these global functions. No `require`/`import` needed — they're all registered directly into the Lua environment at startup.

## App lifecycle

Every app can define these two optional functions:

```lua
function on_key(key)
    -- called when a key is pressed while this app is active
end

function update()
    -- called once per frame while this app is active
end
```

Any top-level code outside these functions runs once, immediately, when the app is opened.

### Display

```lua
draw_text("Hello", 10, 40, 2, 0xFFFF)          -- text, x, y, size (default 2), color (optional, default white)
clear_screen()                                 -- clears the app's drawing area (leaves the status bar intact)
set_color(0x07E0)                              -- set the draw color (RGB565) used by shape functions below
draw_rect(x, y, w, h)
draw_rect_full(x, y, w, h)
draw_line(x0, y0, x1, y1)
draw_triangle(x0, y0, x1, y1, x2, y2)          -- outline
draw_triangle_full(x0, y0, x1, y1, x2, y2)     -- filled
draw_circle(x, y, radius)                      -- outline
draw_circle_full(x, y, radius)                 -- filled
draw_status_bar()                              -- force a status bar redraw
```

### Input

Keys register as **numbers**, not characters — compare against the numeric code, e.g. `if key == 8 then` for backspace, not `if key == '\b' then`.

### Storage (SD card)

```lua
local content = storage_read("/singularity/system/config.lua")  -- returns nil if file doesn't exist
storage_write("/path/to/file.txt", "Hello World!")
storage_exists("/path/to/file")
```

### WiFi

```lua
wifi_connect(ssid, password)     -- blocking (hangs system, deprecated), returns true/false
wifi_is_connected()
wifi_get_ip()
wifi_get_ssid()                  -- from currently connected network
wifi_get_rssi()
wifi_disconnect()

wifi_add_network(ssid, password) -- saves to the known-networks list
wifi_remove_network(index)
wifi_get_network_count()
wifi_get_network_ssid(index)

wifi_set_autoconnect(true)      -- true/false
wifi_get_autoconnect()
```

### IR

Uses [Arduino IRremote](https://github.com/Arduino-IRremote/Arduino-IRremote) library

```lua
-- Sending 
ir_send_protocol("NEC", address, command)   -- works for NEC/NECext/Onkyo/Apple, Denon/Sharp,
                                            -- Panasonic/Kaseikyo, JVC, LG, RC5, RC6, Samsung,
                                            -- Sony, Marantz, BoseWave, Lego, FAST, Whynter, MagiQuest

ir_send_raw({2762, 793, 534, ...}, 38)      -- raw timing array (µs), carrier frequency in kHz

-- Receiving
if ir_receive_available() then
    local protocol = ir_receive_get_protocol()
    local address  = ir_receive_get_address()
    local command  = ir_receive_get_command()
    ir_receive_resume()  -- must call this after reading a result to listen for the next signal
end
```

### GPIO

```lua
gpio_write(pin, value)
gpio_read(pin)
```
Reserved pins (SD card, IR LED, EXT/cap header) will raise a Lua error if accessed — see `src/core/pins.h` for the full list.

### UI widgets

**Scrolling list:**
```lua
list_clear()
list_add_item("Option 1")
list_add_item("Option 2")
list_draw()

function on_key(key)
    local selected = list_handle_key(key)  -- returns the selected index on Enter, -1 otherwise
    list_draw()
    if selected == 0 then
        -- "Option 1" was chosen
    end
end
```

**Text input:**
```lua
text_input_start("Enter SSID:", false)  -- prompt, mask (true for passwords)
text_input_draw()

function on_key(key)
    local result = text_input_handle_key(key)
    text_input_draw()
    if result == 1 then
        local value = text_input_get_value()
    elseif result == 2 then
        -- backspace pressed on an empty field — treat as "cancelled"
    end
end
```

**File browser:**
```lua
file_browser_start("/singularity/system/ir_db", false)  -- root path, folder-select-only mode
file_browser_draw()

function on_key(key)
    local result = file_browser_handle_key(key)
    file_browser_draw()
    if result == 1 then
        local path = file_browser_get_selected_path()
        local is_folder = file_browser_selected_is_dir()
    end
end
```
Navigation is constrained to the given root path — `..` appears when inside a subfolder, and you can never browse above the root.

### Config

```lua
config_set_theme_color(0x07E0)   -- applies immediately and persists to config.lua
config_set_boot_delay(3000)      -- clamped to 1000–10000ms internally
```

### Misc

```lua
serial_print("debug message")   -- prints to the USB serial console
```