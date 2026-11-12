// EH3D-NODE-HMI-GUI - host tests of the settings file, the gate between the operator and the machine, and the voice.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../core/ops.hpp"
#include "../core/server_link.hpp"
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
    Settings pl = parse_settings("language = pl\nvolume_percent = 20", p);
    CHECK(p.empty() && pl.language == Lang::Pl);
    Settings br = parse_settings("language = pt-br", p);
    CHECK(p.empty() && br.language == Lang::PtBr);
    parse_settings("language = PL", p);
    CHECK(!p.empty());  // the codes are lower case
    for (unsigned i = 0; i < kLangCount; ++i) {
        Lang l;
        CHECK(lang_from_code(lang_code(static_cast<Lang>(i)), l) && l == static_cast<Lang>(i));
    }
    // the table of the machine for the warning of a board that does not fit: none by default (the server's is used), or the one of the file
    p.clear();
    CHECK(d.table_width_mm == 0 && d.table_depth_mm == 0);
    Settings t = parse_settings("table_width_mm = 495\ntable_depth_mm = 325\n", p);
    CHECK(p.empty() && t.table_width_mm == 495 && t.table_depth_mm == 325);
    Settings tb = parse_settings("table_width_mm = -5\ntable_depth_mm = 9000\n", p);
    CHECK(p.size() == 2 && tb.table_width_mm == 0 && tb.table_depth_mm == 0);
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

static void spindle_over_the_limit() {
    // the setting alone limits an order when the machine's spindle is not known
    Settings s = ready_settings();
    Gate g(s);
    make_ready(g);
    CHECK(g.spindle_limit() == 24000);
    Decision d = g.request({Kind::Spindle, "spindle on", 12000}, Origin::Touch, 0);
    CHECK(d.verdict == Verdict::NeedConfirm && !d.clipped && d.rpm == 12000 && g.pending_action().rpm == 12000);
    CHECK(g.pending_action().what == "spindle on 12000 rpm");  // the confirmation shows the turns that will be asked
    // the spindle the machine carries gives less: the order is cut to it, and the screen is told
    g.machine_spindle_max(18000);
    CHECK(g.spindle_limit() == 18000);
    d = g.request({Kind::Spindle, "spindle on", 24000}, Origin::Touch, 0);
    CHECK(d.verdict == Verdict::NeedConfirm && d.clipped && d.asked_rpm == 24000 && d.rpm == 18000);
    CHECK(g.pending_action().rpm == 18000 && g.pending_action().what == "spindle on 18000 rpm");
    Action out;
    CHECK(g.confirm(Origin::Touch, 10, out) && out.rpm == 18000 && out.what == "spindle on 18000 rpm");
    // an order inside the limit is not touched; a spindle that gives more than the setting does not raise it
    d = g.request({Kind::Spindle, "spindle on", 18000}, Origin::Touch, 0);
    CHECK(d.verdict == Verdict::NeedConfirm && !d.clipped && d.rpm == 18000);
    g.machine_spindle_max(60000);
    CHECK(g.spindle_limit() == 24000);
    d = g.request({Kind::Spindle, "spindle on", 30000}, Origin::Touch, 0);
    CHECK(d.clipped && d.rpm == 24000);
    // an order that does not say the turns (the speed of the job) is not cut and says none
    g.machine_spindle_max(18000);
    d = g.request({Kind::Spindle, "spindle on"}, Origin::Touch, 0);
    CHECK(d.verdict == Verdict::NeedConfirm && !d.clipped && d.rpm == 0 && g.pending_action().what == "spindle on");
    // a move is never cut
    d = g.request({Kind::Motion, "jog X +", 99999}, Origin::Touch, 0);
    CHECK(d.verdict == Verdict::NeedConfirm && !d.clipped && g.pending_action().rpm == 99999);
    // without the confirmation (a bench setting) the cut order goes at once with the turns it got
    s.confirm_moves = false;
    d = g.request({Kind::Spindle, "spindle on", 24000}, Origin::Touch, 0);
    CHECK(d.verdict == Verdict::Run && d.clipped && d.rpm == 18000);
    // the other choice of the setting: refuse
    s.confirm_moves = true;
    s.spindle_over = SpindleOver::Refuse;
    d = g.request({Kind::Spindle, "spindle on", 24000}, Origin::Touch, 0);
    CHECK(d.verdict == Verdict::Refused && d.why == Why::SpindleTooFast && !g.pending());
    CHECK(g.request({Kind::Spindle, "spindle on", 18000}, Origin::Touch, 0).verdict == Verdict::NeedConfirm);
    // a spoken order is cut the same way, and a stop is never held back
    s.spindle_over = SpindleOver::Clip;
    d = g.request({Kind::Spindle, "spindle on", 24000}, Origin::Voice, 0);
    CHECK(d.verdict == Verdict::NeedConfirm && d.clipped && d.rpm == 18000);
    CHECK(g.request({Kind::Stop, "stop", 0}, Origin::Touch, 0).verdict == Verdict::Run);
    // the setting: the file reads it and the default is to cut
    std::vector<Problem> p;
    CHECK(parse_settings("", p).spindle_over == SpindleOver::Clip && parse_settings("", p).spindle_rpm == 12000);
    CHECK(parse_settings("spindle_rpm = 9000", p).spindle_rpm == 9000 && parse_settings("spindle_rpm = 70000", p).spindle_rpm == 12000);
    p.clear();
    CHECK(parse_settings("spindle_over = refuse", p).spindle_over == SpindleOver::Refuse && p.empty());
    CHECK(parse_settings("spindle_over = clip", p).spindle_over == SpindleOver::Clip && p.empty());
    p.clear();
    CHECK(parse_settings("spindle_over = maybe", p).spindle_over == SpindleOver::Clip && p.size() == 1);
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
    CHECK(kind_of(recognise("暂停", Lang::Zh)) == Kind::Stop || kind_of(recognise("暂停", Lang::Zh)) == Kind::Pause);
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
    CHECK(kind_of(Intent::Pause) == Kind::Pause);
    CHECK(kind_of(Intent::Start) == Kind::Start);
    CHECK(kind_of(Intent::Home) == Kind::Motion);
    CHECK(kind_of(Intent::SpindleOn) == Kind::Spindle);
    CHECK(kind_of(Intent::Yes) == Kind::Info);
}

static void texts_are_complete() {
    // every phrase and every alert has a text in every language, and none of the languages repeats English for the scripts without spaces
    for (const auto& p : detail::phrases())
        for (std::size_t i = 0; i < kBaseLangCount; ++i) CHECK(p.words[i] != nullptr && std::strlen(p.words[i]) > 0);
    for (int a = 0; a <= static_cast<int>(Alert::Armed); ++a) {
        for (std::size_t i = 0; i < kBaseLangCount; ++i) {  // the spoken alerts and orders exist in the eight languages; the others say them in English
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
        for (std::size_t i = 0; i < kBaseLangCount; ++i) {  // the other twenty-nine languages are checked in test_locales.cpp
            const char* s = text(static_cast<T>(t), static_cast<Lang>(i));
            CHECK(s != nullptr && std::strlen(s) > 0);
            if (i >= 6) CHECK(static_cast<unsigned char>(s[0]) >= 0xE3 || (s[0] >= 'A' && s[0] <= 'Z'));
        }
    }
    for (Why w : {Why::HardwareStop, Why::NotArmed, Why::VoiceOrdersOff, Why::NoHardwareStop, Why::SpindleDisabled, Why::SpindleTooFast})
        for (std::size_t i = 0; i < kLangCount; ++i) CHECK(std::strlen(why_text(w, static_cast<Lang>(i))) > 10);
    CHECK(std::strlen(why_text(Why::None, Lang::En)) == 0);
}

static void the_start_the_pause_and_the_edge_of_the_stop() {
    Settings s = ready_settings();
    Gate g(s);
    make_ready(g);
    // a job that is ready is not armed yet: the start still asks, and the question survives the loop that tells the gate the link is not armed
    g.link(false);
    CHECK(g.request({Kind::Motion, "jog X +10"}, Origin::Touch, 0).why == Why::NotArmed);
    Decision d = g.request({Kind::Start, "start job"}, Origin::Touch, 100);
    CHECK(d.verdict == Verdict::NeedConfirm && g.pending());
    g.link(false);
    CHECK(g.pending());
    Action out;
    CHECK(g.confirm(Origin::Touch, 200, out) && out.kind == Kind::Start && out.what == "start job");
    // without the confirmation of the bench the start goes at once
    Settings bench = ready_settings();
    bench.confirm_moves = false;
    Gate gb(bench);
    make_ready(gb);
    gb.link(false);
    CHECK(gb.request({Kind::Start, "start job"}, Origin::Touch, 0).verdict == Verdict::Run);
    // the hardware stop still blocks the start, and a spoken start asks even when the setting does not
    g.hardware_stop(true, true);
    CHECK(g.request({Kind::Start, "start job"}, Origin::Touch, 0).why == Why::HardwareStop);
    g.hardware_stop(true, false);
    CHECK(g.acknowledge_release());
    g.link(false);
    CHECK(g.request({Kind::Start, "start job"}, Origin::Voice, 0).verdict == Verdict::NeedConfirm);
    // a start is dropped when the confirmation comes after the stop was pressed
    g.hardware_stop(true, true);
    CHECK(!g.pending());

    // a pause is never held back, even with the hardware stop latched, and it does not wait for a confirmation
    Gate p(s);
    CHECK(p.latched());
    CHECK(p.request({Kind::Pause, "pause"}, Origin::Touch, 0).verdict == Verdict::Run);
    make_ready(p);
    CHECK(p.request({Kind::Pause, "pause"}, Origin::Touch, 0).verdict == Verdict::Run && !p.pending());

    // the press of the hardware stop is an edge that the application takes once
    Gate e(s);
    e.hardware_stop(true, false);
    CHECK(!e.take_stop_edge());
    e.hardware_stop(true, true);
    CHECK(e.take_stop_edge());
    CHECK(!e.take_stop_edge());
    e.hardware_stop(true, true);  // still held down: not a new press
    CHECK(!e.take_stop_edge());
    e.hardware_stop(true, false);
    e.hardware_stop(true, true);
    CHECK(e.take_stop_edge());
}

// The orders of the screen are an enum, and the id of each one reads back to it.
static void orders_are_an_enum() {
    int seen = 0;
    for (const OpEntry& e : kOps) {
        CHECK(op_of(e.id) == e.op);
        CHECK(std::string(op_id(e.op)) == e.id);
        ++seen;
    }
    CHECK(seen == 7);
    CHECK(op_of("start job") == Op::StartJob && op_of("pause") == Op::Pause && op_of("resume job") == Op::ResumeJob && op_of("tool changed") == Op::ToolChanged);
    CHECK(op_of("spindle on 12000 rpm") == Op::SpindleOn && op_of("spindle on") == Op::SpindleOn);
    // what is not an order is none: a jog, a text nobody sends, the empty text, a near miss
    CHECK(op_of("jog X +10 mm") == Op::None && op_of("") == Op::None && op_of("Pause") == Op::None && op_of("pause ") == Op::None && op_of("spindle") == Op::None);
    CHECK(std::string(op_id(Op::None)).empty());
    // no two orders share an id
    for (const OpEntry& a : kOps)
        for (const OpEntry& b : kOps) CHECK(a.op == b.op || std::string(a.id) != b.id);
}

// The health check tells "nobody answers" from "something answers that is not an EH3D server".
static void health_check() {
    CHECK(health_request().method == "GET" && health_request().path == "/api/v1/health" && health_request().body.empty());
    auto answer = [](int status, const std::string& body) {
        Response r;
        r.status = status;
        r.body = body;
        return r;
    };
    CHECK(classify_health(false, Response{}) == Health::Silent);
    CHECK(classify_health(true, answer(200, "{\"name\":\"eh3d-server\",\"version\":\"0.1.0\",\"api_revision\":3,\"status\":\"ok\"}")) == Health::Ok);
    CHECK(classify_health(true, answer(200, "{\"name\":\"eh3d-server\",\"status\":\"degraded\"}")) == Health::Degraded);
    CHECK(classify_health(true, answer(200, "{\"name\":\"eh3d-server\"}")) == Health::Degraded);   // no status: not said to be well
    // another service on that address, a page of a router or a proxy, a server of another family
    CHECK(classify_health(true, answer(200, "<html><body>It works</body></html>")) == Health::NotEh3d);
    CHECK(classify_health(true, answer(200, "{\"name\":\"other-server\",\"status\":\"ok\"}")) == Health::NotEh3d);
    CHECK(classify_health(true, answer(200, "[]")) == Health::NotEh3d);
    CHECK(classify_health(true, answer(200, "")) == Health::NotEh3d);
    CHECK(classify_health(true, answer(404, "{\"error\":\"not_found\"}")) == Health::NotEh3d);   // an old server without the route is not told apart from another service
    CHECK(classify_health(true, answer(503, "{\"name\":\"eh3d-server\",\"status\":\"ok\"}")) == Health::NotEh3d);
}

// A settings file damaged at every place and in many copies (a fixed seed, so a failure is the same failure every time): it is always read, and no value ever leaves its range.
static void settings_file_damaged() {
    const std::string seed =
        "# the screen\nlanguage = es\ntheme = forge\norientation = landscape\nbrightness_percent = 60\nidle_dim_s = 300\nserver_host = 192.168.0.180\nserver_port = 18200\n"
        "poll_s = 5\nconfirm_moves = on\nconfirm_timeout_ms = 8000\njog_max_mm = 20\nspindle_max_rpm = 24000\ntoken = abcdefgh12345678\nvoice_orders = off\n";
    auto in_range = [](const Settings& s) {
        return s.brightness_percent >= 10 && s.brightness_percent <= 100 && s.idle_dim_s <= 3600 && s.server_port >= 1 && s.poll_s >= 1 && s.poll_s <= 60 &&
               s.confirm_timeout_ms >= 1000 && s.confirm_timeout_ms <= 60000 && s.jog_max_mm >= 1 && s.jog_max_mm <= 200 && s.spindle_max_rpm <= 65535;
    };
    std::vector<Problem> p0;
    Settings whole = parse_settings(seed, p0);
    CHECK(p0.empty() && whole.server_port == 18200 && whole.jog_max_mm == 20);
    for (std::size_t end = 0; end <= seed.size(); ++end) {
        std::vector<Problem> p;
        CHECK(in_range(parse_settings(seed.substr(0, end), p)));
    }
    std::uint64_t x = 0x9E3779B97F4A7C15ULL;  // xorshift64: the same numbers on every machine
    auto next = [&x]() {
        x ^= x << 13;
        x ^= x >> 7;
        x ^= x << 17;
        return x;
    };
    const std::string pieces = "0123456789-+.eE=\n #xX\r\t,;ofnabc";
    for (int round = 0; round < 600; ++round) {
        std::string copy = seed;
        for (int k = 0; k < 1 + round % 5; ++k) {
            std::size_t at = static_cast<std::size_t>(next() % copy.size());
            switch (next() % 4) {
                case 0: copy[at] = pieces[next() % pieces.size()]; break;
                case 1: copy.insert(at, 1, static_cast<char>(next() & 0xFF)); break;
                case 2: copy.erase(at, 1); break;
                default: copy.resize(at > 0 ? at : 1); break;
            }
            if (copy.empty()) copy = "0";
        }
        std::vector<Problem> p, q;
        Settings a = parse_settings(copy, p);
        Settings b = parse_settings(copy, q);
        CHECK(in_range(a));
        CHECK(p.size() == q.size() && a.server_port == b.server_port && a.token == b.token);
    }
}

int main() {
    settings_file_damaged();
    orders_are_an_enum();
    health_check();
    settings_file();
    gate_rules();
    gate_switches();
    the_start_the_pause_and_the_edge_of_the_stop();
    spindle_over_the_limit();
    voice_orders();
    texts_are_complete();
    screen_texts();
    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
