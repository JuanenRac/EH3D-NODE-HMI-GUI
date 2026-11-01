// EH3D-NODE-HMI-GUI - main/ui.hpp: the screens of the touch display. GPL-3.0-or-later.
#pragma once

#include <string>
#include <vector>

#include "../core/gate.hpp"
#include "../core/hmi_config.hpp"
#include "../core/job_tools.hpp"
#include "../core/orientation.hpp"
#include "../core/quality.hpp"
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

enum class What { Action, AckRelease, Confirm, Cancel, SetOrientation, SetLanguage, OpenCard, SendJob, RefreshCard, LightSet, InspectRun, InspectPreset, InspectCalibrate, ToggleTool, Resume, SetSpindleOver };

/// What the operator did on the screen; the application runs it through the gate.
struct Request {
    What what = What::Action;
    hmi::Action action;
    hmi::Orientation orientation = hmi::Orientation::Auto;
    hmi::Lang language = hmi::Lang::En;  // SetLanguage: the language the operator chose
    char file[64] = {0};  // the name of the file of the card for OpenCard
    int light = 0;        // LightSet and InspectRun: the index in hmi::kLightLevels
    int hole = 0;         // InspectRun: the index in hmi::kHoleWidths
    int preset = 0;       // InspectPreset: 0 standard, 1 fine pitch, 2 prototype
    int tool = 0;         // ToggleTool: the number of the tool (the pen of a routing file) to switch off or on
    bool refuse = false;  // SetSpindleOver: true = refuse an order beyond the spindle, false = cut it to the top speed
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
/// Shows the quality check: the verdict of the last one, the ring of LEDs, and whether a check is running now.
void show_quality(const hmi::QualityView& quality, const hmi::LightView& light, bool running, const hmi::Settings& settings);
/// Shows what the chosen file holds: a chip for each tool (a tap switches it off or on), how many holes or cuts stay in the job, and the spindle of the machine.
/// Under the tools it says when the board does not fit the table (it does not block the job) and what the server noted about the job it made.
void show_job(const hmi::ToolSwitch& tools, const hmi::SpindleView& spindle, const hmi::JobAdvice& advice, const hmi::Settings& settings);
/// Where the job is, for the job page: a job stopped part way (and whether it can go on from there), the holes (cuts) already done, and the program that comes next.
struct FlowState {
    bool stopped = false;
    bool failed = false;     // the stopped job failed (the server's reason is in `message`) instead of being cancelled
    bool can_resume = false;
    bool held = false;       // the job is held by the operator (pause): the button that lets it go on
    bool tool_wait = false;  // the job waits at a tool change: the button that answers it
    bool route = false;     // the program in turn is a routing one
    bool gcode = false;     // the program in turn is a G-code one: its counts are lines, shown without a unit
    int done = 0;           // stopped: the holes (paths) the stopped job did
    int total = 0;          // stopped: the holes (paths) it had
    int skipped = 0;        // the holes (paths) already done before the job in turn
    std::string next;       // the name of the file of the program that comes next, empty when none
    std::string message;    // why the job stopped, as the server says it
    std::string resumed_from;  // the id of the stopped job that the job in turn goes on from, empty when none
};
/// Shows where the job is: what a stopped job did and the button to go on from there, and the program that comes next.
void show_flow(const FlowState& flow, const hmi::Settings& settings);
/// Shows the files of the card.
void show_card(const std::vector<hmi::Entry>& files, const hmi::Settings& settings);

}  // namespace ui
