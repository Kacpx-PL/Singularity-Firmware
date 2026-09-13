#pragma once
#include "app.h"

void menu_init();          // call once from setup(), after display is ready
void menu_update(char key); // call every loop, pass whatever input_read() returned (0 is fine)
void menu_enter_folder(const char* path);
void menu_go_back();