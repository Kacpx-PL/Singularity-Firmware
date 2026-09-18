#include "http_service.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <memory>
#include <vector>

namespace {

struct HttpContext {
    HTTPClient* client = nullptr;
    WiFiClientSecure* secure_client = nullptr;
    bool active = false;
};

static HttpContext g_http_context;

void http_release_context() {
    if (g_http_context.secure_client) {
        delete g_http_context.secure_client;
        g_http_context.secure_client = nullptr;
    }
    if (g_http_context.client) {
        g_http_context.client->end();
        delete g_http_context.client;
        g_http_context.client = nullptr;
    }
    g_http_context.active = false;
}

}  // namespace

HttpResponse http_fetch(const char* url, int timeout_ms) {
    HttpResponse response;

    if (!url || !*url) {
        response.error = "empty url";
        return response;
    }

    if (WiFi.status() != WL_CONNECTED) {
        response.error = "wifi not connected";
        return response;
    }

    http_release_context();

    const bool use_https = (strncasecmp(url, "https://", 8) == 0);
    const bool use_http = (strncasecmp(url, "http://", 7) == 0);
    if (!use_http && !use_https) {
        response.error = "unsupported url scheme";
        return response;
    }

    HTTPClient* client = new (std::nothrow) HTTPClient();
    if (!client) {
        response.error = "out of memory";
        return response;
    }

    g_http_context.client = client;
    g_http_context.active = true;

    client->setTimeout(timeout_ms);
    client->setConnectTimeout(timeout_ms);
    client->setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

    bool begin_ok = false;
    if (use_https) {
        WiFiClientSecure* secure_client = new (std::nothrow) WiFiClientSecure();
        if (!secure_client) {
            delete client;
            response.error = "out of memory";
            return response;
        }
        secure_client->setInsecure();
        g_http_context.secure_client = secure_client;
        begin_ok = client->begin(*secure_client, url);
    } else {
        WiFiClient* plain_client = new (std::nothrow) WiFiClient();
        if (!plain_client) {
            delete client;
            response.error = "out of memory";
            return response;
        }
        begin_ok = client->begin(*plain_client, url);
    }

    if (!begin_ok) {
        response.error = "http begin failed";
        http_release_context();
        return response;
    }

    int http_code = client->GET();
    response.status_code = http_code;
    response.ok = (http_code >= 200 && http_code < 300);

    if (response.ok) {
        response.body = client->getString().c_str();
    } else {
        response.error = client->errorToString(http_code).c_str();
    }

    http_release_context();
    return response;
}

void http_cleanup() {
    http_release_context();
}
