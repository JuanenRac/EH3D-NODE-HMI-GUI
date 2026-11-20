// EH3D-NODE-HMI-GUI - main/server_link.hpp: the sockets of the client of EH3D-SERVER (the requests and the answers are core/server_link.hpp).
// GPL-3.0-or-later.
#pragma once

#include "../core/hmi_config.hpp"
#include "../core/server_link.hpp"

namespace srvlink {

/// Sends a request to the server of the settings and waits for the answer (blocking: call it from the task of the link, never from the one
/// of the screen). False with `error` when the server cannot be reached or the answer is not whole.
/// `timeout_ms` is how long it waits for the server in each try (0: `server_timeout_s` of the settings; the stop uses a short time and has tries of its own). `retries` is how many more
/// tries follow a failure (-1: `server_retries` of the settings, 0: none); a request is sent again only when that is safe (see `hmi::retry_allowed`). The token is the one of the login
/// when there is one, else the one of the settings; a 401 makes the screen forget the login.
bool call(const hmi::Settings& settings, const hmi::Request& request, hmi::Response& response, hmi::CallError& error, int timeout_ms = 0, int retries = -1);

/// Logs in (`POST /auth/login`, application `hmi`) when the settings have a name and a password and there is no login, and renews the login in the last quarter of its life. Call it from
/// the task of the link, once per round, after the health check. Returns the sentence to show the operator when a login was refused (in their language), else empty.
std::string keep_login(const hmi::Settings& settings, std::uint32_t now_ms);

/// Whether the screen holds a login now.
bool logged_in();

/// The role the server gave the login (`viewer`, `operator`, `admin`); empty while there is no login (the screen then never changes a setting of the server).
std::string role();

/// Ends the login at the server and forgets the token.
void logout(const hmi::Settings& settings);

/// Tells the link what the last health check found: after `NotEh3d` or `Silent` it sends nothing with the token or a body (only the health check and the stop). It starts as `Silent`, so
/// nothing but the health check goes out before the first one. Callable from any task.
void set_verdict(hmi::Health verdict);

/// Names this screen to the server: `name/version` goes in the headers User-Agent and X-EH3D-Client of every request. Call it once, before the tasks start.
void set_client(const std::string& client);

}  // namespace srvlink
