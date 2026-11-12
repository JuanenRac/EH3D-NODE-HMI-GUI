// EH3D-NODE-HMI-GUI - core/gate.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The gate between what the operator asks (a touch or a spoken order) and what the screen sends to the machine.
//
// The rules, all of them tested on the host:
//  * a stop is never held back: from a touch or from the voice, in any state, it goes at once; so is a pause, which only holds the job;
//  * the press of the hardware stop is an edge the application takes once, to stop the job of the machine on the server;
//  * the start of a job that is ready asks like a move, but it does not need the link to be armed (a job is armed only once it runs);
//  * the hardware stop is a latch the screen cannot clear by itself: while the input is pressed, or until the operator has
//    acknowledged its release on the screen, no move and no spindle order passes;
//  * a move by hand from the jog panel (`Kind::Jog`) passes without a running job and is refused while one runs (unless the switch `jog_block_running` is turned off);
//  * any other move or a spindle order passes only when the link is armed, and (unless the bench setting turns it off for the touch) it first asks for a
//    confirmation that lives `confirm_timeout_ms`; a spoken order always asks, whatever the setting says;
//  * a spoken "yes" confirms only when the setting `voice_confirm` is on; by default only a touch on the screen does;
//  * a spindle order never asks for more turns than the spindle gives (the setting `spindle_max_rpm` and the top speed of the spindle the server says the machine carries,
//    whichever is lower): with `spindle_over = clip` (the default) the order is cut to that speed and the screen says so, with `refuse` it is refused; the confirmation
//    always shows the turns the spindle will really get;
//  * a new request drops the one that waited, and a lost link drops it too.
#pragma once

#include <cstdint>
#include <string>

#include "hmi_config.hpp"
#include "ops.hpp"

namespace hmi {

enum class Origin { Touch, Voice };
enum class Kind { Info, Motion, Spindle, Stop, Pause, Start, Jog };

struct Action {
    Kind kind = Kind::Info;
    std::string what;  // what is asked, as the confirmation shows it ("jog X +10 mm")
    int rpm = 0;       // Spindle: the turns asked (0: the speed the job says); the gate cuts it to what the spindle gives and writes the turns it will really get in `what`
    Op op = Op::None;  // the order as an enum (core/ops.hpp): what the application compares, never the text of `what`
};

enum class Verdict { Run, NeedConfirm, Refused };
enum class Why { None, HardwareStop, NotArmed, VoiceOrdersOff, NoHardwareStop, SpindleDisabled, NoSpindleHere, SpindleTooFast, JobRunning };

struct Decision {
    Verdict verdict = Verdict::Refused;
    Why why = Why::None;
    bool clipped = false;  // a spindle order that asked for more than the spindle gives and was cut to it (`rpm` is what it will get)
    int asked_rpm = 0;     // the turns the operator asked, when they were cut
    int rpm = 0;           // the turns of a spindle order that goes on
};

class Gate {
public:
    explicit Gate(const Settings& s) : s_(s) {}

    /// The state of the hardware stop input: `wired` says whether the input is read at all, `pressed` whether the button is pushed.
    void hardware_stop(bool wired, bool pressed) {
        wired_ = wired;
        if (pressed) {
            if (!pressed_) stop_edge_ = true;
            latched_ = true;
            drop();
        }
        pressed_ = pressed;
    }

    /// True once each time the hardware stop is pressed (not while it is held down): the application then stops the job of the machine on the server at once.
    bool take_stop_edge() {
        const bool edge = stop_edge_;
        stop_edge_ = false;
        return edge;
    }

    /// The link to the machine: armed means the program may move it. Losing the arming drops what waited.
    void link(bool armed) {
        armed_ = armed;
        if (!armed && !(pending_ && action_.kind == Kind::Start)) drop();  // the start of a ready job waits for its answer while the link is not armed
    }

    /// The operator says on the screen that the stop was released and the machine may be used again. It works only once the button is up.
    bool acknowledge_release() {
        if (pressed_) return false;
        latched_ = false;
        return true;
    }

    /// The top speed of the spindle the machine carries, as the server says it (0 when it is not known: only the setting limits the order).
    void machine_spindle_max(int rpm) { machine_max_rpm_ = rpm < 0 ? 0 : rpm; }
    /// The most turns a spindle order may get now: the setting and the spindle of the machine, whichever is lower (0: the screen never starts the spindle).
    int spindle_limit() const {
        const int set = static_cast<int>(s_.spindle_max_rpm);
        if (set <= 0) return 0;
        return machine_max_rpm_ > 0 && machine_max_rpm_ < set ? machine_max_rpm_ : set;
    }

    bool latched() const { return latched_; }
    bool pending() const { return pending_; }
    const Action& pending_action() const { return action_; }

    /// An operator's request at time `now_ms`.
    Decision request(const Action& a, Origin from, std::uint32_t now_ms) {
        if (a.kind == Kind::Stop) {
            drop();
            return {Verdict::Run, Why::None};
        }
        if (a.kind == Kind::Info) return {Verdict::Run, Why::None};
        if (a.kind == Kind::Pause) {
            drop();
            return {Verdict::Run, Why::None};
        }
        drop();
        if (latched_ || pressed_) return {Verdict::Refused, Why::HardwareStop};
        if (s_.hw_stop_required && !wired_) return {Verdict::Refused, Why::NoHardwareStop};
        if (from == Origin::Voice && !s_.voice_orders) return {Verdict::Refused, Why::VoiceOrdersOff};
        if (a.kind == Kind::Spindle && s_.spindle_max_rpm == 0) return {Verdict::Refused, Why::SpindleDisabled};
        // a move by hand (the jog panel) does not need a job to be running; it is refused while one runs (the switch `jog_block_running`), and the hardware stop above always blocks it
        if (a.kind == Kind::Spindle && !controls_for(s_.machine).spindle) return {Verdict::Refused, Why::NoSpindleHere};
        if (a.kind == Kind::Jog && armed_ && s_.jog_block_running) return {Verdict::Refused, Why::JobRunning};
        if (!armed_ && a.kind != Kind::Start && a.kind != Kind::Jog) return {Verdict::Refused, Why::NotArmed};
        Decision d;
        action_ = a;
        if (a.kind == Kind::Spindle && a.rpm > 0) {
            const int limit = spindle_limit();
            if (a.rpm > limit) {
                if (s_.spindle_over == SpindleOver::Refuse) return {Verdict::Refused, Why::SpindleTooFast};
                d.clipped = true;
                d.asked_rpm = a.rpm;
                action_.rpm = limit;
            }
            action_.what = a.what + " " + std::to_string(action_.rpm) + " rpm";
            d.rpm = action_.rpm;
        }
        if (from == Origin::Touch && !s_.confirm_moves) {
            d.verdict = Verdict::Run;
            return d;
        }
        deadline_ = now_ms + s_.confirm_timeout_ms;
        pending_ = true;
        d.verdict = Verdict::NeedConfirm;
        return d;
    }

    /// The confirmation of the waiting request; the action to run is copied to `out`. False when nothing waits, the time is over, or the
    /// confirmation comes from a source the settings do not accept.
    bool confirm(Origin from, std::uint32_t now_ms, Action& out) {
        if (!pending_) return false;
        if (static_cast<std::int32_t>(now_ms - deadline_) > 0) {
            drop();
            return false;
        }
        if (from == Origin::Voice && !s_.voice_confirm) return false;
        if (latched_ || pressed_ || (!armed_ && action_.kind != Kind::Start && action_.kind != Kind::Jog) || (armed_ && action_.kind == Kind::Jog && s_.jog_block_running)) {
            drop();
            return false;
        }
        out = action_;
        drop();
        return true;
    }

    void cancel() { drop(); }

    /// How long the waiting question still lives (0 when nothing waits or the time is over): the screen counts it down.
    std::uint32_t remaining_ms(std::uint32_t now_ms) const {
        if (!pending_) return 0;
        const std::int32_t left = static_cast<std::int32_t>(deadline_ - now_ms);
        return left > 0 ? static_cast<std::uint32_t>(left) : 0;
    }

    /// Time passes: a request nobody confirmed in time is dropped.
    void tick(std::uint32_t now_ms) {
        if (pending_ && static_cast<std::int32_t>(now_ms - deadline_) > 0) drop();
    }

private:
    void drop() { pending_ = false; }

    const Settings& s_;
    bool wired_ = false;
    bool pressed_ = false;
    bool latched_ = true;  // a screen that has just started has not seen the stop released yet
    bool armed_ = false;
    bool stop_edge_ = false;
    int machine_max_rpm_ = 0;
    bool pending_ = false;
    Action action_;
    std::uint32_t deadline_ = 0;
};

}  // namespace hmi
