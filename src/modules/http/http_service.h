#pragma once

#include <string>

struct HttpResponse {
    int status_code = -1;
    bool ok = false;
    std::string body;
    std::string error;
};

HttpResponse http_fetch(const char* url, int timeout_ms = 10000);
void http_cleanup();
