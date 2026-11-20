// EH3D-NODE-HMI-GUI - core/session.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The login of the screen. With `server_user` and `server_password` in hmi.cfg the screen logs in to EH3D-SERVER as the application `hmi` (`POST /auth/login`), keeps the token
// of that login, renews it (`POST /auth/refresh`) once it is in the last quarter of its life, and forgets it when the server answers 401 (so the next round logs in again). Without
// them it keeps using the `token` of the settings, as before. The sockets are the firmware's (main/server_link.cpp); the rules are here so that they are tested on a computer.
// Times are milliseconds of a clock that only goes forward and may wrap.
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "hmi_config.hpp"
#include "server_link.hpp"

namespace hmi {

/// The application the screen says it is when it logs in (the server knows it by this name).
inline constexpr const char* kLoginApp = "hmi";

inline Request login_request(const std::string& user, const std::string& password) {
    return {"POST", "/api/v1/auth/login", std::string("{\"name\":") + json_quote(user) + ",\"password\":" + json_quote(password) + ",\"app\":" + json_quote(kLoginApp) + "}"};
}
inline Request refresh_request() { return {"POST", "/api/v1/auth/refresh", "{}"}; }
inline Request logout_request() { return {"POST", "/api/v1/auth/logout", "{}"}; }
inline Request sessions_request() { return {"GET", "/api/v1/auth/sessions", ""}; }
/// Closes one login of the person (the id is the one `GET /auth/sessions` gives, never the token). The id is cut to what an id may hold, so nothing else can ride in the path.
inline Request close_session_request(const std::string& id) {
    std::string safe;
    for (char c : id)
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '-' || c == '_') safe += c;
    return {"DELETE", "/api/v1/auth/sessions/" + safe, ""};
}

/// The longest life of a login the screen counts (23 days): the clock of the screen is 32 bits of milliseconds.
constexpr long kMaxLoginLifeS = 2000000;

/// What `POST /auth/login` answers: the token, who the person is, the role, the application and the seconds the login lasts.
struct LoginView {
    std::string token, name, role, app, session_id;
    long expires_s = 0;
};

/// False when the answer has no token or no time left (the screen then treats the login as refused).
inline bool parse_login(const std::string& body, LoginView& out) {
    Json j;
    if (!parse_json(body, j) || j.type != Json::Type::Object) return false;
    LoginView v;
    v.token = j.str("token");
    v.name = j.str("name");
    v.role = j.str("role");
    v.app = j.str("app");
    v.session_id = j.str("session_id");
    v.expires_s = j.int_in("expires_s", 0, kMaxLoginLifeS);
    if (v.token.empty() || v.expires_s <= 0) return false;
    out = std::move(v);
    return true;
}

/// The seconds left that `POST /auth/refresh` answers; false when it says none.
inline bool parse_refresh(const std::string& body, long& expires_s) {
    Json j;
    if (!parse_json(body, j) || j.type != Json::Type::Object) return false;
    const long left = j.int_in("expires_s", 0, kMaxLoginLifeS);
    if (left <= 0) return false;
    expires_s = left;
    return true;
}

/// One open login of the person, as `GET /auth/sessions` lists it.
struct SessionRow {
    std::string id, app, role, ip;
    bool current = false;  // the login that asked
};

inline bool parse_sessions(const std::string& body, std::vector<SessionRow>& out) {
    Json j;
    if (!parse_json(body, j) || j.type != Json::Type::Object) return false;
    const Json* list = j.get("sessions");
    if (!list || list->type != Json::Type::Array) return false;
    std::vector<SessionRow> rows;
    for (const Json& s : list->a) {
        if (s.type != Json::Type::Object || s.str("id").empty() || rows.size() >= 64) continue;
        SessionRow r;
        r.id = s.str("id");
        r.app = s.str("app");
        r.role = s.str("role");
        r.ip = s.str("ip");
        const Json* cur = s.get("current");
        r.current = cur && cur->type == Json::Type::Bool && cur->b;
        rows.push_back(std::move(r));
    }
    out = std::move(rows);
    return true;
}

/// What the link has to do about the login before its next request.
enum class AuthStep { None, Login, Refresh };

/// Keeps the token of the login and when it was last renewed. It does not talk to the network: the link asks `next_step` and tells it what came back.
class SessionTracker {
public:
    /// The token of the login, empty while there is none.
    const std::string& token() const { return token_; }
    bool logged_in() const { return !token_.empty(); }

    /// True when the settings hold a name and a password to log in with.
    static bool has_credentials(const Settings& s) { return !s.server_user.empty() && !s.server_password.empty(); }

    /// What to do now. A login is made when there are credentials, no token and the wait after the last refusal is over; a login in the last quarter of its life is renewed (at
    /// most once every `kRefreshRetryMs` while the renewal keeps failing); a login that ran out is forgotten here (the next call logs in again).
    AuthStep next_step(const Settings& s, std::uint32_t now_ms) {
        if (!has_credentials(s)) return AuthStep::None;
        if (logged_in()) {
            const std::uint32_t age = now_ms - renewed_ms_;
            if (age >= life_ms_) { forget(); return next_step(s, now_ms); }
            if (life_ms_ - age <= life_ms_ / 4 && (!refresh_wait_ || elapsed(now_ms, next_refresh_ms_))) return AuthStep::Refresh;
            return AuthStep::None;
        }
        return (!login_wait_ || elapsed(now_ms, next_login_ms_)) ? AuthStep::Login : AuthStep::None;
    }

    /// A login came back good.
    void on_login(const LoginView& v, std::uint32_t now_ms) {
        token_ = v.token;
        life_ms_ = static_cast<std::uint32_t>(v.expires_s) * 1000u;
        renewed_ms_ = now_ms;
        refresh_wait_ = false;
        login_wait_ = false;
        failures_ = 0;
    }

    /// A login was refused or could not be made: the next one waits 30 s, then 60 s, 120 s... up to 10 minutes, so a wrong password is not tried over and over (the server blocks the
    /// name after a few wrong ones).
    void on_login_failed(std::uint32_t now_ms) {
        const std::uint32_t wait = kLoginWaitMs << (failures_ < 5 ? failures_ : 5);
        next_login_ms_ = now_ms + (wait < kLoginWaitMaxMs ? wait : kLoginWaitMaxMs);
        login_wait_ = true;
        if (failures_ < 30) ++failures_;
    }

    /// The renewal came back with the seconds left.
    void on_refresh(long expires_s, std::uint32_t now_ms) {
        life_ms_ = static_cast<std::uint32_t>(expires_s) * 1000u;
        renewed_ms_ = now_ms;
        refresh_wait_ = false;
    }

    /// The renewal failed: `status` 401 or 404 mean the login is gone (it is forgotten, the next round logs in); anything else keeps it and tries again later.
    void on_refresh_failed(int status, std::uint32_t now_ms) {
        if (status == 401 || status == 403 || status == 404) {
            forget();
        } else {
            next_refresh_ms_ = now_ms + kRefreshRetryMs;
            refresh_wait_ = true;
        }
    }

    /// Any request of the screen came back with `status`: a 401 means the token no longer works, so it is forgotten and the next round logs in again.
    void on_status(int status) {
        if (status == 401 && logged_in()) forget();
    }

    /// Forgets the login (the screen logged out, or the settings changed).
    void forget() {
        token_.clear();
        life_ms_ = 0;
        refresh_wait_ = false;
        login_wait_ = false;
    }

    /// The token a request carries: the one of the login when there is one, else the one of the settings.
    std::string effective_token(const Settings& s) const { return logged_in() ? token_ : s.token; }

    static constexpr std::uint32_t kLoginWaitMs = 30000;
    static constexpr std::uint32_t kLoginWaitMaxMs = 600000;
    static constexpr std::uint32_t kRefreshRetryMs = 30000;

private:
    static bool elapsed(std::uint32_t now, std::uint32_t at) { return static_cast<std::int32_t>(now - at) >= 0; }

    std::string token_;
    std::uint32_t life_ms_ = 0;
    std::uint32_t renewed_ms_ = 0;
    std::uint32_t next_refresh_ms_ = 0;
    std::uint32_t next_login_ms_ = 0;
    bool refresh_wait_ = false;  // a renewal failed: wait until next_refresh_ms_
    bool login_wait_ = false;    // a login failed: wait until next_login_ms_
    unsigned failures_ = 0;
};

// ---- tries again --------------------------------------------------------------------------------------------------------------------------------

/// Whether a request that failed may be sent again. `error` is why no answer came (`None` when one did), `status` the status of an answer. A connection that could not be made is tried again
/// for any method, because nothing was sent; a time-out or a broken connection only for the methods that do the same thing when they are sent twice (GET, HEAD, PUT, DELETE, OPTIONS), so
/// a POST (a job, a login, a start) is never sent twice by the link; a 502, 503 or 504 likewise. An answer too long is never read again.
inline bool retry_allowed(const std::string& method, CallError error, int status) {
    const bool safe = method == "GET" || method == "HEAD" || method == "PUT" || method == "DELETE" || method == "OPTIONS";
    switch (error) {
        case CallError::NoAnswer: return true;
        case CallError::Broke:
        case CallError::Send: return safe;
        case CallError::None: return safe && (status == 502 || status == 503 || status == 504);
        case CallError::NoServer:
        case CallError::Client:
        case CallError::TooLong: return false;
    }
    return false;
}

/// The wait before the try number `attempt` (1 for the first retry): 200 ms, 400 ms, 800 ms... up to 2 s.
inline std::uint32_t retry_wait_ms(unsigned attempt) {
    if (attempt == 0) return 0;
    const unsigned shift = attempt < 5 ? attempt - 1 : 4;
    const std::uint32_t wait = 200u << shift;
    return wait < 2000u ? wait : 2000u;
}

}  // namespace hmi
