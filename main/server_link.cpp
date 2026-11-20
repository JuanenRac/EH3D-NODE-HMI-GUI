// EH3D-NODE-HMI-GUI - main/server_link.cpp: one request to EH3D-SERVER over HTTP with the login or the access token of the settings. This has not been
// built or run against a server from a module yet (the requests and the answers are tested on a computer). GPL-3.0-or-later.
#include "server_link.hpp"

#include <atomic>
#include <mutex>

#include "esp_http_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "../core/server_errors.hpp"
#include "../core/session.hpp"

namespace srvlink {

namespace {
constexpr std::size_t kMaxAnswer = 256 * 1024;  // a fleet of a big workshop is far smaller
std::string g_client;                           // `name/version`, set once at the start
std::atomic<int> g_verdict{static_cast<int>(hmi::Health::Silent)};  // what the last health check found
hmi::SessionTracker g_session;                  // the login of the screen (guarded by g_session_mutex: the stop is sent from another task)
std::mutex g_session_mutex;

esp_http_method_t method_of(const std::string& m) {
    if (m == "POST") return HTTP_METHOD_POST;
    if (m == "DELETE") return HTTP_METHOD_DELETE;
    if (m == "PUT") return HTTP_METHOD_PUT;
    return HTTP_METHOD_GET;
}

/// One try: the request goes out once and the answer is read. False with `error` when no whole answer came.
bool call_once(const hmi::Settings& s, const hmi::Request& r, hmi::Response& out, hmi::CallError& error, int timeout_ms) {
    error = hmi::CallError::None;
    esp_http_client_config_t cfg = {};
    cfg.host = s.server_host.c_str();
    cfg.port = s.server_port;
    cfg.path = r.path.c_str();
    cfg.transport_type = HTTP_TRANSPORT_OVER_TCP;
    cfg.timeout_ms = timeout_ms;
    cfg.method = method_of(r.method);
    esp_http_client_handle_t c = esp_http_client_init(&cfg);
    if (c == nullptr) {
        error = hmi::CallError::Client;
        return false;
    }
    if (!g_client.empty()) {
        esp_http_client_set_header(c, "User-Agent", g_client.c_str());
        esp_http_client_set_header(c, "X-EH3D-Client", g_client.c_str());
    }
    if (hmi::request_carries_token(r)) {
        std::string token;
        {
            std::lock_guard<std::mutex> lock(g_session_mutex);
            token = g_session.effective_token(s);
        }
        if (!token.empty()) {
            const std::string bearer = "Bearer " + token;
            esp_http_client_set_header(c, "Authorization", bearer.c_str());
        }
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
}  // namespace

void set_verdict(hmi::Health verdict) { g_verdict.store(static_cast<int>(verdict)); }

void set_client(const std::string& client) { g_client = client; }

bool call(const hmi::Settings& s, const hmi::Request& r, hmi::Response& out, hmi::CallError& error, int timeout_ms, int retries) {
    error = hmi::CallError::None;
    if (s.server_host.empty()) {
        error = hmi::CallError::NoServer;
        return false;
    }
    // whatever answers on the address is not known to be the server: nothing with the token or a body goes to it (the stop is the exception)
    if (!hmi::may_send(static_cast<hmi::Health>(g_verdict.load()), r)) {
        error = hmi::CallError::NoAnswer;
        return false;
    }
    if (timeout_ms <= 0) timeout_ms = static_cast<int>(s.server_timeout_s) * 1000;
    if (retries < 0) retries = static_cast<int>(s.server_retries);
    bool ok = false;
    for (int attempt = 0;; ++attempt) {
        out = hmi::Response();
        ok = call_once(s, r, out, error, timeout_ms);
        const bool again = ok ? hmi::retry_allowed(r.method, hmi::CallError::None, out.status) : hmi::retry_allowed(r.method, error, 0);
        if (!again || attempt >= retries) break;
        vTaskDelay(pdMS_TO_TICKS(hmi::retry_wait_ms(static_cast<unsigned>(attempt) + 1)));
    }
    // a 401 means the token of the login no longer works: it is forgotten and the next round logs in again
    if (ok && out.status == 401) {
        std::lock_guard<std::mutex> lock(g_session_mutex);
        g_session.on_status(out.status);
    }
    return ok;
}

std::string keep_login(const hmi::Settings& s, std::uint32_t now_ms) {
    hmi::AuthStep step = hmi::AuthStep::None;
    {
        std::lock_guard<std::mutex> lock(g_session_mutex);
        step = g_session.next_step(s, now_ms);
    }
    if (step == hmi::AuthStep::None) return "";
    hmi::Response r;
    hmi::CallError error = hmi::CallError::None;
    if (step == hmi::AuthStep::Login) {
        // a login is never sent twice by the link (no retries): the next one waits, see SessionTracker::on_login_failed
        const bool reached = call(s, hmi::login_request(s.server_user, s.server_password), r, error, 0, 0);
        hmi::LoginView view;
        const bool good = reached && r.status == 200 && hmi::parse_login(r.body, view);
        std::lock_guard<std::mutex> lock(g_session_mutex);
        if (good) {
            g_session.on_login(view, now_ms);
            return "";
        }
        g_session.on_login_failed(now_ms);
        return reached ? hmi::error_note(r, s.language) : hmi::call_error_text(error, s, s.language);
    }
    const bool reached = call(s, hmi::refresh_request(), r, error, 0, 0);
    long left = 0;
    std::lock_guard<std::mutex> lock(g_session_mutex);
    if (reached && r.status == 200 && hmi::parse_refresh(r.body, left)) g_session.on_refresh(left, now_ms);
    else g_session.on_refresh_failed(reached ? r.status : 0, now_ms);
    return "";
}

bool logged_in() {
    std::lock_guard<std::mutex> lock(g_session_mutex);
    return g_session.logged_in();
}

void logout(const hmi::Settings& s) {
    if (!logged_in()) return;
    hmi::Response r;
    hmi::CallError error = hmi::CallError::None;
    call(s, hmi::logout_request(), r, error, 0, 0);
    std::lock_guard<std::mutex> lock(g_session_mutex);
    g_session.forget();
}

}  // namespace srvlink
