-- File test: writes files into this app's own folder (sys.appDir()) and reads them back.
-- Every launch bumps a counter, so launching it twice proves the data survives app restarts.
-- Look on the SD card in <app folder>/hello.txt, count.txt and sub/nested.txt.

local GREEN, RED, WHITE, GREY = 0x07E0, 0xF800, 0xFFFF, 0x7BEF

local y = 24
local function say(text, color)
    gfx.drawText(text, 4, y, 1, color or WHITE)
    y = y + 12
end

gfx.clearScreen()

local dir = sys.appDir()
if not dir then
    say("sys.appDir() returned nil", RED)
    say("(built-in apps have no folder)", GREY)
    return
end

say("appDir:", GREY)
say(dir, WHITE)

-- launch counter (nil on the very first launch)
local launches = tonumber(storage_read(dir .. "/count.txt") or "0") or 0
launches = launches + 1
local ok_count = storage_write(dir .. "/count.txt", tostring(launches))

-- the hello world file
local ok_hello = storage_write(dir .. "/hello.txt", "Hello World!\nlaunch #" .. launches)

-- a path whose folder doesn't exist yet: should be created automatically
local ok_nested = storage_write(dir .. "/sub/nested.txt", "nested ok")

say("")
say("count.txt:       " .. (ok_count and "written" or "FAILED"), ok_count and GREEN or RED)
say("hello.txt:       " .. (ok_hello and "written" or "FAILED"), ok_hello and GREEN or RED)
say("sub/nested.txt:  " .. (ok_nested and "written" or "FAILED"), ok_nested and GREEN or RED)

local back = storage_read(dir .. "/hello.txt")
say("")
if back then
    say("read back: " .. back:gsub("\n", " | "), GREEN)
else
    say("read back: nil", RED)
end
say("Launch #" .. launches .. "  (reopen me!)", WHITE)
