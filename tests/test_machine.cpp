// EH3D-NODE-HMI-GUI - host tests of the kind of machine: what the screen may order on a CNC machine, a laser and a pick and place machine.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdio>
#include <string>
#include <vector>

#include "../core/ui_text.hpp"

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

static void kinds_from_text() {
    MachineKind k = MachineKind::Laser;
    CHECK(machine_kind_from_text("cnc", k) && k == MachineKind::Cnc);
    CHECK(machine_kind_from_text("LASER", k) && k == MachineKind::Laser);
    CHECK(machine_kind_from_text("pnp", k) && k == MachineKind::PickAndPlace);
    CHECK(machine_kind_from_text("Pick-And-Place", k) && k == MachineKind::PickAndPlace);
    k = MachineKind::Laser;
    CHECK(!machine_kind_from_text("", k) && k == MachineKind::Laser);
    CHECK(!machine_kind_from_text("lathe", k) && k == MachineKind::Laser);
    CHECK(!machine_kind_from_text("cnc ", k));
    for (MachineKind each : {MachineKind::Cnc, MachineKind::Laser, MachineKind::PickAndPlace}) {
        MachineKind back = MachineKind::Cnc;
        CHECK(machine_kind_from_text(machine_kind_key(each), back) && back == each);
    }
}

static void only_a_cnc_machine_has_a_spindle() {
    CHECK(controls_for(MachineKind::Cnc).spindle);
    CHECK(!controls_for(MachineKind::Laser).spindle);
    CHECK(!controls_for(MachineKind::PickAndPlace).spindle);
}

static void the_settings_file_takes_the_kind() {
    std::vector<Problem> p;
    CHECK(parse_settings("", p).machine == MachineKind::Cnc);
    CHECK(p.empty());
    CHECK(parse_settings("machine_kind = laser\n", p).machine == MachineKind::Laser);
    CHECK(parse_settings("machine_kind = PNP  # a comment is not part of the value\n", p).machine == MachineKind::Cnc);
    CHECK(p.size() == 1 && p[0].key == "machine_kind");
    p.clear();
    // a typing mistake keeps the default and says so
    Settings s = parse_settings("machine_kind = lasr\n", p);
    CHECK(s.machine == MachineKind::Cnc && p.size() == 1 && p[0].line == 1);
    p.clear();
    CHECK(parse_settings("machine_kind = pick-and-place\n", p).machine == MachineKind::PickAndPlace);
    CHECK(p.empty());
}

static Settings ready(MachineKind k) {
    Settings s;
    s.voice_orders = true;
    s.machine = k;
    return s;
}

static void make_ready(Gate& g) {
    g.hardware_stop(true, false);
    CHECK(g.acknowledge_release());
    g.link(true);
}

static void the_gate_never_lets_the_screen_start_a_spindle_that_the_machine_does_not_have() {
    Action spindle{Kind::Spindle, "spindle on"};
    Action jog{Kind::Motion, "jog X +10"};
    {
        Settings s = ready(MachineKind::Cnc);
        Gate g(s);
        make_ready(g);
        CHECK(g.request(spindle, Origin::Touch, 0).verdict == Verdict::NeedConfirm);
    }
    for (MachineKind k : {MachineKind::Laser, MachineKind::PickAndPlace}) {
        Settings s = ready(k);
        Gate g(s);
        make_ready(g);
        Decision d = g.request(spindle, Origin::Touch, 0);
        CHECK(d.verdict == Verdict::Refused && d.why == Why::NoSpindleHere);
        CHECK(!g.pending());
        // a move is still asked for as always
        CHECK(g.request(jog, Origin::Touch, 10).verdict == Verdict::NeedConfirm);
        // and no setting turns the refusal off: a spindle speed in the file does not give a laser a spindle
        s.spindle_max_rpm = 65535;
        Gate g2(s);
        make_ready(g2);
        CHECK(g2.request(spindle, Origin::Touch, 0).why == Why::NoSpindleHere);
    }
}

static void the_refusal_has_its_sentence_in_every_language() {
    for (unsigned l = 0; l < 8; ++l) {
        const char* t = why_text(Why::NoSpindleHere, static_cast<Lang>(l));
        CHECK(t != nullptr && t[0] != '\0');
        CHECK(std::string(t) != why_text(Why::SpindleDisabled, static_cast<Lang>(l)));
    }
    CHECK(std::string(why_text(Why::NoSpindleHere, Lang::En)).find("spindle") != std::string::npos);
}

int main() {
    kinds_from_text();
    only_a_cnc_machine_has_a_spindle();
    the_settings_file_takes_the_kind();
    the_gate_never_lets_the_screen_start_a_spindle_that_the_machine_does_not_have();
    the_refusal_has_its_sentence_in_every_language();
    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
