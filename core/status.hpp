// EH3D-NODE-HMI-GUI - core/status.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// What the screen tells the operator about itself, in the language of the operator and never as a bare English literal: the errors of the settings file, the state of the link to
// the server (connected, no connection, a different server) with the age of the data, the notes of the application, the question of the gate with its countdown. The firmware only
// shows these texts; the rules are tested on a computer.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "hmi_config.hpp"
#include "server_link.hpp"
#include "ui_text.hpp"
#include "why_text.hpp"

namespace hmi {

// ---- the errors of the settings file ---------------------------------------------------------------------------------------------------------

inline const char* cfg_reason_text(CfgReason r, Lang l) {
    switch (r) {
        case CfgReason::NotPair: return text(T::CfgNotPair, l);
        case CfgReason::Range: return text(T::CfgRange, l);
        case CfgReason::OnOff: return text(T::CfgOnOff, l);
        case CfgReason::Choice: return text(T::CfgChoice, l);
        case CfgReason::Token: return text(T::CfgToken, l);
        case CfgReason::Unknown: return text(T::CfgUnknown, l);
    }
    return "";
}

/// What the screen says about the lines of hmi.cfg that were not used: the first one (its number, its key and why) and how many more there are; empty when all were fine.
/// The token is never named in the text beyond its key, and its value is never shown.
inline std::string cfg_report(const std::vector<Problem>& problems, Lang l) {
    if (problems.empty()) return "";
    const Problem& p = problems.front();
    std::string out = std::string(text(T::CfgLine, l)) + " " + std::to_string(p.line);
    if (p.reason != CfgReason::NotPair && !p.key.empty()) out += " (" + p.key + ")";
    out += ": ";
    out += cfg_reason_text(p.reason, l);
    if (problems.size() > 1) out += "  +" + std::to_string(problems.size() - 1) + " " + text(T::CfgMore, l);
    return out;
}

// ---- the link to the server ---------------------------------------------------------------------------------------------------------------------

/// What the screen knows about the server: nothing yet, connected, no connection, or an answer that is not the one of the server this screen is set for.
enum class Link { Unknown, Connected, Lost, OtherServer };

/// What came back from a reading of the fleet.
enum class Answer { Fresh, ServerError, NotEh3d, Silent };

/// Sorts the answer of `GET /fleet`: `reached` says whether any answer came, `parsed` whether it was a fleet. A server that answers with its own error (`{"error": ...}`: a wrong
/// token, for example) is an EH3D server that did not give data; any other answer is not from an EH3D server.
inline Answer classify_fleet(bool reached, const Response& r, bool parsed) {
    if (!reached) return Answer::Silent;
    if (r.status == 200 && parsed) return Answer::Fresh;
    Json j;
    if (parse_json(r.body, j) && j.type == Json::Type::Object && !j.str("error").empty()) return Answer::ServerError;
    return Answer::NotEh3d;
}

/// True when the machine of the settings is among the machines the server lists (or the settings name none).
inline bool fleet_has_machine(const FleetView& v, const std::string& machine) {
    if (machine.empty()) return true;
    for (const MachineRow& m : v.machines)
        if (m.id == machine) return true;
    return false;
}

/// Keeps the state of the link and the age of the data it brought. Times are milliseconds of a clock that only goes forward.
class LinkTracker {
public:
    /// A reading of the fleet came back as `a`; `machine_found` is false when the server does not list the machine of the settings.
    void update(Answer a, bool machine_found, std::uint32_t now_ms) {
        switch (a) {
            case Answer::Fresh:
                state_ = machine_found ? Link::Connected : Link::OtherServer;
                fresh_ = true;
                last_fresh_ = now_ms;
                break;
            case Answer::ServerError: state_ = Link::Connected; break;  // it is there, but gave no data
            case Answer::NotEh3d: state_ = Link::OtherServer; break;
            case Answer::Silent: state_ = Link::Lost; break;
        }
    }
    Link state() const { return state_; }
    /// True once data have come at least once.
    bool has_data() const { return fresh_; }
    /// Whole seconds since the last data came (0 before any).
    std::uint32_t age_s(std::uint32_t now_ms) const { return fresh_ ? (now_ms - last_fresh_) / 1000u : 0u; }
    /// True when the data shown are older than `stale_after_s`: the operator must not take them for current. Data that never came are not old, they are absent.
    bool stale(std::uint32_t now_ms, std::uint32_t stale_after_s) const { return fresh_ && age_s(now_ms) >= stale_after_s; }

private:
    Link state_ = Link::Unknown;
    bool fresh_ = false;
    std::uint32_t last_fresh_ = 0;
};

/// The line of the top bar: the state of the link, then the age of the data when they are old (`Connected | Old data 42 s`).
inline std::string link_label(Link s, bool stale, std::uint32_t age_s, Lang l) {
    std::string out;
    switch (s) {
        case Link::Unknown: out = "-"; break;
        case Link::Connected: out = text(T::LinkOk, l); break;
        case Link::Lost: out = text(T::LinkLost, l); break;
        case Link::OtherServer: out = text(T::LinkOther, l); break;
    }
    if (stale) out += std::string("  |  ") + text(T::LinkOld, l) + " " + std::to_string(age_s) + " s";
    return out;
}

/// The mark put after what is shown from old data (a row of the list of machines).
inline std::string stale_mark(Lang l) { return std::string("[") + text(T::LinkOld, l) + "]"; }

// ---- what went wrong in a call to the server (the enum CallError is in server_link.hpp) --------------------------------------------------------

/// The error as the operator reads it; when the server does not answer the text names where the screen looked (`host:port`).
inline std::string call_error_text(CallError e, const Settings& s, Lang l) {
    switch (e) {
        case CallError::None: return "";
        case CallError::NoServer: return text(T::SrvNone, l);
        case CallError::Client: return text(T::ErrClient, l);
        case CallError::TooLong: return text(T::ErrTooLong, l);
        case CallError::Broke: return text(T::ErrBroke, l);
        case CallError::Send: return text(T::ErrSend, l);
        case CallError::NoAnswer: return std::string(text(T::SrvSilent, l)) + ": " + s.server_host + ":" + std::to_string(s.server_port);
    }
    return "";
}

/// The note for a link that is not connected, or empty when it is. It is said once, when the state changes into it.
inline std::string link_note(Link s, const Settings& st, Lang l) {
    if (st.server_host.empty()) return text(T::SrvNone, l);
    if (s == Link::Lost) return call_error_text(CallError::NoAnswer, st, l);
    if (s == Link::OtherServer) return std::string(text(T::SrvOther, l)) + ": " + st.server_host + ":" + std::to_string(st.server_port);
    return "";
}

// ---- the notes of the application ---------------------------------------------------------------------------------------------------------------

enum class ScreenNote { StopFail, StopRefused, StopSent, NoJobToStop, MoveNoLink, Busy, NothingToSend, OldServer, Ok, Following, NoJobReady, GoingOn, File, Unreadable, AllOff };

inline const char* note_text(ScreenNote n, Lang l) {
    switch (n) {
        case ScreenNote::StopFail: return text(T::NoteStopFail, l);
        case ScreenNote::StopRefused: return text(T::NoteStopRefused, l);
        case ScreenNote::StopSent: return text(T::NoteStopSent, l);
        case ScreenNote::NoJobToStop: return text(T::NoteNoJobToStop, l);
        case ScreenNote::MoveNoLink: return text(T::NoteMoveNoLink, l);
        case ScreenNote::Busy: return text(T::NoteBusy, l);
        case ScreenNote::NothingToSend: return text(T::NoteNothingToSend, l);
        case ScreenNote::OldServer: return text(T::NoteOldServer, l);
        case ScreenNote::Ok: return text(T::NoteOk, l);
        case ScreenNote::Following: return text(T::NoteFollowing, l);
        case ScreenNote::NoJobReady: return text(T::NoteNoJobReady, l);
        case ScreenNote::GoingOn: return text(T::NoteGoingOn, l);
        case ScreenNote::File: return text(T::NoteFile, l);
        case ScreenNote::Unreadable: return text(T::NoteUnreadable, l);
        case ScreenNote::AllOff: return text(T::NoteAllOff, l);
    }
    return "";
}

/// A note with what goes with it (`File: name.ncd`, `Stop refused: unauthorized`); the detail is cut when it is empty.
inline std::string note_with(ScreenNote n, const std::string& detail, Lang l) { return detail.empty() ? std::string(note_text(n, l)) : std::string(note_text(n, l)) + ": " + detail; }

/// The reason the workflow gave, in the language of the operator; a reason that is the server's own words (or any other text) is shown as it is.
inline std::string reason_text(const std::string& why, Lang l) {
    if (why == reasons::kNotAJobFile) return text(T::WfKind, l);
    if (why == reasons::kEmptyFile) return text(T::WfEmpty, l);
    if (why == reasons::kOldServerNoFile) return text(T::WfOldNoFile, l);
    if (why == reasons::kNotStopped || why == reasons::kNothingToGoOn) return text(T::WfNothing, l);
    return why;
}

// ---- the question of the gate --------------------------------------------------------------------------------------------------------------------

namespace detail {
inline bool starts_with(const std::string& s, const char* p) { return s.compare(0, std::char_traits<char>::length(p), p) == 0; }
}  // namespace detail

/// What the gate asks, in the language of the operator: the order itself ("Stop the machine", "Start the job", "Jog X -", "Spindle 12000 rpm"). The ids of the actions
/// ("start job", "jog X -") are the code's; an id that is not known is shown as it is.
inline std::string order_text(const Action& a, Lang l) {
    const std::string& w = a.what;
    if (w == "stop") return text(T::OrdStop, l);
    if (w == "start job") return text(T::OrdStart, l);
    if (w == "pause") return text(T::OrdPause, l);
    if (w == "resume job") return text(T::OrdResume, l);
    if (w == "tool changed") return text(T::OrdToolChanged, l);
    if (w == "home") return text(T::OrdHome, l);
    if (detail::starts_with(w, "jog X")) return std::string(text(T::JogX, l)) + w.substr(5);
    if (detail::starts_with(w, "jog Y")) return std::string(text(T::JogY, l)) + w.substr(5);
    if (detail::starts_with(w, "jog Z")) return std::string(text(T::JogZ, l)) + w.substr(5);
    if (detail::starts_with(w, "spindle on")) return std::string(text(T::Spindle, l)) + w.substr(10);
    return w;
}

/// The seconds a question still waits, rounded up, so the operator never reads 0 s while it is still there.
inline std::uint32_t countdown_s(std::uint32_t remaining_ms) { return (remaining_ms + 999u) / 1000u; }

/// `Cancels in 5 s`; empty when nothing waits.
inline std::string countdown_text(std::uint32_t remaining_ms, Lang l) {
    if (remaining_ms == 0) return "";
    return std::string(text(T::CancelsIn, l)) + " " + std::to_string(countdown_s(remaining_ms)) + " s";
}

// ---- the version ---------------------------------------------------------------------------------------------------------------------------------

/// `Firmware 0.0.5`
inline std::string firmware_label(const std::string& version, Lang l) { return std::string(text(T::Firmware, l)) + " " + version; }

}  // namespace hmi
