#pragma once
#include <vector>
#include <string>

void list_clear();
void list_add_item(const char* text);
void list_draw();
int list_handle_key(char key);   // returns selected index on Enter, -1 otherwise
int list_get_selected_index();
const char* list_get_selected_text();