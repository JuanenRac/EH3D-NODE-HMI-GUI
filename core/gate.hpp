// EH3D-NODE-HMI-GUI - core/gate.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The gate between what the operator asks (a touch or a spoken order) and what the screen sends to the machine.
//
// The rules, all of them tested on the host:
//  * a stop is never held back: from a touch or from the voice, in any state, it goes at once;
//  * the hardware stop is a latch the screen cannot clear by itself: while the input is pressed, or until the operator has
//    acknowledged its release on the screen, no move and no spindle order passes;
//  * a move or a spindle order passes only when the link is armed, and (unless the bench setting turns it off for the touch) it first asks for a
//    confirmation that lives `confirm_timeout_ms`; a spoken order always asks, whatever the setting says;
//  * a spoken "yes" confirms only when the setting `voice_confirm` is on; by default only a touch on the screen does;
//  * a new request drops the one that waited, and a lost link drops it too.
#pragma once

#include <cstdint>
#include <string>

#include "hmi_config.hpp"

namespace hmi {

enum class Origin { Touch, Voice };
enum class Kind { Info, Motion, Spindle, Stop };

struct Action {
    Kind kind = Kind::Info;
    std::string what;  // what is asked, as the confirmation shows it ("jog X +10 mm")
};

enum class Verdict { Run, NeedConfirm, Refused };
enum class Why { None, HardwareStop, NotArmed, VoiceOrdersOff, NoHardwareStop, SpindleDisabled, NoSpindleHere };

struct Decision {
    Verdict verdict = Verdict::Refused;
    Why why = Why::None;
};

class Gate {
public:
    explicit Gate(const Settings& s) : s_(s) {}

    /// The state of the hardware stop input: `wired` says whether the input is read at all, `pressed` whether the button is pushed.
    void hardware_stop(bool wired, bool pressed) {
        wired_ = wired;
        if (pressed) {
            latched_ = true;
            drop();
        }
        pressed_ = pressed;
    }

    /// The link to the machine: armed means the program may move it. Losing the arming drops what waited.
    void link(bool armed) {
        armed_ = armed;
        if (!armed) drop();
    }

    /// The operator says on the screen that the stop was released and the machine may be used again. It works only once the button is up.
    bool acknowledge_release() {
        if (pressed_) return false;
        latched_ = false;
        return true;
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
        drop();
        if (latched_ || pressed_) return {Verdict::Refused, Why::HardwareStop};
        if (s_.hw_stop_required && !wired_) return {Verdict::Refused, Why::NoHardwareStop};
        if (from == Origin::Voice && !s_.voice_orders) return {Verdict::Refused, Why::VoiceOrdersOff};
        if (a.kind == Kind::Spindle && !controls_for(s_.machine).spindle) return {Verdict::Refused, Why::NoSpindleHere};
        if (a.kind == Kind::Spindle && s_.spindle_max_rpm == 0) return {Verdict::Refused, Why::SpindleDisabled};
        if (!armed_) return {Verdict::Refused, Why::NotArmed};
        if (from == Origin::Touch && !s_.confirm_moves) return {Verdict::Run, Why::None};
        action_ = a;
        deadline_ = now_ms + s_.confirm_timeout_ms;
        pending_ = true;
        return {Verdict::NeedConfirm, Why::None};
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
        if (latched_ || pressed_ || !armed_) {
            drop();
            return false;
        }
        out = action_;
        drop();
        return true;
    }

    void cancel() { drop(); }

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
    bool pending_ = false;
    Action action_;
    std::uint32_t deadline_ = 0;
};

}  // namespace hmi
