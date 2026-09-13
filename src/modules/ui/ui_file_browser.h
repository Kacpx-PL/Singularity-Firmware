#pragma once
#include <string>

void file_browser_start(const char* root_path, bool folder_select_mode);
void file_browser_draw();
int file_browser_handle_key(char key); // returns 1 when a file is selected
const char* file_browser_get_selected_path(); // full path of selected file
bool file_browser_selected_is_dir();