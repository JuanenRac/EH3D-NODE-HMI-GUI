// EH3D-NODE-HMI-GUI - main/ui.hpp: the screens of the touch display. GPL-3.0-or-later.
#pragma once

#include <string>
#include <vector>

#include "../core/gate.hpp"
#include "../core/hmi_config.hpp"
#include "../core/orientation.hpp"
#include "../core/sdfiles.hpp"
#include "../core/server_link.hpp"

namespace ui {

/// What the screens need to know about the machine, refreshed by the application.
struct State {
    bool armed = false;
    bool stop_pressed = false;
    bool stop_latched = true;
    bool stop_wired = false;
    bool server_ok = false;
    int percent = 0;
    std::string note;  // the line under the buttons: why something was refused, what the server answered
};

enum class What { Action, AckRelease, Confirm, Cancel, SetOrientation, OpenCard, SendJob, RefreshCard };

/// What the operator did on the screen; the application runs it through the gate.
struct Request {
    What what = What::Action;
    hmi::Action action;
    hmi::Orientation orientation = hmi::Orientation::Auto;
    char file[64] = {0};  // the name of the file of the card for OpenCard
};

/// Builds the screens for a layout (with the LVGL lock taken). `post` is called, in the LVGL task, for each request of the operator.
void build(const hmi::Settings& settings, const hmi::Layout& layout, void (*post)(const Request&));
/// Shows the state (with the LVGL lock taken).
void show(const State& state, const hmi::Settings& settings);
/// Shows the question of the gate, or hides it when `action` is null.
void ask(const hmi::Action* action, const hmi::Settings& settings);
/// Shows why the gate refused.
void refused(hmi::Why why, const hmi::Settings& settings);
/// Shows the machines of the server, each with its state and its job.
void show_fleet(const hmi::FleetView& fleet, const hmi::Settings& settings);
/// Shows the files of the card.
void show_card(const std::vector<hmi::Entry>& files, const hmi::Settings& settings);

}  // namespace ui
