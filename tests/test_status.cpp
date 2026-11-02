// EH3D-NODE-HMI-GUI - host tests of what the screen tells the operator: the errors of the settings file, the state of the link and the age of the data, the notes, the question
// of the gate with its countdown, and the identification of the client. Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../core/status.hpp"
#include "../core/workflow.hpp"

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

static bool has(const std::string& s, const char* part) { return s.find(part) != std::string::npos; }

static void errors_of_the_settings_file() {
    std::vector<Problem> p;
    parse_settings("language = en\nvolume_percent = 700\nflux = 1\nnothing here\nconfirm_moves = maybe\ntoken = short\nlanguage = xx\n", p);
    CHECK(p.size() == 6);
    CHECK(p[0].reason == CfgReason::Range && p[0].line == 2 && p[0].key == "volume_percent");
    CHECK(p[1].reason == CfgReason::Unknown && p[2].reason == CfgReason::NotPair && p[3].reason == CfgReason::OnOff);
    CHECK(p[4].reason == CfgReason::Token && p[5].reason == CfgReason::Choice);
    // the screen says which line and why, in the language of the operator, and how many more there are
    const std::string en = cfg_report(p, Lang::En);
    CHECK(has(en, "hmi.cfg") && has(en, "line 2") && has(en, "(volume_percent)") && has(en, "out of range") && has(en, "+5 more"));
    const std::string es = cfg_report(p, Lang::Es);
    CHECK(has(es, "hmi.cfg") && has(es, "línea 2") && has(es, "fuera de rango") && !has(es, "out of range"));
    CHECK(has(cfg_report(p, Lang::Ja), "行") && has(cfg_report(p, Lang::Ru), "строка"));
    // a line with no key shows no key; no problem shows nothing
    std::vector<Problem> one;
    parse_settings("nothing here\n", one);
    CHECK(one.size() == 1 && !has(cfg_report(one, Lang::En), "(") && !has(cfg_report(one, Lang::En), "more"));
    CHECK(cfg_report({}, Lang::En).empty());
    // the token is never repeated in the report
    std::vector<Problem> tok;
    parse_settings("token = abc\n", tok);
    CHECK(tok.size() == 1 && !has(cfg_report(tok, Lang::En), "abc"));
    // every reason has a text in every language
    for (std::size_t i = 0; i < kLangCount; ++i) {
        for (CfgReason r : {CfgReason::NotPair, CfgReason::Range, CfgReason::OnOff, CfgReason::Choice, CfgReason::Token, CfgReason::Unknown}) CHECK(std::strlen(cfg_reason_text(r, static_cast<Lang>(i))) > 0);
    }
    // the age after which data are old is a setting, with a range
    std::vector<Problem> q;
    CHECK(parse_settings("", q).stale_after_s == 15);
    CHECK(parse_settings("stale_after_s = 30", q).stale_after_s == 30 && q.empty());
    CHECK(parse_settings("stale_after_s = 1", q).stale_after_s == 15 && q.size() == 1);
    CHECK(parse_settings("stale_after_s = 99999", q).stale_after_s == 15 && q.size() == 2);
}

static void the_link() {
    Response ok;
    ok.status = 200;
    Response err;
    err.status = 401;
    err.body = "{\"error\":\"unauthorized\",\"detail\":\"bad token\"}";
    Response html;
    html.status = 200;
    html.body = "<html>router login</html>";
    Response missing;
    missing.status = 404;
    CHECK(classify_fleet(false, ok, false) == Answer::Silent);
    CHECK(classify_fleet(true, ok, true) == Answer::Fresh);
    CHECK(classify_fleet(true, err, false) == Answer::ServerError);
    CHECK(classify_fleet(true, html, false) == Answer::NotEh3d);
    CHECK(classify_fleet(true, missing, false) == Answer::NotEh3d);

    LinkTracker t;
    CHECK(t.state() == Link::Unknown && !t.has_data() && !t.stale(1000000, 15));  // data that never came are absent, not old
    t.update(Answer::Fresh, true, 1000);
    CHECK(t.state() == Link::Connected && t.has_data() && t.age_s(1000) == 0 && !t.stale(1000, 15));
    CHECK(t.age_s(8500) == 7 && !t.stale(8500, 15));
    CHECK(!t.stale(15999, 15) && t.stale(16000, 15));  // exactly N seconds old is old
    t.update(Answer::Silent, true, 20000);
    CHECK(t.state() == Link::Lost && t.stale(20000, 15) && t.age_s(20000) == 19);  // lost and old: both are said
    t.update(Answer::ServerError, true, 21000);
    CHECK(t.state() == Link::Connected && t.age_s(21000) == 20);  // it answers but gave no data: the data go on growing old
    t.update(Answer::NotEh3d, true, 22000);
    CHECK(t.state() == Link::OtherServer);
    t.update(Answer::Fresh, false, 23000);  // an EH3D server that does not list the machine of the settings is another server
    CHECK(t.state() == Link::OtherServer && t.age_s(23000) == 0);
    t.update(Answer::Fresh, true, 24000);
    CHECK(t.state() == Link::Connected && !t.stale(24000, 15));
    // the clock may wrap
    LinkTracker w;
    w.update(Answer::Fresh, true, 0xFFFFFF00u);
    CHECK(w.age_s(0x00000100u) == 0 && !w.stale(0x00000100u, 2));

    FleetView v;
    MachineRow a;
    a.id = "drill1";
    v.machines.push_back(a);
    CHECK(fleet_has_machine(v, "drill1") && !fleet_has_machine(v, "drill2") && fleet_has_machine(v, ""));

    // the labels
    CHECK(link_label(Link::Connected, false, 0, Lang::En) == "Connected");
    CHECK(link_label(Link::Lost, false, 0, Lang::En) == "No connection");
    CHECK(link_label(Link::OtherServer, false, 0, Lang::En) == "Other server");
    CHECK(link_label(Link::Unknown, false, 0, Lang::En) == "-");
    CHECK(link_label(Link::Connected, true, 42, Lang::En) == "Connected  |  Old data 42 s");
    CHECK(has(link_label(Link::Lost, true, 42, Lang::Es), "Sin conexión") && has(link_label(Link::Lost, true, 42, Lang::Es), "Datos antiguos 42 s"));
    CHECK(stale_mark(Lang::En) == "[Old data]" && stale_mark(Lang::De) == "[Alte Daten]");
    for (std::size_t i = 0; i < kLangCount; ++i) {
        const Lang l = static_cast<Lang>(i);
        CHECK(link_label(Link::Connected, false, 0, l) != link_label(Link::Lost, false, 0, l));
        CHECK(link_label(Link::Lost, false, 0, l) != link_label(Link::OtherServer, false, 0, l));
    }

    // the notes of a link that is not connected: where the screen looked
    Settings s;
    s.server_host = "192.168.0.180";
    CHECK(link_note(Link::Connected, s, Lang::En).empty() && link_note(Link::Unknown, s, Lang::En).empty());
    CHECK(link_note(Link::Lost, s, Lang::En) == "The server does not answer: 192.168.0.180:18200");
    CHECK(has(link_note(Link::OtherServer, s, Lang::En), "EH3D") && has(link_note(Link::OtherServer, s, Lang::En), "192.168.0.180:18200"));
    s.server_port = 9000;
    CHECK(has(link_note(Link::Lost, s, Lang::Fr), "192.168.0.180:9000"));
    Settings none;
    CHECK(has(link_note(Link::Lost, none, Lang::En), "server_host") && has(link_note(Link::Lost, none, Lang::En), "hmi.cfg"));
    CHECK(call_error_text(CallError::None, s, Lang::En).empty() && has(call_error_text(CallError::NoServer, s, Lang::Es), "server_host"));
    for (CallError e : {CallError::Client, CallError::TooLong, CallError::Broke, CallError::Send, CallError::NoAnswer}) CHECK(!call_error_text(e, s, Lang::Pl).empty());
}

static void the_notes() {
    const ScreenNote all[] = {ScreenNote::StopFail, ScreenNote::StopRefused, ScreenNote::StopSent, ScreenNote::NoJobToStop, ScreenNote::MoveNoLink, ScreenNote::Busy, ScreenNote::NothingToSend, ScreenNote::OldServer,
                        ScreenNote::Ok, ScreenNote::Following, ScreenNote::NoJobReady, ScreenNote::GoingOn, ScreenNote::File, ScreenNote::Unreadable, ScreenNote::AllOff};
    for (std::size_t i = 0; i < kLangCount; ++i) {
        for (ScreenNote n : all) CHECK(std::strlen(note_text(n, static_cast<Lang>(i))) > 0);
    }
    CHECK(std::string(note_text(ScreenNote::StopSent, Lang::En)) == "Stop sent" && std::string(note_text(ScreenNote::StopSent, Lang::Es)) == "Paro enviado");
    CHECK(note_with(ScreenNote::File, "board.ncd", Lang::En) == "File: board.ncd" && note_with(ScreenNote::StopSent, "", Lang::En) == "Stop sent");
    // the reasons of the workflow are translated, and the words of the server are left alone
    Workflow w;
    std::string why;
    CHECK(!w.choose("notes.txt", "abc", why) && why == reasons::kNotAJobFile);
    CHECK(has(reason_text(why, Lang::Es), "Solo se pueden enviar") && !has(reason_text(why, Lang::Es), "only drilling"));
    CHECK(!w.choose("board.ncd", "", why) && reason_text(why, Lang::En) == "The file is empty");
    CHECK(reason_text(reasons::kOldServerNoFile, Lang::De) != reasons::kOldServerNoFile && reason_text(reasons::kNotStopped, Lang::Fr) == reason_text(reasons::kNothingToGoOn, Lang::Fr));
    CHECK(reason_text("unauthorized: bad token", Lang::Ja) == "unauthorized: bad token");
}

static void the_question_of_the_gate() {
    // the order in the language of the operator, whatever the id of the action
    CHECK(order_text({Kind::Stop, "stop"}, Lang::Es) == "Parar la máquina");
    CHECK(order_text({Kind::Start, "start job"}, Lang::Es) == "Empezar el trabajo");
    CHECK(order_text({Kind::Start, "start job"}, Lang::En) == "Start the job");
    CHECK(order_text({Kind::Pause, "pause"}, Lang::De) == "Auftrag pausieren");
    CHECK(order_text({Kind::Motion, "resume job"}, Lang::It) == "Riprendi il lavoro");
    CHECK(order_text({Kind::Motion, "home"}, Lang::Fr) == "Ramener la machine à l'origine");
    CHECK(order_text({Kind::Motion, "tool changed"}, Lang::Pt) == "Confirmar a troca de ferramenta");
    CHECK(order_text({Kind::Motion, "jog X -"}, Lang::En) == "Jog X -" && order_text({Kind::Motion, "jog Z +"}, Lang::Es) == "Mover Z +");
    CHECK(order_text({Kind::Spindle, "spindle on 12000 rpm"}, Lang::Es) == "Husillo 12000 rpm");
    CHECK(order_text({Kind::Info, "something new"}, Lang::Es) == "something new");
    for (std::size_t i = 0; i < kLangCount; ++i) {
        const Lang l = static_cast<Lang>(i);
        for (const char* id : {"stop", "start job", "pause", "resume job", "tool changed", "home"}) CHECK(order_text({Kind::Motion, id}, l) != id);
    }

    // the countdown follows the time the gate gives
    Settings s;
    s.confirm_timeout_ms = 5000;
    Gate g(s);
    g.hardware_stop(true, false);
    g.acknowledge_release();
    g.link(true);
    CHECK(g.remaining_ms(100) == 0);  // nothing waits
    Decision d = g.request({Kind::Motion, "home"}, Origin::Touch, 1000);
    CHECK(d.verdict == Verdict::NeedConfirm);
    CHECK(g.remaining_ms(1000) == 5000 && g.remaining_ms(3500) == 2500 && g.remaining_ms(5999) == 1 && g.remaining_ms(6000) == 0 && g.remaining_ms(9000) == 0);
    CHECK(countdown_s(5000) == 5 && countdown_s(4001) == 5 && countdown_s(1) == 1 && countdown_s(0) == 0);
    CHECK(countdown_text(5000, Lang::En) == "Cancels in 5 s" && countdown_text(2500, Lang::Es) == "Se cancela en 3 s" && countdown_text(0, Lang::En).empty());
    g.tick(7000);
    CHECK(!g.pending() && g.remaining_ms(7000) == 0);  // it aborts by itself
    g.request({Kind::Motion, "home"}, Origin::Touch, 0xFFFFFFF0u);
    CHECK(g.remaining_ms(0xFFFFFFF0u) == 5000 && g.remaining_ms(0x00000010u) < 5000);  // the clock may wrap
    g.cancel();
    CHECK(g.remaining_ms(0xFFFFFFF0u) == 0);
}

static void the_identification() {
    CHECK(client_id("eh3d-node-hmi-gui", "0.0.5") == "eh3d-node-hmi-gui/0.0.5" && client_id("x", "") == "x");
    const std::string w = wire(fleet_request(), "h:18200", "tok-1234", client_id("eh3d-node-hmi-gui", "0.0.5"));
    CHECK(has(w, "User-Agent: eh3d-node-hmi-gui/0.0.5\r\n") && has(w, "X-EH3D-Client: eh3d-node-hmi-gui/0.0.5\r\n"));
    CHECK(w.find("\r\n\r\n") == w.size() - 4 && has(w, "Authorization: Bearer tok-1234\r\n"));
    CHECK(!has(wire(fleet_request(), "h", "t"), "X-EH3D-Client"));  // no name given, no header
    CHECK(firmware_label("0.0.5", Lang::En) == "Firmware 0.0.5" && has(firmware_label("0.0.5", Lang::Zh), "固件"));
}

int main() {
    errors_of_the_settings_file();
    the_link();
    the_notes();
    the_question_of_the_gate();
    the_identification();
    std::printf("status: %d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
