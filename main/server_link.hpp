// EH3D-NODE-HMI-GUI - main/server_link.hpp: the sockets of the client of EH3D-SERVER (the requests and the answers are core/server_link.hpp).
// GPL-3.0-or-later.
#pragma once

#include "../core/hmi_config.hpp"
#include "../core/server_link.hpp"

namespace srvlink {

/// Sends a request to the server of the settings and waits for the answer (blocking: call it from the task of the link, never from the one
/// of the screen). False with `error` when the server cannot be reached or the answer is not whole.
bool call(const hmi::Settings& settings, const hmi::Request& request, hmi::Response& response, std::string& error);

}  // namespace srvlink
