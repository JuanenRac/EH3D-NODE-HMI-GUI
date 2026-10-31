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
//  * a spindle order never asks for more turns than the spindle gives (the setting `spindle_max_rpm` and the top speed of the spindle the server says the machine carries,
//    whichever is lower): with `spindle_over = clip` (the default) the order is cut to that speed and the screen says so, with `refuse` it is refused; the confirmation
//    always shows the turns the spindle will really get;
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
    int rpm = 0;       // Spindle: the turns asked (0: the speed the job says); the gate cuts it to what the spindle gives and writes the turns it will really get in `what`
};

enum class Verdict { Run, NeedConfirm, Refused };
enum class Why { None, HardwareStop, NotArmed, VoiceOrdersOff, NoHardwareStop, SpindleDisabled, NoSpindleHere, SpindleTooFast };

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
        drop();
        if (latched_ || pressed_) return {Verdict::Refused, Why::HardwareStop};
        if (s_.hw_stop_required && !wired_) return {Verdict::Refused, Why::NoHardwareStop};
        if (from == Origin::Voice && !s_.voice_orders) return {Verdict::Refused, Why::VoiceOrdersOff};
        if (a.kind == Kind::Spindle && s_.spindle_max_rpm == 0) return {Verdict::Refused, Why::SpindleDisabled};
        if (a.kind == Kind::Spindle && !controls_for(s_.machine).spindle) return {Verdict::Refused, Why::NoSpindleHere};
        if (!armed_) return {Verdict::Refused, Why::NotArmed};
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
    int machine_max_rpm_ = 0;
    bool pending_ = false;
    Action action_;
    std::uint32_t deadline_ = 0;
};

}  // namespace hmi
