// EH3D-NODE-HMI-GUI - core/locales.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The texts of the screen in the twenty-nine languages that are not in the tables of eight. Every text of the screen has a key (the names below, in the order of the enums T of
// ui_text.hpp and Q of quality.hpp, then the words of the findings of the analyser); the files of the folder `locales` give one text per key and language and
// tools/make_locales.py compiles them into locales_gen.hpp. A key that a file lacks is shown in English (see lang.hpp).
#pragma once

#include <cstddef>

#include "lang.hpp"

namespace hmi {

constexpr std::size_t kUiKeyCount = 44;
constexpr std::size_t kQualityKeyCount = 19;
constexpr std::size_t kFindingKeyCount = 18;
constexpr std::size_t kKeyCount = kUiKeyCount + kQualityKeyCount + kFindingKeyCount;

/// The name of every key, as the files of `locales` write it: `ui.` for T, `q.` for Q and `f.` for the code of a finding.
inline constexpr const char* const kKeyNames[kKeyCount] = {
    "ui.Viewer", "ui.Job", "ui.Machine", "ui.Settings", "ui.Fleet", "ui.Start", "ui.Pause", "ui.Stop", "ui.Home", "ui.Confirm", "ui.Cancel", "ui.Armed", "ui.NotArmed",
    "ui.StopPressed", "ui.AckRelease", "ui.Language", "ui.Voice", "ui.Brightness", "ui.Volume", "ui.JogX", "ui.JogY", "ui.JogZ", "ui.Spindle", "ui.AskMove", "ui.AskSpindle",
    "ui.WhyHardwareStop", "ui.WhyNotArmed", "ui.WhyVoiceOff", "ui.WhyNoStop", "ui.WhySpindle", "ui.WhyNoSpindleHere", "ui.Feed", "ui.Progress", "ui.Tool", "ui.Jog", "ui.Holes", "ui.Cuts", "ui.SpindleCut", "ui.WhySpindleFast", "ui.Resume", "ui.Stopped", "ui.Next", "ui.ResumedFrom", "ui.ToolChanged",
    "q.Title", "q.Light", "q.LightOff", "q.CheckNow", "q.Checking", "q.VerdictOk", "q.VerdictNotOk", "q.VerdictUnknown", "q.NotChecked", "q.HoleWidth", "q.HoleNone", "q.Profile",
    "q.Standard", "q.FinePitch", "q.Prototype", "q.Calibrate", "q.HolesOk", "q.ChannelWidth", "q.NeedServer",
    "f.hole_missing", "f.hole_wrong_size", "f.hole_off_centre", "f.hole_not_round", "f.hole_debris", "f.hole_out_of_view", "f.mill_no_channels", "f.mill_too_narrow", "f.mill_too_wide",
    "f.mill_pinched", "f.mill_uneven", "f.image_too_dark", "f.image_too_bright", "f.image_low_contrast", "f.image_blurry", "f.not_calibrated", "f.nothing_to_check", "f.no_expected_holes",
};

}  // namespace hmi

#include "locales_gen.hpp"

namespace hmi {

/// The text of a key in a language of the twenty-nine; null for the eight of the tables and for a key that the file of the language lacks.
inline const char* locale_text(Lang l, std::size_t key) {
    const std::size_t i = static_cast<std::size_t>(l);
    if (i < kBaseLangCount || i >= kLangCount || key >= kKeyCount) return nullptr;
    return gen::kColumns[i - kBaseLangCount][key];
}

/// The name of a language in itself, for the selector of the settings (English, Español, ... Polski, Српски).
inline const char* lang_name(Lang l) { return gen::kLangNames[static_cast<std::size_t>(l)]; }

/// A text of the screen: the one of the file of the language when it has it, else the one of the table of eight for the language it falls back to.
inline const char* translated(Lang l, std::size_t key, const Row& row) {
    if (const char* s = locale_text(l, key)) return s;
    return pick(row, l);
}

}  // namespace hmi
