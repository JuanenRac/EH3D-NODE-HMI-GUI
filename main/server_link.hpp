// EH3D-NODE-HMI-GUI - main/server_link.hpp: the sockets of the client of EH3D-SERVER (the requests and the answers are core/server_link.hpp).
// GPL-3.0-or-later.
#pragma once

#include "../core/hmi_config.hpp"
#include "../core/server_link.hpp"

namespace srvlink {

/// Sends a request to the server of the settings and waits for the answer (blocking: call it from the task of the link, never from the one
/// of the screen). False with `error` when the server cannot be reached or the answer is not whole.
/// `timeout_ms` is how long it waits for the server (the stop uses a short time and tries again).
bool call(const hmi::Settings& settings, const hmi::Request& request, hmi::Response& response, hmi::CallError& error, int timeout_ms = 4000);

/// Names this screen to the server: `name/version` goes in the headers User-Agent and X-EH3D-Client of every request. Call it once, before the tasks start.
void set_client(const std::string& client);

}  // namespace srvlink
