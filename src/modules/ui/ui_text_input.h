#pragma once

void text_input_start(const char* prompt, bool mask);
void text_input_draw();
int text_input_handle_key(char key); // 0 typing, 1 submitted, 2 cancelled
const char* text_input_get_value();