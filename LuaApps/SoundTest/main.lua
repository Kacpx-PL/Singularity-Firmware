-- Jingle: plays a few notes when the app opens. Press R to play it again.
--
-- snd.tone() returns immediately and a new tone replaces the one still playing, so notes
-- can't just be called one after another (they would cut each other off, and `while true`
-- loops are forbidden). Instead update() starts each note only once the previous note and
-- the silence after it are over, using sys.millis().

local WHITE, GREEN, GREY = 0xFFFF, 0x07E0, 0x7BEF

-- { frequency in Hz, duration in ms }   C5 E5 G5 C6 (a C major arpeggio)
local NOTES = {
    { 523, 120 },
    { 659, 120 },
    { 784, 120 },
    { 1047, 300 },
}
local GAP_MS = 40          -- silence between notes
local VOLUME = 0.6         -- app volume 0.0..1.0, scales inside the user's global volume

local index = 1            -- next note to play (> #NOTES means finished)
local next_at = 0          -- sys.millis() time at which the next note may start
local status = ""
local muted = false

local function show(text, color)
    if text == status then return end   -- only redraw when it changed
    status = text
    gfx.clearScreen()
    gfx.drawText("Jingle", 10, 30, 3, WHITE)
    gfx.drawText(text, 10, 70, 2, color)
    gfx.drawText("R = play again", 10, 110, 1, GREY)
end

local function start()
    index = 1
    next_at = sys.millis()
    status = ""
    muted = false
end

start()   -- top-level code runs once when the app opens

function on_key(key)
    if key == 114 or key == 82 then   -- r / R
        start()
    end
end

function update()
    if index > #NOTES then
        -- wait for the last note to finish before saying we're done
        if not muted and sys.millis() >= next_at then show("Done!", GREEN) end
        return
    end

    if sys.millis() < next_at then return end   -- previous note or gap still going

    local note = NOTES[index]
    if not snd.tone(note[1], note[2], VOLUME) then
        -- tone() returns false when the user has muted the device (Fn + - to lower the volume)
        show("Muted", GREY)
        muted = true
        index = #NOTES + 1
        return
    end

    show("Playing " .. index .. "/" .. #NOTES, WHITE)
    next_at = sys.millis() + note[2] + GAP_MS
    index = index + 1
end
