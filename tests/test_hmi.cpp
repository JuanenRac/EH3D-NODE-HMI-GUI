// EH3D-NODE-HMI-GUI - host tests of the settings file, the gate between the operator and the machine, and the voice.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../core/ui_text.hpp"
#include "../core/voice.hpp"

static int g_checks = 0;
static int g_failed = 0;
#define CHECK(c)                                                              \
    do {                                                                      \
        ++g_checks;                                                           \
        if (!(c)) {                                                           \
            ++g_failed;                                                       \
            std::printf("FAILED line %d: %s\n", __LINE__, #c);               \
        }                                                                     \
    } while (0)

using namespace hmi;

static void settings_file() {
    std::vector<Problem> p;
    Settings d = parse_settings("", p);
    CHECK(p.empty());
    // every safety default is the conservative one
    CHECK(d.confirm_moves);
    CHECK(d.hw_stop_required);
    CHECK(!d.voice_orders);
    CHECK(!d.voice_confirm);
    CHECK(d.voice_alerts);
    CHECK(d.jog_max_mm == 20);

    Settings s = parse_settings("# comment\nlanguage = pt\nconfirm_timeout_ms = 5000\nvoice_orders = on\ntoken = abcdefgh\n server_port=9000 \n", p);
    CHECK(p.empty());
    CHECK(s.language == Lang::Pt);
    CHECK(s.confirm_timeout_ms == 5000);
    CHECK(s.voice_orders);
    CHECK(s.token == "abcdefgh");
    CHECK(s.server_port == 9000);

    // a value that cannot be used is reported and the default stays: a typing mistake never opens a switch
    p.clear();
    Settings bad = parse_settings("confirm_moves = maybe\nconfirm_timeout_ms = 10\njog_max_mm = 500\nlanguage = xx\nvolume_percent = -3\ntoken = short\nnothing here\nflux = 1\n", p);
    CHECK(p.size() == 8);
    CHECK(bad.confirm_moves);
    CHECK(bad.confirm_timeout_ms == 8000);
    CHECK(bad.jog_max_mm == 20);
    CHECK(bad.language == Lang::En);
    CHECK(bad.volume_percent == 60);
    CHECK(bad.token.empty());
    CHECK(p[0].line == 1 && p[0].key == "confirm_moves");
    // the last line without a newline and Windows line ends are read
    p.clear();
    Settings w = parse_settings("language = de\r\nvolume_percent = 10", p);
    CHECK(p.empty() && w.language == Lang::De && w.volume_percent == 10);
    for (unsigned i = 0; i < kLangCount; ++i) {
        Lang l;
        CHECK(lang_from_code(lang_code(static_cast<Lang>(i)), l) && l == static_cast<Lang>(i));
    }
}

static Settings ready_settings() {
    Settings s;
    s.voice_orders = true;
    return s;
}

// a gate that has seen the stop wired and released, acknowledged, and the link armed
static void make_ready(Gate& g) {
    g.hardware_stop(true, false);
    CHECK(g.acknowledge_release());
    g.link(true);
}

static void gate_rules() {
    Settings s = ready_settings();
    Gate g(s);
    Action jog{Kind::Motion, "jog X +10"};
    // a screen that has just started has not seen the stop released: nothing moves
    CHECK(g.latched());
    CHECK(g.request(jog, Origin::Touch, 0).why == Why::HardwareStop);
    make_ready(g);
    CHECK(!g.latched());

    // a stop and a question always go
    CHECK(g.request({Kind::Stop, "stop"}, Origin::Voice, 0).verdict == Verdict::Run);
    CHECK(g.request({Kind::Info, "status"}, Origin::Touch, 0).verdict == Verdict::Run);

    // a move asks, and only a touch inside the time confirms it
    Decision d = g.request(jog, Origin::Touch, 1000);
    CHECK(d.verdict == Verdict::NeedConfirm && g.pending());
    Action out;
    CHECK(!g.confirm(Origin::Voice, 1500, out));  // a spoken yes does not confirm by default
    CHECK(g.pending());
    CHECK(g.confirm(Origin::Touch, 1500, out) && out.what == "jog X +10");
    CHECK(!g.pending());
    CHECK(!g.confirm(Origin::Touch, 1600, out));  // used once

    // the time is over
    g.request(jog, Origin::Touch, 1000);
    CHECK(!g.confirm(Origin::Touch, 1000 + s.confirm_timeout_ms + 1, out));
    CHECK(!g.pending());
    // the clock may wrap around
    g.request(jog, Origin::Touch, 0xFFFFFF00u);
    CHECK(g.confirm(Origin::Touch, 100, out));

    // a new request drops the one that waited
    g.request({Kind::Motion, "first"}, Origin::Touch, 0);
    g.request({Kind::Motion, "second"}, Origin::Touch, 10);
    CHECK(g.confirm(Origin::Touch, 20, out) && out.what == "second");

    // a stop drops what waited
    g.request(jog, Origin::Touch, 0);
    g.request({Kind::Stop, "stop"}, Origin::Touch, 5);
    CHECK(!g.pending());

    // the hardware stop: pressed drops what waited and refuses everything that moves, until the operator acknowledges the release
    g.request(jog, Origin::Touch, 0);
    g.hardware_stop(true, true);
    CHECK(!g.pending() && g.latched());
    CHECK(g.request(jog, Origin::Touch, 0).why == Why::HardwareStop);
    CHECK(!g.acknowledge_release());  // the button is still down
    g.hardware_stop(true, false);
    CHECK(g.latched());  // released, but the screen does not clear it by itself
    CHECK(g.request(jog, Origin::Touch, 0).why == Why::HardwareStop);
    CHECK(g.acknowledge_release());
    CHECK(g.request(jog, Origin::Touch, 0).verdict == Verdict::NeedConfirm);
    // a stop pressed while a confirmation waits cannot be confirmed afterwards
    g.hardware_stop(true, true);
    CHECK(!g.confirm(Origin::Touch, 1, out));
    g.hardware_stop(true, false);
    g.acknowledge_release();

    // the link: no move while the machine is not armed, and a lost link drops what waited
    g.request(jog, Origin::Touch, 0);
    g.link(false);
    CHECK(!g.pending());
    CHECK(g.request(jog, Origin::Touch, 0).why == Why::NotArmed);
    g.link(true);
}

static void gate_switches() {
    // the hardware stop that is not wired: the screen does not control the machine when the setting asks for it
    Settings s = ready_settings();
    Gate g(s);
    g.hardware_stop(false, false);
    g.acknowledge_release();
    g.link(true);
    CHECK(g.request({Kind::Motion, "m"}, Origin::Touch, 0).why == Why::NoHardwareStop);
    s.hw_stop_required = false;  // a bench setting
    CHECK(g.request({Kind::Motion, "m"}, Origin::Touch, 0).verdict == Verdict::NeedConfirm);

    // confirmation off (a bench setting): the touch goes straight, the voice still asks
    s.confirm_moves = false;
    CHECK(g.request({Kind::Motion, "m"}, Origin::Touch, 0).verdict == Verdict::Run);
    CHECK(g.request({Kind::Motion, "m"}, Origin::Voice, 0).verdict == Verdict::NeedConfirm);

    // spoken orders off: refused, but a spoken stop still stops
    s.voice_orders = false;
    CHECK(g.request({Kind::Motion, "m"}, Origin::Voice, 0).why == Why::VoiceOrdersOff);
    CHECK(g.request({Kind::Stop, "s"}, Origin::Voice, 0).verdict == Verdict::Run);

    // a spoken yes confirms only when the setting says so
    s.voice_orders = true;
    s.voice_confirm = true;
    Action out;
    g.request({Kind::Motion, "m"}, Origin::Voice, 0);
    CHECK(g.confirm(Origin::Voice, 10, out));

    // a spindle limit of zero: the screen never starts the spindle
    s.spindle_max_rpm = 0;
    CHECK(g.request({Kind::Spindle, "on"}, Origin::Touch, 0).why == Why::SpindleDisabled);
}

static void voice_orders() {
    CHECK(recognise("Stop!", Lang::En) == Intent::Stop);
    CHECK(recognise("please stop now", Lang::En) == Intent::Stop);
    CHECK(recognise("stopwatch", Lang::En) == Intent::None);  // a word that only contains it is no stop
    CHECK(recognise("Para, por favor", Lang::Es) == Intent::Stop);
    CHECK(recognise("ferma", Lang::It) == Intent::Stop);
    CHECK(recognise("Nothalt", Lang::De) == Intent::Stop);
    CHECK(recognise("arrête", Lang::Fr) == Intent::Stop);
    CHECK(recognise("pára", Lang::Pt) == Intent::Stop);
    CHECK(recognise("请停止", Lang::Zh) == Intent::Stop);
    CHECK(recognise("止めて", Lang::Ja) == Intent::Stop);
    CHECK(recognise("pause", Lang::En) == Intent::Pause);
    // in Chinese the word for stop is inside the word for pause: either way the order halts
    CHECK(kind_of(recognise("暂停", Lang::Zh)) == Kind::Stop);
    CHECK(recognise("desliga o fuso", Lang::Pt) == Intent::SpindleOff);

    // what moves or spins must be said whole: inside a longer sentence it is no order
    CHECK(recognise("start job", Lang::En) == Intent::Start);
    CHECK(recognise("I could start job later", Lang::En) == Intent::None);
    CHECK(recognise("Spindel an.", Lang::De) == Intent::SpindleOn);
    CHECK(recognise("accendi il mandrino", Lang::It) == Intent::SpindleOn);
    CHECK(recognise("原点復帰", Lang::Ja) == Intent::Home);
    CHECK(recognise("continúa", Lang::Es) == Intent::Resume);
    CHECK(recognise("sí", Lang::Es) == Intent::Yes);
    CHECK(recognise("nein", Lang::De) == Intent::No);
    CHECK(recognise("", Lang::En) == Intent::None);
    CHECK(recognise("hello there", Lang::En) == Intent::None);
    // an order of another language is no order
    CHECK(recognise("empieza el trabajo", Lang::En) == Intent::None);

    CHECK(kind_of(Intent::Stop) == Kind::Stop);
    CHECK(kind_of(Intent::Pause) == Kind::Stop);
    CHECK(kind_of(Intent::Start) == Kind::Motion);
    CHECK(kind_of(Intent::Home) == Kind::Motion);
    CHECK(kind_of(Intent::SpindleOn) == Kind::Spindle);
    CHECK(kind_of(Intent::Yes) == Kind::Info);
}

static void texts_are_complete() {
    // every phrase and every alert has a text in every language, and none of the languages repeats English for the scripts without spaces
    for (const auto& p : detail::phrases())
        for (std::size_t i = 0; i < kLangCount; ++i) CHECK(p.words[i] != nullptr && std::strlen(p.words[i]) > 0);
    for (int a = 0; a <= static_cast<int>(Alert::Armed); ++a) {
        for (std::size_t i = 0; i < kLangCount; ++i) {
            const char* t = alert_text(static_cast<Alert>(a), static_cast<Lang>(i));
            CHECK(t != nullptr && std::strlen(t) > 3);
            if (i >= 6) CHECK(static_cast<unsigned char>(t[0]) >= 0xE3);  // Chinese and Japanese are not English
            if (i > 0 && i < 6) CHECK(std::strcmp(t, alert_text(static_cast<Alert>(a), Lang::En)) != 0);
        }
    }
}

static void screen_texts() {
    // every text exists in the eight languages, the scripts without spaces are not English, and every refusal has its sentence
    for (unsigned t = 0; t < static_cast<unsigned>(T::Count); ++t) {
        for (std::size_t i = 0; i < kLangCount; ++i) {
            const char* s = text(static_cast<T>(t), static_cast<Lang>(i));
            CHECK(s != nullptr && std::strlen(s) > 0);
            if (i >= 6) CHECK(static_cast<unsigned char>(s[0]) >= 0xE3 || (s[0] >= 'A' && s[0] <= 'Z'));
        }
    }
    for (Why w : {Why::HardwareStop, Why::NotArmed, Why::VoiceOrdersOff, Why::NoHardwareStop, Why::SpindleDisabled})
        for (std::size_t i = 0; i < kLangCount; ++i) CHECK(std::strlen(why_text(w, static_cast<Lang>(i))) > 10);
    CHECK(std::strlen(why_text(Why::None, Lang::En)) == 0);
}

int main() {
    settings_file();
    gate_rules();
    gate_switches();
    voice_orders();
    texts_are_complete();
    screen_texts();
    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
