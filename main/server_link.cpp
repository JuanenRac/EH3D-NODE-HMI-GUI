// EH3D-NODE-HMI-GUI - main/server_link.cpp: one request to EH3D-SERVER over HTTP with the access token of the settings. This has not been
// built or run against a server from a module yet (the requests and the answers are tested on a computer). GPL-3.0-or-later.
#include "server_link.hpp"

#include "esp_http_client.h"

namespace srvlink {

namespace {
constexpr std::size_t kMaxAnswer = 256 * 1024;  // a fleet of a big workshop is far smaller
std::string g_client;                           // `name/version`, set once at the start
}

void set_client(const std::string& client) { g_client = client; }

bool call(const hmi::Settings& s, const hmi::Request& r, hmi::Response& out, hmi::CallError& error, int timeout_ms) {
    error = hmi::CallError::None;
    if (s.server_host.empty()) {
        error = hmi::CallError::NoServer;
        return false;
    }
    esp_http_client_config_t cfg = {};
    cfg.host = s.server_host.c_str();
    cfg.port = s.server_port;
    cfg.path = r.path.c_str();
    cfg.transport_type = HTTP_TRANSPORT_OVER_TCP;
    cfg.timeout_ms = timeout_ms;
    cfg.method = r.method == "POST" ? HTTP_METHOD_POST : HTTP_METHOD_GET;
    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    if (c == nullptr) {
        error = hmi::CallError::Client;
        return false;
    }
    if (!g_client.empty()) {
        esp_http_client_set_header(c, "User-Agent", g_client.c_str());
        esp_http_client_set_header(c, "X-EH3D-Client", g_client.c_str());
    }
    if (!s.token.empty()) {
        const std::string bearer = "Bearer " + s.token;
        esp_http_client_set_header(c, "Authorization", bearer.c_str());
    }
    if (!r.body.empty()) esp_http_client_set_header(c, "Content-Type", "application/json");
    bool ok = false;
    if (esp_http_client_open(c, static_cast<int>(r.body.size())) == ESP_OK) {
        if (r.body.empty() || esp_http_client_write(c, r.body.data(), static_cast<int>(r.body.size())) == static_cast<int>(r.body.size())) {
            esp_http_client_fetch_headers(c);
            out.status = esp_http_client_get_status_code(c);
            out.body.clear();
            char buf[512];
            int n;
            ok = true;
            while ((n = esp_http_client_read(c, buf, sizeof buf)) > 0) {
                if (out.body.size() + static_cast<std::size_t>(n) > kMaxAnswer) {
                    error = hmi::CallError::TooLong;
                    ok = false;
                    break;
                }
                out.body.append(buf, static_cast<std::size_t>(n));
            }
            if (n < 0) {
                error = hmi::CallError::Broke;
                ok = false;
            }
        } else {
            error = hmi::CallError::Send;
        }
    } else {
        error = hmi::CallError::NoAnswer;
    }
    esp_http_client_close(c);
    esp_http_client_cleanup(c);
    return ok;
}

}  // namespace srvlink
