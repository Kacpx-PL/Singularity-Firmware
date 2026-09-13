#pragma once
#include "app.h"

App make_lua_app(const char* name, const char* script, const Icon* icon);
App make_lua_app_from_file(const char* name, const char* path, const Icon* icon);
App make_folder_app(const char* name, const char* path, const Icon* icon);