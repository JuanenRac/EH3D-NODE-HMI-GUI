// EH3D-NODE-HMI-GUI - core/notices.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The notices of the screen that are said by a recorded voice: one when the job ends, one when the machine asks for a tool and two for the visual check of the board with the camera
// of the drill head (correct and not correct). A notice is set as the desktop program
// sets it (`off`, `default` or `voice:LANG:f|m`), and the recordings live in the folder `voices` of the card or of the flash, one folder for each language and each
// gender: `voices/es/f/finished.wav`, `voices/es/f/tool.wav`, `voices/es/f/check_ok.wav`, `voices/es/f/check_bad.wav`, `voices/es/f/stage.wav` (the drilling is done and the routing comes next), and the same under `m`. This says which file a setting names; playing it is the job of the board.
#pragma once

#include <string>

#include "lang.hpp"

namespace hmi {

enum class Cue { Finished, ToolChange, CheckOk, CheckNotOk, Stage };

struct NoticeSetting {
    enum class Kind { Off, Tone, Voice } kind = Kind::Off;
    Lang lang = Lang::En;
    bool female = true;
};

/// Reads `off`, `default` or `voice:LANG:f|m` (LANG: the code of a language, `es`, `pt-br`...). False for anything else (the setting then keeps its default).
inline bool parse_notice(const std::string& text, NoticeSetting& out) {
    if (text == "off") {
        out = NoticeSetting{};
        return true;
    }
    if (text == "default") {
        out = NoticeSetting{NoticeSetting::Kind::Tone, Lang::En, true};
        return true;
    }
    // voice:LANG:f|m, where LANG is the code of any language of the screen (the folder of a language that has no recordings is simply not there)
    if (text.compare(0, 6, "voice:") != 0 || text.size() < 10 || text[text.size() - 2] != ':' || (text.back() != 'f' && text.back() != 'm')) return false;
    Lang l;
    if (!lang_from_code(text.substr(6, text.size() - 8), l)) return false;
    out = NoticeSetting{NoticeSetting::Kind::Voice, l, text.back() == 'f'};
    return true;
}

inline std::string notice_text(const NoticeSetting& s) {
    switch (s.kind) {
        case NoticeSetting::Kind::Off: return "off";
        case NoticeSetting::Kind::Tone: return "default";
        case NoticeSetting::Kind::Voice: return std::string("voice:") + lang_code(s.lang) + ":" + (s.female ? "f" : "m");
    }
    return "off";
}

/// The file that a setting names for a cue, below `root` (`/sdcard` or `/config`); empty for a setting that is not a voice.
inline std::string notice_file(const std::string& root, const NoticeSetting& s, Cue cue) {
    if (s.kind != NoticeSetting::Kind::Voice) return "";
    const char* name = "tool.wav";
    switch (cue) {
        case Cue::Finished: name = "finished.wav"; break;
        case Cue::ToolChange: name = "tool.wav"; break;
        case Cue::CheckOk: name = "check_ok.wav"; break;
        case Cue::CheckNotOk: name = "check_bad.wav"; break;
        case Cue::Stage: name = "stage.wav"; break;
    }
    return root + "/voices/" + lang_code(s.lang) + "/" + (s.female ? "f" : "m") + "/" + name;
}

}  // namespace hmi
