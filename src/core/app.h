#pragma once
#include "icons.h"

struct App {
    const char* name;
    const char* data;
    const Icon* icon;
    int priority;
    bool is_folder;
    void (*on_open)(const char* data, const char* name);
    void (*on_loop)(const char* data);
    void (*on_key)(const char* data, char key);
    void (*on_close)(const char* data);
};