// EH3D-NODE-HMI-GUI - core/hmi_config.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// Every tuned value of the screen lives in one settings file (hmi.cfg), never in the code. The file is `key = value` lines with `#`
// comments; a value out of range or a key nobody knows is reported and the default stays, so a typing mistake never opens a safety switch.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "lang.hpp"
#include "machine.hpp"
#include "notices.hpp"
#include "orientation.hpp"

namespace hmi {

/// What the gate does with a spindle order that asks for more turns than the spindle gives: cut it to the top speed and say so (the default), or refuse it.
enum class SpindleOver { Clip, Refuse };

struct Settings {
    // the screen
    Lang language = Lang::En;
    std::string theme = "forge";
    Orientation orientation = Orientation::Auto;  // auto, landscape or portrait
    std::uint16_t brightness_percent = 80;  // 10..100
    std::uint32_t idle_dim_s = 120;         // 0 (never) .. 3600

    // the kind of machine: what the screen may order on it (a CNC machine has a spindle and the camera of the drill head, a laser and a pick and place machine do not)
    MachineKind machine = MachineKind::Cnc;

    // where the machine is: the EH3D server (or the Bungard node) the screen controls
    std::string server_host;                // empty: nothing is controlled yet
    std::uint16_t server_port = 18200;      // 1..65535
    std::string server_machine;             // the machine of the server this screen controls
    std::string token;                      // write-only: never shown, never logged
    std::uint32_t poll_s = 3;               // how often the screen asks the server for the fleet, 1..60
    std::uint32_t table_width_mm = 0;       // the table of the machine, for the warning of a board that does not fit: 0 = the one the server says, 0..5000
    std::uint32_t table_depth_mm = 0;       // (the course of the machine less the offset); both are used only when the server does not say it

    // the safety switches; every default is the conservative one
    bool confirm_moves = true;              // a move or a spindle order asks for a confirmation on the screen
    std::uint32_t confirm_timeout_ms = 8000;  // 1000..60000
    std::uint32_t jog_max_mm = 20;          // the longest single jog the screen sends, 1..200
    std::uint32_t spindle_max_rpm = 24000;  // the highest speed the screen asks for, 0..65535 (0: the screen never starts the spindle)
    std::uint32_t spindle_rpm = 12000;     // the turns the spindle button asks for, 0..65535 (0: the speed the job says); the gate cuts it to what the spindle gives
    SpindleOver spindle_over = SpindleOver::Clip;  // clip: an order for more turns than the spindle gives is cut to its top speed and the screen says so; refuse: it is refused
    bool hw_stop_required = true;           // the screen controls the machine only while the hardware stop input is wired and released
    std::uint32_t link_idle_ms = 3000;      // the link is lost when the machine stays quiet this long, 200..60000
    std::uint32_t stale_after_s = 15;       // the data shown (the fleet, the progress) are marked old when the server has not updated them for this long, 2..3600

    // voice
    bool voice_alerts = true;               // spoken alerts (the end of a job, a tool change, an error)
    bool voice_orders = false;              // spoken orders; off until the operator turns it on
    bool voice_confirm = false;             // a spoken "yes" confirms a move; off: only a touch on the screen confirms
    std::uint16_t volume_percent = 60;      // 0..100
    std::uint16_t mic_gain_percent = 50;    // 0..100
    NoticeSetting sound_finish;             // the notice when a job ends: off, the tone or a recorded voice (voice:es:f)
    NoticeSetting sound_tool_change;        // the notice when the machine asks for a tool
    NoticeSetting sound_check_ok;           // CNC only: the notice when the visual check of the board (the camera of the drill head) is correct
    NoticeSetting sound_check_bad;          // and when it is not correct
};

/// Why a line of the settings file cannot be used: the screen says it in the language of the operator (the English `what` is for the log).
enum class CfgReason { NotPair, Range, OnOff, Choice, Token, Unknown };

struct Problem {
    int line = 0;
    std::string key;
    std::string what;
    CfgReason reason = CfgReason::Unknown;
};

namespace detail {

inline std::string trim(const std::string& s) {
    std::size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r')) --b;
    return s.substr(a, b - a);
}

inline bool to_uint(const std::string& v, std::uint32_t lo, std::uint32_t hi, std::uint32_t& out) {
    if (v.empty() || v.size() > 10) return false;
    std::uint64_t n = 0;
    for (char c : v) {
        if (c < '0' || c > '9') return false;
        n = n * 10 + static_cast<unsigned>(c - '0');
    }
    if (n < lo || n > hi) return false;
    out = static_cast<std::uint32_t>(n);
    return true;
}

inline bool to_bool(const std::string& v, bool& out) {
    if (v == "on" || v == "true" || v == "yes" || v == "1") { out = true; return true; }
    if (v == "off" || v == "false" || v == "no" || v == "0") { out = false; return true; }
    return false;
}

}  // namespace detail

/// Reads a settings file. The result starts from the defaults; every line that cannot be used is listed in `problems` and changes nothing.
inline Settings parse_settings(const std::string& text, std::vector<Problem>& problems) {
    Settings s;
    std::size_t pos = 0;
    int line_no = 0;
    while (pos <= text.size()) {
        std::size_t end = text.find('\n', pos);
        if (end == std::string::npos) end = text.size();
        std::string line = detail::trim(text.substr(pos, end - pos));
        pos = end + 1;
        ++line_no;
        if (line.empty() || line[0] == '#') continue;
        std::size_t eq = line.find('=');
        if (eq == std::string::npos) {
            problems.push_back({line_no, line, "not a key = value line", CfgReason::NotPair});
            continue;
        }
        std::string key = detail::trim(line.substr(0, eq));
        std::string val = detail::trim(line.substr(eq + 1));
        std::uint32_t n = 0;
        bool b = false;
        auto bad = [&](const char* why, CfgReason reason = CfgReason::Choice) { problems.push_back({line_no, key, why, reason}); };
        auto num = [&](std::uint32_t lo, std::uint32_t hi) {
            if (detail::to_uint(val, lo, hi, n)) return true;
            bad("out of range or not a number", CfgReason::Range);
            return false;
        };
        auto flag = [&]() {
            if (detail::to_bool(val, b)) return true;
            bad("expected on or off", CfgReason::OnOff);
            return false;
        };
        if (key == "language") {
            Lang l;
            if (lang_from_code(val, l)) s.language = l; else bad("not one of the language codes of the screen (en, es, it, de, fr, pt, zh, ja, pl, ru, ...)");
        } else if (key == "theme") {
            s.theme = val;
        } else if (key == "orientation") {
            Orientation o;
            if (orientation_from_text(val, o)) s.orientation = o; else bad("not auto, landscape or portrait");
        } else if (key == "machine_kind") {
            MachineKind k;
            if (machine_kind_from_text(val, k)) s.machine = k; else bad("not cnc, laser or pnp");
        } else if (key == "poll_s") {
            if (num(1, 60)) s.poll_s = n;
        } else if (key == "table_width_mm") {
            if (num(0, 5000)) s.table_width_mm = n;
        } else if (key == "table_depth_mm") {
            if (num(0, 5000)) s.table_depth_mm = n;
        } else if (key == "brightness_percent") {
            if (num(10, 100)) s.brightness_percent = static_cast<std::uint16_t>(n);
        } else if (key == "idle_dim_s") {
            if (num(0, 3600)) s.idle_dim_s = n;
        } else if (key == "server_host") {
            s.server_host = val;
        } else if (key == "server_port") {
            if (num(1, 65535)) s.server_port = static_cast<std::uint16_t>(n);
        } else if (key == "server_machine") {
            s.server_machine = val;
        } else if (key == "token") {
            if (val.empty() || (val.size() >= 8 && val.size() <= 64)) s.token = val; else bad("8 to 64 characters", CfgReason::Token);
        } else if (key == "confirm_moves") {
            if (flag()) s.confirm_moves = b;
        } else if (key == "confirm_timeout_ms") {
            if (num(1000, 60000)) s.confirm_timeout_ms = n;
        } else if (key == "jog_max_mm") {
            if (num(1, 200)) s.jog_max_mm = n;
        } else if (key == "spindle_max_rpm") {
            if (num(0, 65535)) s.spindle_max_rpm = n;
        } else if (key == "spindle_rpm") {
            if (num(0, 65535)) s.spindle_rpm = n;
        } else if (key == "spindle_over") {
            if (val == "clip") s.spindle_over = SpindleOver::Clip;
            else if (val == "refuse") s.spindle_over = SpindleOver::Refuse;
            else bad("not clip or refuse");
        } else if (key == "hw_stop_required") {
            if (flag()) s.hw_stop_required = b;
        } else if (key == "link_idle_ms") {
            if (num(200, 60000)) s.link_idle_ms = n;
        } else if (key == "stale_after_s") {
            if (num(2, 3600)) s.stale_after_s = n;
        } else if (key == "voice_alerts") {
            if (flag()) s.voice_alerts = b;
        } else if (key == "voice_orders") {
            if (flag()) s.voice_orders = b;
        } else if (key == "voice_confirm") {
            if (flag()) s.voice_confirm = b;
        } else if (key == "volume_percent") {
            if (num(0, 100)) s.volume_percent = static_cast<std::uint16_t>(n);
        } else if (key == "sound_finish") {
            if (!parse_notice(val, s.sound_finish)) bad("not off, default or voice:LANG:f|m");
        } else if (key == "sound_tool_change") {
            if (!parse_notice(val, s.sound_tool_change)) bad("not off, default or voice:LANG:f|m");
        } else if (key == "sound_check_ok") {
            if (!parse_notice(val, s.sound_check_ok)) bad("not off, default or voice:LANG:f|m");
        } else if (key == "sound_check_bad") {
            if (!parse_notice(val, s.sound_check_bad)) bad("not off, default or voice:LANG:f|m");
        } else if (key == "mic_gain_percent") {
            if (num(0, 100)) s.mic_gain_percent = static_cast<std::uint16_t>(n);
        } else {
            bad("unknown key", CfgReason::Unknown);
        }
    }
    return s;
}

}  // namespace hmi
