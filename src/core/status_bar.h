#pragma once

// Height in pixels the status bar occupies at the top of the screen.
// Menu and app content should start below this Y offset.
extern const int STATUS_BAR_HEIGHT;

void status_bar_draw();

// Shows the volume level in place of the normal status bar content for ~1.5 s, then the normal
// bar comes back by itself. Call status_bar_tick() every loop so the bar is restored on time.
void status_bar_show_volume();
void status_bar_tick();