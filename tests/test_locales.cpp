// EH3D-NODE-HMI-GUI host tests: the texts of the screen in the thirty-seven languages. Every key has a text in every language (the two variants only what changes), nothing
// is empty or too long for the screen, what must stay stays (the name hmi.cfg, the three dots), the script of each language is the right one, and every character that
// any text can draw is in the range of the fonts of the screen (core/font_ranges.hpp, made by tools/make_fonts.py).
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include "../core/font_ranges.hpp"
#include "../core/keyboard.hpp"
#include "../core/quality.hpp"
#include "../core/ui_text.hpp"

using namespace hmi;

static int failures = 0;
static int checks = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        ++checks;                                                       \
        if (!(cond)) {                                                  \
            ++failures;                                                 \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
        }                                                               \
    } while (0)

#define CHECK_MSG(cond, ...)                                            \
    do {                                                                \
        ++checks;                                                       \
        if (!(cond)) {                                                  \
            ++failures;                                                 \
            std::printf("FAIL %s:%d  ", __FILE__, __LINE__);            \
            std::printf(__VA_ARGS__);                                   \
            std::printf("\n");                                          \
        }                                                               \
    } while (0)

// The code points of an UTF-8 text; false when the text is not well formed.
static bool decode(const char* s, std::vector<unsigned>& out) {
    out.clear();
    const unsigned char* p = reinterpret_cast<const unsigned char*>(s);
    while (*p) {
        unsigned cp;
        int more;
        if (*p < 0x80) { cp = *p; more = 0; }
        else if ((*p & 0xE0) == 0xC0) { cp = *p & 0x1F; more = 1; }
        else if ((*p & 0xF0) == 0xE0) { cp = *p & 0x0F; more = 2; }
        else if ((*p & 0xF8) == 0xF0) { cp = *p & 0x07; more = 3; }
        else return false;
        ++p;
        for (int i = 0; i < more; ++i, ++p) {
            if ((*p & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (*p & 0x3F);
        }
        out.push_back(cp);
    }
    return true;
}

static bool in_font(unsigned cp) {
    for (std::size_t i = 0; i < kFontRangeCount; ++i) {
        if (kFontRanges[i].lo <= cp && cp <= kFontRanges[i].hi) return true;
    }
    return false;
}

struct Entry {
    std::size_t key;
    const char* text;
};

// Every text of the screen in a language, with its key.
static std::vector<Entry> all_texts(Lang l) {
    std::vector<Entry> v;
    for (std::size_t i = 0; i < static_cast<std::size_t>(T::Count); ++i) v.push_back({i, text(static_cast<T>(i), l)});
    for (std::size_t i = 0; i < static_cast<std::size_t>(Q::Count); ++i) v.push_back({kUiKeyCount + i, quality_text(static_cast<Q>(i), l)});
    const auto& table = finding_table();
    for (std::size_t i = 0; i < table.size(); ++i) v.push_back({kUiKeyCount + kQualityKeyCount + i, finding_text(table[i].code, l)});
    return v;
}

static bool count_is(std::size_t a, std::size_t b) { return a == b; }

static void the_keys() {
    CHECK(count_is(kKeyCount, 146));  // (the enums are tied to the counts by static_assert in ui_text.hpp and quality.hpp)
    // the order of the names follows the enums: every name checked against the enum it stands for
    const char* ui[] = {"Viewer", "Job", "Machine", "Settings", "Fleet", "Start", "Pause", "Stop", "Home", "Confirm", "Cancel", "Armed", "NotArmed", "StopPressed", "AckRelease", "Language", "Voice",
                        "Brightness", "Volume", "JogX", "JogY", "JogZ", "Spindle", "AskMove", "AskSpindle", "WhyHardwareStop", "WhyNotArmed", "WhyVoiceOff", "WhyNoStop", "WhySpindle", "WhyNoSpindleHere", "Feed",
                        "Progress", "Tool", "Jog", "Holes", "Cuts", "SpindleCut", "WhySpindleFast", "Resume", "Stopped", "Next", "ResumedFrom", "ToolChanged", "BoardTooBig", "Failed",
                        "OrdStart", "OrdPause", "OrdResume", "OrdToolChanged", "OrdHome", "OrdStop", "CancelsIn", "LinkOk", "LinkLost", "LinkOther", "LinkOld", "Firmware", "CfgLine", "CfgMore", "CfgNoFile", "CfgNotPair", "CfgRange", "CfgOnOff", "CfgChoice", "CfgToken", "CfgUnknown", "SrvNone", "SrvSilent", "SrvOther", "NoteStopFail", "NoteStopRefused", "NoteStopSent", "NoteNoJobToStop", "NoteMoveNoLink", "NoteBusy", "NoteNothingToSend", "NoteOldServer", "NoteOk", "NoteFollowing", "NoteNoJobReady", "NoteGoingOn", "NoteFile", "NoteUnreadable", "NoteAllOff", "ErrClient", "ErrTooLong", "ErrBroke", "ErrSend", "WfKind", "WfEmpty", "WfOldNoFile", "WfNothing", "OrientAuto", "OrientLandscape", "OrientPortrait", "OverClip", "OverRefuse",
                        "Tools", "TimeLeft", "EstTime", "AtcAuto", "AtcByHand", "PutTool", "WarnTiny", "WarnFar", "WarnScale", "WarnCourse", "WfRunning"};
    const T ui_enum[] = {T::Viewer, T::Job, T::Machine, T::Settings, T::Fleet, T::Start, T::Pause, T::Stop, T::Home, T::Confirm, T::Cancel, T::Armed, T::NotArmed, T::StopPressed, T::AckRelease,
                         T::Language, T::Voice, T::Brightness, T::Volume, T::JogX, T::JogY, T::JogZ, T::Spindle, T::AskMove, T::AskSpindle, T::WhyHardwareStop, T::WhyNotArmed, T::WhyVoiceOff,
                         T::WhyNoStop, T::WhySpindle, T::WhyNoSpindleHere, T::Feed, T::Progress, T::Tool, T::Jog, T::Holes, T::Cuts, T::SpindleCut, T::WhySpindleFast, T::Resume, T::Stopped, T::Next, T::ResumedFrom, T::ToolChanged, T::BoardTooBig, T::Failed,
                         T::OrdStart, T::OrdPause, T::OrdResume, T::OrdToolChanged, T::OrdHome, T::OrdStop, T::CancelsIn, T::LinkOk, T::LinkLost, T::LinkOther, T::LinkOld, T::Firmware, T::CfgLine, T::CfgMore, T::CfgNoFile, T::CfgNotPair, T::CfgRange, T::CfgOnOff, T::CfgChoice, T::CfgToken, T::CfgUnknown, T::SrvNone, T::SrvSilent, T::SrvOther, T::NoteStopFail, T::NoteStopRefused, T::NoteStopSent, T::NoteNoJobToStop, T::NoteMoveNoLink, T::NoteBusy, T::NoteNothingToSend, T::NoteOldServer, T::NoteOk, T::NoteFollowing, T::NoteNoJobReady, T::NoteGoingOn, T::NoteFile, T::NoteUnreadable, T::NoteAllOff, T::ErrClient, T::ErrTooLong, T::ErrBroke, T::ErrSend, T::WfKind, T::WfEmpty, T::WfOldNoFile, T::WfNothing, T::OrientAuto, T::OrientLandscape, T::OrientPortrait, T::OverClip, T::OverRefuse,
                         T::Tools, T::TimeLeft, T::EstTime, T::AtcAuto, T::AtcByHand, T::PutTool, T::WarnTiny, T::WarnFar, T::WarnScale, T::WarnCourse, T::WfRunning};
    CHECK(count_is(sizeof ui / sizeof ui[0], kUiKeyCount));
    for (std::size_t i = 0; i < kUiKeyCount; ++i) {
        CHECK_MSG(static_cast<std::size_t>(ui_enum[i]) == i, "ui enum order at %zu", i);
        CHECK_MSG(std::string(kKeyNames[i]) == std::string("ui.") + ui[i], "ui key name %zu: %s", i, kKeyNames[i]);
    }
    const char* q[] = {"Title", "Light", "LightOff", "CheckNow", "Checking", "VerdictOk", "VerdictNotOk", "VerdictUnknown", "NotChecked", "HoleWidth", "HoleNone", "Profile", "Standard", "FinePitch",
                       "Prototype", "Calibrate", "HolesOk", "ChannelWidth", "NeedServer"};
    const Q q_enum[] = {Q::Title, Q::Light, Q::LightOff, Q::CheckNow, Q::Checking, Q::VerdictOk, Q::VerdictNotOk, Q::VerdictUnknown, Q::NotChecked, Q::HoleWidth, Q::HoleNone, Q::Profile,
                        Q::Standard, Q::FinePitch, Q::Prototype, Q::Calibrate, Q::HolesOk, Q::ChannelWidth, Q::NeedServer};
    CHECK(count_is(sizeof q / sizeof q[0], kQualityKeyCount));
    for (std::size_t i = 0; i < kQualityKeyCount; ++i) {
        CHECK_MSG(static_cast<std::size_t>(q_enum[i]) == i, "quality enum order at %zu", i);
        CHECK_MSG(std::string(kKeyNames[kUiKeyCount + i]) == std::string("q.") + q[i], "quality key name %zu: %s", i, kKeyNames[kUiKeyCount + i]);
    }
    const auto& table = finding_table();
    for (std::size_t i = 0; i < kFindingKeyCount; ++i) {
        CHECK_MSG(std::string(kKeyNames[kUiKeyCount + kQualityKeyCount + i]) == std::string("f.") + table[i].code, "finding key name %zu", i);
    }
    // two keys of different tables are never mixed up: the same key, the same text as the table of its language
    CHECK(std::string(text(T::Start, Lang::Pl)) != "" && std::string(quality_text(Q::NeedServer, Lang::Pl)).find("hmi.cfg") != std::string::npos);
}

static void the_languages() {
    std::set<std::string> codes, names;
    for (std::size_t i = 0; i < kLangCount; ++i) {
        const Lang l = static_cast<Lang>(i);
        CHECK(codes.insert(lang_code(l)).second);
        CHECK(names.insert(lang_name(l)).second);  // two languages never show the same name in the selector
        CHECK(std::strlen(lang_name(l)) > 0);
        Lang back;
        CHECK(lang_from_code(lang_code(l), back) && back == l);
    }
    CHECK(codes.size() == 37 && std::string(lang_code(Lang::PtBr)) == "pt-br" && std::string(lang_code(Lang::FrCa)) == "fr-ca" && std::string(lang_code(Lang::Sr)) == "sr");
    Lang none;
    CHECK(!lang_from_code("xx", none) && !lang_from_code("", none) && !lang_from_code("pt-pt", none));
    CHECK(lang_fallback(Lang::PtBr) == Lang::Pt && lang_fallback(Lang::FrCa) == Lang::Fr && lang_fallback(Lang::Pl) == Lang::En && lang_fallback(Lang::Ja) == Lang::Ja);
}

static bool short_label(std::size_t key) {
    const std::string name = kKeyNames[key];
    if (name.compare(0, 2, "f.") == 0) return false;
    const char* sentences[] = {"ui.Why", "ui.Ask", "ui.StopPressed", "ui.AckRelease", "ui.SpindleCut", "ui.BoardTooBig", "q.NeedServer", "q.HoleWidth", "q.Title", "q.Calibrate",
                              "ui.Ord", "ui.Cfg", "ui.Srv", "ui.Note", "ui.Err", "ui.Wf", "ui.Over", "ui.CancelsIn", "ui.Atc", "ui.Warn"};  // (the last ones fill a whole line)
    for (const char* s : sentences) {
        if (name.compare(0, std::strlen(s), s) == 0) return false;
    }
    return true;
}

static void every_text() {
    for (std::size_t i = kBaseLangCount; i < kLangCount; ++i) {
        const Lang l = static_cast<Lang>(i);
        const bool variant = (l == Lang::PtBr || l == Lang::FrCa);
        std::size_t translated_count = 0;
        for (std::size_t k = 0; k < kKeyCount; ++k) {
            const char* own = locale_text(l, k);
            if (own) ++translated_count;
            if (!variant) CHECK_MSG(own != nullptr, "%s lacks %s", lang_code(l), kKeyNames[k]);
        }
        if (variant) CHECK_MSG(translated_count > 0 && translated_count < kKeyCount, "%s must hold only what changes", lang_code(l));
        for (const Entry& e : all_texts(l)) {
            std::vector<unsigned> cps;
            CHECK_MSG(e.text != nullptr && decode(e.text, cps) && !cps.empty(), "%s %s is empty or not UTF-8", lang_code(l), kKeyNames[e.key]);
            if (e.text == nullptr) continue;
            decode(e.text, cps);
            CHECK_MSG(cps.size() <= (short_label(e.key) ? 28u : 90u), "%s %s is too long for the screen (%zu characters)", lang_code(l), kKeyNames[e.key], cps.size());
            for (unsigned cp : cps) {
                CHECK_MSG(cp >= 0x20 && cp != '%' && cp != 0x7F, "%s %s has a control or format character", lang_code(l), kKeyNames[e.key]);
                CHECK_MSG(in_font(cp), "%s %s: U+%04X is not in the font", lang_code(l), kKeyNames[e.key], cp);
            }
            // what must stay whatever the language says
            if (std::string(kKeyNames[e.key]) == "q.NeedServer") CHECK_MSG(std::string(e.text).find("hmi.cfg") != std::string::npos, "%s NeedServer lost hmi.cfg", lang_code(l));
            if (std::string(kKeyNames[e.key]) == "q.Checking") CHECK_MSG(std::string(e.text).find("...") != std::string::npos, "%s Checking lost the dots", lang_code(l));
            if (std::string(kKeyNames[e.key]) == "ui.JogX") CHECK_MSG(std::string(e.text).find('X') != std::string::npos, "%s JogX lost the axis", lang_code(l));
        }
    }
    // the variants fall back to the language they vary, the others to English
    CHECK(std::string(text(T::Pause, Lang::FrCa)) == text(T::Pause, Lang::Fr));
    CHECK(std::string(text(T::Settings, Lang::FrCa)) == "Paramètres");
    CHECK(std::string(text(T::Viewer, Lang::PtBr)) == text(T::Viewer, Lang::Pt));
    CHECK(std::string(text(T::Settings, Lang::PtBr)) != text(T::Settings, Lang::Pt));  // Definições / Configurações
    // the eight keep their own tables
    CHECK(std::string(text(T::Start, Lang::Es)) == "Iniciar" && std::string(text(T::Start, Lang::Ja)) == "開始" && std::string(text(T::Start, Lang::En)) == "Start");
    CHECK(std::string(finding_text("hole_missing", Lang::De)) == "keine Bohrung" && std::string(finding_text("unknown_code", Lang::Pl)) == "unknown_code");
}

// A text in the script of its language, not a copy of the English one.
static void the_scripts() {
    struct S {
        Lang l;
        unsigned lo, hi;
    };
    const S scripts[] = {{Lang::Bg, 0x400, 0x4FF}, {Lang::Ru, 0x400, 0x4FF}, {Lang::Uk, 0x400, 0x4FF}, {Lang::Sr, 0x400, 0x4FF}, {Lang::El, 0x370, 0x3FF}};
    for (const S& s : scripts) {
        std::size_t native = 0, letters = 0;
        for (const Entry& e : all_texts(s.l)) {
            std::vector<unsigned> cps;
            decode(e.text, cps);
            for (unsigned cp : cps) {
                if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z')) ++letters;
                else if (cp >= s.lo && cp <= s.hi) ++native;
            }
        }
        // the Latin letters left are the few that stay (hmi.cfg, mm, stop, the axes)
        CHECK_MSG(native > 8 * letters, "%s is mostly not in its script (%zu native, %zu Latin)", lang_code(s.l), native, letters);
    }
    // the others write Latin with their own letters
    const struct {
        Lang l;
        const char* letter;
    } marks[] = {{Lang::Pl, "ł"}, {Lang::Cs, "ř"}, {Lang::Hu, "ő"}, {Lang::Ro, "ă"}, {Lang::Mt, "ħ"}, {Lang::Lv, "ā"}, {Lang::Lt, "ė"}, {Lang::Is, "ð"}};
    for (const auto& m : marks) {
        bool found = false;
        for (const Entry& e : all_texts(m.l)) found = found || std::string(e.text).find(m.letter) != std::string::npos;
        CHECK_MSG(found, "%s never uses %s", lang_code(m.l), m.letter);
    }
}

static void the_font() {
    std::set<unsigned> used;
    std::vector<unsigned> cps;
    for (std::size_t i = 0; i < kLangCount; ++i) {
        const Lang l = static_cast<Lang>(i);
        for (const Entry& e : all_texts(l)) {
            decode(e.text, cps);
            used.insert(cps.begin(), cps.end());
        }
        decode(lang_name(l), cps);
        used.insert(cps.begin(), cps.end());
        for (const std::string& row : letter_rows(l)) {
            decode(row.c_str(), cps);
            used.insert(cps.begin(), cps.end());
        }
    }
    std::size_t lost = 0;
    for (unsigned cp : used) {
        if (!in_font(cp)) {
            ++lost;
            std::printf("not in the font: U+%04X\n", cp);
        }
    }
    CHECK(lost == 0);
    CHECK(used.size() > 300);  // the languages really use many characters, so the range is not trivial
    CHECK(kFontRangeCount > 20 && in_font('A') && in_font(0x4E2D) && in_font(0x3042) && in_font(0x411) && in_font(0x3B1) && in_font(0x142));
    CHECK(!in_font(0x0627) && !in_font(0x0E01));  // Arabic and Thai are not offered
    unsigned previous_end = 0;
    for (std::size_t i = 0; i < kFontRangeCount; ++i) {  // the ranges are sorted and apart
        CHECK(kFontRanges[i].lo <= kFontRanges[i].hi && (i == 0 || kFontRanges[i].lo > previous_end + 1));
        previous_end = kFontRanges[i].hi;
    }
}

int main() {
    the_keys();
    the_languages();
    every_text();
    the_scripts();
    the_font();
    std::printf("locales: %d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
