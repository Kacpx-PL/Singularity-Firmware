# Apps

See individual README files under each app for instructions and/or documentation.

For file structure see `sd_files`.

# Lua API Reference

Every app has access to these global namespaces (`gfx`, `wifi`, `ble`, `espnow`, `http`, `ir`, `i2c`, `sys`, `gpio`, `cnfig`). No `require`/`import` needed — they are all registered directly into the Lua environment at startup.

## App Lifecycle

Every app can define these two optional functions:

```lua
function on_key(key)
    -- called when a key is pressed while this app is active
    -- the key is an integer (standard ASCII code in decimal)
end

function update()
    -- called once per frame while this app is active
end
```

Any top-level code outside these functions runs once, immediately, when the app is opened.

### Display (gfx)

```lua
gfx.drawText("Hello", 10, 40, 2, 0xFFFF)     -- text, x, y, size (default 2), color (optional, default white)
gfx.clearScreen()                             -- clears the app's drawing area (leaves the status bar intact)
gfx.setColor(0x07E0)                          -- set the draw color (RGB565 !!NOT HEX!!) used by shape functions below
gfx.drawRect(x, y, w, h)
gfx.drawRectF(x, y, w, h)                    -- full/filled rectangle
gfx.drawLine(x0, y0, x1, y1)
gfx.drawTri(x0, y0, x1, y1, x2, y2)
gfx.drawTriF(x0, y0, x1, y1, x2, y2)         -- full/filled triangle
gfx.drawCircle(x, y, radius)
gfx.drawCircleF(x, y, radius)                -- full/filled circle
gfx.drawStatusBar()                          -- force a status bar redraw
```

### Input & System

```
sys.keyPressed(key)                          -- checks for a pressed key (use an ASCII to decimal converter)
sys.millis()                                 -- returns ms since boot
sys.serialPrint("debug message")              -- prints to the USB serial console
local imuData = sys.getImu()                  -- gets current IMU sensor reading
sys.appDir()                                  -- this app's own folder, e.g. "/singularity/apps/weather" (nil for built-in apps)
```
Keys register as **numbers**, not characters compare against the numeric code, e.g. `if key == 8 then` for backspace, not `if key == '\b' then`.

### Time (NTP)

The Lua environment has no `os` library, so use these instead.

```lua
sys.ntpStart("CET-1CEST,M3.5.0,M10.5.0/3")   -- optional POSIX timezone string (default "UTC0"), optional NTP server
sys.ntpSynced()                              -- true once the NTP server has answered
local t = sys.time()                         -- local time table, or nil if the clock was never set
local e = sys.epoch()                        -- seconds since 1970 (UTC), or nil if the clock was never set
```

- `sys.ntpStart` returns immediately and syncs in the background. Wi-Fi must be connected; if it isn't yet, it keeps retrying. Poll `sys.ntpSynced()` from `update()`.
- `sys.time()` returns `year, month, day, hour, min, sec, wday, yday`. `wday` is 1 for Sunday.
- The timezone is a POSIX TZ string, and the sign is inverted from what you'd expect: `"UTC0"` is UTC, `"CET-1CEST,M3.5.0,M10.5.0/3"` is Central Europe with daylight saving, `"EST5EDT,M3.2.0,M11.1.0"` is US Eastern. It stays set after the app closes.
- `sys.time()` / `sys.epoch()` return nil until the clock holds a date from 2024 or later, so they don't need `ntpSynced()` to be true first.

```lua
sys.ntpStart("CET-1CEST,M3.5.0,M10.5.0/3")

function update()
    local t = sys.time()
    if t then
        gfx.drawText(string.format("%02d:%02d:%02d", t.hour, t.min, t.sec), 10, 40)
    end
end
```

### Storage (SD card)

```lua
local content = storage_read("/singularity/system/config.lua")  -- returns nil if file doesn't exist
storage_write("/path/to/file.txt", "Hello World!")
storage_exists("/path/to/file")
```

Every app has its own folder, and `sys.appDir()` tells you where it is. Where an app lives depends on how the user organised their SD card (apps can sit inside folders), so never hardcode the path; build it from `sys.appDir()`. Writing to a path whose folders don't exist yet creates them.

```lua
local dir = sys.appDir()
local saved = storage_read(dir .. "/city.txt")     -- nil on the first launch
storage_write(dir .. "/city.txt", "Berlin")
storage_write(dir .. "/saves/slot1.txt", "...")    -- creates /saves automatically
```

### WiFi

```lua
wifi.connect(ssid, password)     -- blocking (hangs system, deprecated), returns true/false
wifi.connected()
wifi.getIP()
wifi.getSSID()                  -- from currently connected network
wifi.getRssi()
wifi.disconnect()

wifi.addNetwork(ssid, password) -- saves to the known-networks list
wifi.remNetwork(index)
wifi.getNetworkCount()
wifi.getNetworkSSID(index)

wifi.setAutoconnect(true)       -- true/false
wifi.getAutoconnect()
```

Wi-Fi discovery is asynchronous. Result indexes are zero-based, matching the existing network-management API.

```lua
wifi.scanStart(false)           -- optional argument includes hidden networks

function update()
    if wifi.scanStatus() == "complete" then
        for i = 0, wifi.scanCount() - 1 do
            local network = wifi.scanGet(i)
            sys.serialPrint(network.ssid .. " " .. network.rssi .. " dBm " .. network.bssid)
        end
    end
end
```

Each Wi-Fi result contains `ssid`, `bssid`, `rssi`, `channel`, and numeric `encryption` fields.

### ESP-NOW

ESP-NOW is initialized at boot. Add a peer before sending; MAC addresses use `AA:BB:CC:DD:EE:FF` format. `receive()` returns the latest received `{mac, data}` packet, or `nil`.

```lua
espnow.addPeer("AA:BB:CC:DD:EE:FF")
espnow.send("AA:BB:CC:DD:EE:FF", "hello")

local packet = espnow.receive()
if packet then
    sys.serialPrint(packet.mac .. ": " .. packet.data)
end
```

### Http/Json

Note : http fetching is blocking it will block all draw calls untill the fetch succeeds/fails\
before calling its worth adding a screen eg "Loading..."

```
http.fetch(url)
http.jsonParse(jsonstring)
```
``http.fetch`` returns 2 values result, err                 -- result, error
``http.jsonParse `` also returns 2 values lua_table, err    -- result, error

Example usage :
```
local res, err = http.fetch("https://api.open-meteo.com/v1/forecast?latitude=52.52&longitude=13.41&current=temperature_2m") -- berlin

if err then
    sys.serialPrint("httperr")
    return
end

local jres, jerr = http.jsonParse(res)

if jerr then
    sys.serialPrint("jsonerr")
    return
end

if not jres or not jres.current then
    sys.serialPrint("apierr")
    return
end

gfx.clearScreen()
gfx.drawText(tostring(jres.current.temperature_2m), 10, 30)
```

### BLE scanning

BLE scanning is also asynchronous and uses the framework's bundled ESP32 BLE library. The optional duration is in seconds and active scanning is enabled by default.

```lua
ble.scanStart(5, true)

function update()
    if ble.scanStatus() == "complete" then
        for i = 0, ble.scanCount() - 1 do
            local device = ble.scanGet(i)
            sys.serialPrint(device.address .. " " .. device.rssi .. " dBm " .. device.name)
            for _, uuid in ipairs(device.service_uuids) do
                sys.serialPrint("service " .. uuid)
            end
        end
    end
end
```

BLE results contain `address`, `address_type`, `rssi`, `name`, `service_uuids`, `service_data_uuids`, `service_data`, `manufacturer_data`, and `payload`. `appearance` and `tx_power` are present when advertised. Binary values are Lua arrays of byte integers from 0 to 255.

Use the deep advertisement parser to inspect every complete AD structure, including unknown types:

```lua
local fields = ble.parseAdvert(device.payload)
for _, field in ipairs(fields) do
    sys.serialPrint("AD type=" .. field.type .. " length=" .. field.length)
    if field.name then sys.serialPrint(field.name) end
    if field.uuid then sys.serialPrint(field.uuid) end
end
```

Parser fields always include `type`, `length`, and raw `data`. Known structures may additionally include `name`, `uuid`, `flags`, `tx_power`, or `company_id`. The parser never connects to a device or performs GATT discovery.

### IR

Uses [Arduino IRremote](https://github.com/Arduino-IRremote/Arduino-IRremote) library

`ir.sendProtocol` returns `true` if the signal was sent and `false` if the protocol isn't supported. Protocol names are case-insensitive and ignore separators, and both IRremote names (what `ir.receiveGetProtocol()` returns) and Flipper `.ir` names work (e.g. `NEC`, `NECext`, `Samsung32`, `SIRC`, `SIRC15`, `SIRC20`, `RC5X`). `ir.sendRaw` takes the carrier frequency in kHz (38) or Hz (38000).

```lua
-- Sending 
ir.sendProtocol("NEC", address, command)   -- works for NEC/NECext/Onkyo/Apple, Denon/Sharp,
                                           -- Panasonic/Kaseikyo, JVC, LG, RC5, RC6, Samsung,
                                           -- Sony, Marantz, BoseWave, Lego, FAST, Whynter, MagiQuest

ir.sendRaw({2762, 793, 534, ...}, 38)      -- raw timing array (µs), carrier frequency in kHz

-- Receiving
if ir.receiveAvbl() then
    local protocol = ir.receiveGetProtocol()
    local address  = ir.receiveGetAddress()
    local command  = ir.receiveGetCmd()
    ir.receiveResume()  -- must call this after reading a result to listen for the next signal
end
```

### GPIO

```lua
gpio.write(pin, value)
gpio.read(pin)
```
Reserved pins (SD card, IR LED/receiver, display, USB, audio, keyboard interrupt, flash and the EXT header) and pins that don't exist on the ESP32-S3 will raise a Lua error if accessed — see `src/core/pins.h` for the full list.

### I2C

Crude I2C master on the Grove port: **SDA = GPIO 2, SCL = GPIO 1**. You send raw bytes and read raw bytes back; the app has to know what a given module expects. The internal bus (keyboard, IMU) is separate and not reachable from here.

```lua
if not i2c.begin(100000) then       -- optional clock in Hz, default 100000 (clamped 10 kHz..1 MHz)
    sys.serialPrint("i2c failed")
end

local found = i2c.scan()            -- array of addresses that answered, e.g. {0x44}

i2c.write(0x44, {0x24, 0x00})       -- true if the module ACKed. Takes a table of bytes or a string
local data = i2c.read(0x44, 6)      -- table of 6 bytes, or nil if the module didn't answer

-- most sensors want "write a register number, then read" with a repeated start in between:
local reg = i2c.writeRead(0x68, {0x75}, 1)

i2c.close()                         -- optional, it also happens automatically when the app closes
```

- Addresses are 7-bit and must be `0x08`–`0x77`. Up to 255 bytes per call.
- I2C modules never push data on their own, so "listening" means polling: call `i2c.read` from `update()`.
- GPIO 1 is also the **IR receiver pin**. While I2C is open, `ir.receive*` doesn't work; `i2c.close()` (or closing the app) turns it back on.
- While I2C is open, `gpio.read/write` on pins 1 and 2 raise an error.
- Calling `i2c.write/read/writeRead/scan` before `i2c.begin()` raises an error.

### UI widgets

**Scrolling list:**
```lua
gfx.listClr()
gfx.listAddItem("Option 1")
gfx.listAddItem("Option 2")
gfx.listDraw()

function on_key(key)
    local selected = gfx.listHandleKey(key)  -- returns the selected index on Enter, -1 otherwise
    gfx.listDraw()
    if selected == 0 then
        -- "Option 1" was chosen
    end
end
```

**Text input:**
```lua
gfx.textInputStart("Enter SSID:", false)  -- prompt, mask (true for passwords)
gfx.textInputDraw()

function on_key(key)
    local result = gfx.textInputHandleKey(key)
    gfx.textInputDraw()
    if result == 1 then
        local value = gfx.textInputGetVal()
    elseif result == 2 then
        -- backspace pressed on an empty field — treat as "cancelled"
    end
end
```

**File browser:**
```lua
gfx.fileBrowserStart("/singularity/system/ir_db", false)  -- root path, folder-select-only mode
gfx.fileBrowserDraw()

function on_key(key)
    local result = gfx.fileBrowserHandleKey(key)
    gfx.fileBrowserDraw()
    if result == 1 then
        local path = gfx.fileBrowserGetSelectedPath()
    end
end
```
Navigation is constrained to the given root path — `..` appears when inside a subfolder, and you can never browse above the root.

### Config

```lua
cnfig.setThemeColor(0x07E0)   -- applies immediately and persists to config.lua
cnfig.setBootDelay(3000)      -- clamped to 1000–10000ms internally
```

### Misc

```lua
millis()                        -- returns ms since boot delay example : if millis() - last_trigger(registered with millis) < interval then
serial_print("debug message")   -- prints to the USB serial console
```

### Extra info that doesnt fit any category

- DO NOT USE while True do it will break the firmware and require a restart
- After a app closes the whole state gets wiped therefore to carry any information between app lanuches you need to store them in a file
- The top status bar reserves 20px at the top of the screen
- If something is missing check lua_bindings.cpp (some bindings are redundant)
- Max script file size is 32KB while 160KB is mostly free for modules/values
- minimize redrawing the elements, only draw the delta between frames if possible
- **Resource limits:** each app may use about 96KB of Lua memory. Exceeding it (or dropping the device's free heap below ~16KB) kills the app with an "Out of memory" screen and frees everything
- **Watchdog:** a single call (app start, `update()`, `on_key()`) that runs longer than 3 seconds is killed, so `while true do` no longer hangs the device. Time spent inside `http.fetch` / `wifi.connect` does not count. Press ESC after a kill to return to the menu
- An uncaught error in the app's top-level code or in `update()` also stops the app and shows the error until you press ESC