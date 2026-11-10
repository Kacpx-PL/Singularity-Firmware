#include "http_service.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <memory>
#include <vector>

namespace {

// Owns every heap object created for one request. Everything is released
// in one place (http_release_context) so no code path can leak or double free.
struct HttpContext {
    HTTPClient* client = nullptr;
    WiFiClientSecure* secure_client = nullptr;
    WiFiClient* plain_client = nullptr;
    bool active = false;
};

static HttpContext g_http_context;

void http_release_context() {
    // HTTPClient::end() touches the underlying transport, so it must run
    // BEFORE the transport object is deleted.
    if (g_http_context.client) {
        g_http_context.client->end();
        delete g_http_context.client;
        g_http_context.client = nullptr;
    }
    if (g_http_context.secure_client) {
        delete g_http_context.secure_client;
        g_http_context.secure_client = nullptr;
    }
    if (g_http_context.plain_client) {
        delete g_http_context.plain_client;
        g_http_context.plain_client = nullptr;
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

    g_http_context.client = new (std::nothrow) HTTPClient();
    if (!g_http_context.client) {
        response.error = "out of memory";
        http_release_context();
        return response;
    }
    g_http_context.active = true;
    HTTPClient* client = g_http_context.client;

    client->setTimeout(timeout_ms);
    client->setConnectTimeout(timeout_ms);
    client->setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);

    bool begin_ok = false;
    if (use_https) {
        g_http_context.secure_client = new (std::nothrow) WiFiClientSecure();
         if (!g_http_context.secure_client) {
            response.error = "out of memory";
            http_release_context();
            return response;
        }
        g_http_context.secure_client->setInsecure();
        begin_ok = client->begin(*g_http_context.secure_client, url);
    } else {
        g_http_context.plain_client = new (std::nothrow) WiFiClient();
        if (!g_http_context.plain_client) {
            response.error = "out of memory";
            http_release_context();
            return response;
        }
        begin_ok = client->begin(*g_http_context.plain_client, url);
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
