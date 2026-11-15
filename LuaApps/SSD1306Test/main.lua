-- SSD1306 (128x64, I2C) test using only raw i2c.write: no display library.
--
-- Wiring (Grove port pins): SDA = GPIO2 (G2), SCL = GPIO1 (G1), GND.
-- Power the OLED from 3.3V, NOT the 5V Grove pin: most modules pull SDA/SCL up to VCC,
-- which would put 5V onto the ESP32-S3 pins.
--
-- The OLED shows a border, uptime, the clock (if the system clock is set, e.g. after
-- running NTP Test) and a bar that fills over a minute. Press R on the Cardputer to retry.

local WHITE, GREEN, RED, GREY = 0xFFFF, 0x07E0, 0xF800, 0x7BEF

local ADDR = nil
local ready = false
local last_sec = -1

-- ---------------------------------------------------------------- font (5x7)
-- one entry per character, 5 columns, bit 0 = top row
local font = {
    [" "] = {0x00,0x00,0x00,0x00,0x00}, ["!"] = {0x00,0x00,0x5F,0x00,0x00},
    ["-"] = {0x08,0x08,0x08,0x08,0x08}, ["."] = {0x00,0x60,0x60,0x00,0x00},
    [":"] = {0x00,0x36,0x36,0x00,0x00}, ["?"] = {0x02,0x01,0x51,0x09,0x06},
    ["0"] = {0x3E,0x51,0x49,0x45,0x3E}, ["1"] = {0x00,0x42,0x7F,0x40,0x00},
    ["2"] = {0x42,0x61,0x51,0x49,0x46}, ["3"] = {0x21,0x41,0x45,0x4B,0x31},
    ["4"] = {0x18,0x14,0x12,0x7F,0x10}, ["5"] = {0x27,0x45,0x45,0x45,0x39},
    ["6"] = {0x3C,0x4A,0x49,0x49,0x30}, ["7"] = {0x01,0x71,0x09,0x05,0x03},
    ["8"] = {0x36,0x49,0x49,0x49,0x36}, ["9"] = {0x06,0x49,0x49,0x29,0x1E},
    ["A"] = {0x7E,0x11,0x11,0x11,0x7E}, ["B"] = {0x7F,0x49,0x49,0x49,0x36},
    ["C"] = {0x3E,0x41,0x41,0x41,0x22}, ["D"] = {0x7F,0x41,0x41,0x22,0x1C},
    ["E"] = {0x7F,0x49,0x49,0x49,0x41}, ["F"] = {0x7F,0x09,0x09,0x09,0x01},
    ["G"] = {0x3E,0x41,0x49,0x49,0x7A}, ["H"] = {0x7F,0x08,0x08,0x08,0x7F},
    ["I"] = {0x00,0x41,0x7F,0x41,0x00}, ["J"] = {0x20,0x40,0x41,0x3F,0x01},
    ["K"] = {0x7F,0x08,0x14,0x22,0x41}, ["L"] = {0x7F,0x40,0x40,0x40,0x40},
    ["M"] = {0x7F,0x02,0x0C,0x02,0x7F}, ["N"] = {0x7F,0x04,0x08,0x10,0x7F},
    ["O"] = {0x3E,0x41,0x41,0x41,0x3E}, ["P"] = {0x7F,0x09,0x09,0x09,0x06},
    ["Q"] = {0x3E,0x41,0x51,0x21,0x5E}, ["R"] = {0x7F,0x09,0x19,0x29,0x46},
    ["S"] = {0x46,0x49,0x49,0x49,0x31}, ["T"] = {0x01,0x01,0x7F,0x01,0x01},
    ["U"] = {0x3F,0x40,0x40,0x40,0x3F}, ["V"] = {0x1F,0x20,0x40,0x20,0x1F},
    ["W"] = {0x3F,0x40,0x38,0x40,0x3F}, ["X"] = {0x63,0x14,0x08,0x14,0x63},
    ["Y"] = {0x07,0x08,0x70,0x08,0x07}, ["Z"] = {0x61,0x51,0x49,0x45,0x43},
}

-- ---------------------------------------------------------------- framebuffer
-- 128x64 px, 8 pages of 128 bytes. byte = column slice of 8 vertical pixels.
local fb = {}
for i = 1, 1024 do fb[i] = 0 end

local function clear()
    for i = 1, 1024 do fb[i] = 0 end
end

local function px(x, y)
    if x < 0 or x > 127 or y < 0 or y > 63 then return end
    local i = (y >> 3) * 128 + x + 1
    fb[i] = fb[i] | (1 << (y & 7))
end

local function fillrect(x, y, w, h)
    for yy = y, y + h - 1 do
        for xx = x, x + w - 1 do px(xx, yy) end
    end
end

local function text(str, x, y, s)
    s = s or 1
    str = string.upper(str)
    for n = 1, #str do
        local glyph = font[str:sub(n, n)] or font["?"]
        for col = 1, 5 do
            local bits = glyph[col]
            for row = 0, 6 do
                if (bits & (1 << row)) ~= 0 then
                    fillrect(x + (col - 1) * s, y + row * s, s, s)
                end
            end
        end
        x = x + 6 * s
    end
end

-- ---------------------------------------------------------------- SSD1306 raw I2C
local function cmd(...)
    return i2c.write(ADDR, {0x00, ...})        -- control byte 0x00 = what follows are commands
end

local function init_display()
    return cmd(0xAE,          -- display off
               0xD5, 0x80,    -- clock divide
               0xA8, 0x3F,    -- 64 rows
               0xD3, 0x00,    -- no display offset
               0x40,          -- start line 0
               0x8D, 0x14,    -- charge pump on
               0x20, 0x00,    -- horizontal addressing mode
               0xA1, 0xC8,    -- flip x/y (use 0xA0, 0xC0 if the image is upside down)
               0xDA, 0x12,    -- COM pins
               0x81, 0xCF,    -- contrast
               0xD9, 0xF1,    -- pre-charge
               0xDB, 0x40,    -- VCOM detect
               0xA4,          -- follow RAM
               0xA6,          -- normal (not inverted)
               0xAF)          -- display on
end

local page = {}
local function flush()
    if not cmd(0x21, 0, 127, 0x22, 0, 7) then return false end   -- column + page range
    for p = 0, 7 do
        page[1] = 0x40                                            -- control byte 0x40 = data
        for x = 1, 128 do page[x + 1] = fb[p * 128 + x] end
        if not i2c.write(ADDR, page) then return false end
    end
    return true
end

-- ---------------------------------------------------------------- Cardputer screen
local y = 24
local function say(str, color)
    gfx.drawText(str, 4, y, 1, color or WHITE)
    y = y + 12
end

local function fresh_screen(title)
    gfx.clearScreen()
    y = 24
    say(title, WHITE)
end

-- ---------------------------------------------------------------- app
local function setup()
    ready = false
    ADDR = nil
    last_sec = -1
    fresh_screen("SSD1306 Test")

    if not i2c.begin(400000) then
        say("i2c.begin failed", RED)
        return
    end

    local found = i2c.scan()
    local names = {}
    for _, a in ipairs(found) do
        names[#names + 1] = string.format("0x%02X", a)
        if a == 0x3C or a == 0x3D then ADDR = a end
    end
    say("scan: " .. (#names > 0 and table.concat(names, " ") or "nothing"), GREY)

    if not ADDR then
        say("no OLED at 0x3C / 0x3D", RED)
        say("SDA=G2(GPIO2) SCL=G1(GPIO1)", GREY)
        say("try swapping SDA and SCL", GREY)
        say("R = retry", GREY)
        i2c.close()                -- hands GPIO1 back to the IR receiver
        return
    end

    if not init_display() then
        say("init failed", RED)
        return
    end

    say(string.format("OLED found at 0x%02X", ADDR), GREEN)
    say("drawing... R = restart", GREY)
    ready = true
end

local function draw_frame(up)
    clear()
    fillrect(0, 0, 128, 1)  fillrect(0, 63, 128, 1)     -- border
    fillrect(0, 0, 1, 64)   fillrect(127, 0, 1, 64)

    text("SINGULARITY", 4, 4, 1)
    text("UP " .. up .. "S", 4, 16, 2)

    local t = sys.time()
    if t then
        text(string.format("%02d:%02d:%02d", t.hour, t.min, t.sec), 4, 36, 1)
    else
        text("NO CLOCK", 4, 36, 1)
    end

    text(string.format("ADDR 0X%02X", ADDR), 4, 46, 1)
    fillrect(4, 57, (up % 60) * 2, 3)                   -- fills over one minute
end

function on_key(key)
    if key == 114 or key == 82 then setup() end         -- r / R
end

function update()
    if not ready then return end
    local sec = sys.millis() // 1000
    if sec == last_sec then return end
    last_sec = sec

    draw_frame(sec)
    if not flush() then
        ready = false
        fresh_screen("SSD1306 Test")
        say("write failed", RED)
        say("R = retry", GREY)
    end
end

setup()
