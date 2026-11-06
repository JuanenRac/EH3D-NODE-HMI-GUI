// EH3D-NODE-HMI-GUI - host tests of the bar of the tool changer (places, chaining of the drilling and the routing, the request of the second version), the stages of a job that drills and routes
// and the settings of both.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cmath>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

#include "../core/hmi_config.hpp"
#include "../core/notices.hpp"
#include "../core/rack.hpp"
#include "../core/stage_notice.hpp"
#include "../core/status.hpp"
#include "../core/workflow.hpp"

static int g_checks = 0;
static int g_failed = 0;
#define CHECK(c)                                                \
    do {                                                        \
        ++g_checks;                                             \
        if (!(c)) {                                             \
            ++g_failed;                                         \
            std::printf("FAILED line %d: %s\n", __LINE__, #c); \
        }                                                       \
    } while (0)

using namespace hmi;

static bool has(const std::string& s, const char* part) { return s.find(part) != std::string::npos; }

static Response answer(int status, const std::string& body) { return Response{status, body}; }

static FleetView fleet_with(const std::string& job_id, const std::string& model, int done = 2, int total = 5, const std::string& extra = "") {
    const std::string job = job_id.empty() ? std::string("null")
                                           : "{\"id\":\"" + job_id + "\",\"name\":\"board\",\"percent\":40,\"holes_done\":" + std::to_string(done) + ",\"holes_total\":" + std::to_string(total) + extra + "}";
    FleetView v;
    parse_fleet("{\"time_s\":1800000000,\"machines\":[{\"id\":\"m1\",\"name\":\"M\",\"model\":\"" + model + "\",\"state\":\"" + std::string(job_id.empty() ? "idle" : "busy") + "\",\"job\":" + job + "}]}", v);
    return v;
}

// a drilling file with tools 1 (0.6 mm), 2 (0.8 mm) and 3 (1.0 mm), used in the order 2, 1, 3, and a routing file with the pens 1 and 2
static const char* kDrill = "M48\nMETRIC\nT1C0.6\nT2C0.8\nT3C1.0\n%\nT2\nX1Y1\nX2Y2\nT1\nX3Y3\nT3\nX4Y4\nM30\n";
static const char* kRoute = "IN;SP1;PU0,0;PD40,0,40,40;PU;SP2;PU10,10;PD20,20;PU;";

// ---- the places of the bar --------------------------------------------------------------------------------------------------------------

static void places_of_the_bar() {
    std::vector<int> v;
    CHECK(parse_places("1-14", v) && v.size() == 14 && v.front() == 1 && v.back() == 14);
    CHECK(parse_places("1, 2,5-8", v) && v.size() == 6 && v[2] == 5 && v[5] == 8);
    CHECK(parse_places("3,3,2", v) && v.size() == 2 && v[0] == 2);
    CHECK(parse_places("0,15,16,4", v) && v.size() == 1 && v[0] == 4);  // 0 is the test pin and 15 the detector: not places for a tool
    for (const char* bad : {"", " ", "a", "1-", "-3", "5-2", "1,,2", "1,", "1 2"}) CHECK(!parse_places(bad, v));

    const std::vector<int> defined = {1, 2, 3, 4, 5};
    const std::vector<RackTool> tools = {{false, 1, 0.6}, {false, 2, 0.8}, {true, 1, 0}};
    RackAssignment a;
    // nothing given: every tool has no place
    std::vector<RackProblem> p = check_rack(tools, a, defined);
    CHECK(p.size() == 3 && p[0].kind == RackProblem::Kind::Unassigned && p[2].route && p[2].tool == 1);
    // two in one place, one in a place the machine does not have
    set_slot(a, false, 1, 2);
    set_slot(a, false, 2, 2);
    set_slot(a, true, 1, 9);
    p = check_rack(tools, a, defined);
    CHECK(p.size() == 2 && p[0].kind == RackProblem::Kind::Shared && p[0].tool == 2 && p[0].other_tool == 1 && p[0].slot == 2);
    CHECK(p[1].kind == RackProblem::Kind::Undefined && p[1].route && p[1].slot == 9);
    // taking a place away and giving another
    set_slot(a, false, 2, 0);
    CHECK(slot_of(a, false, 2) == 0 && slot_of(a, false, 1) == 2);
    set_slot(a, false, 2, 4);
    set_slot(a, true, 1, 5);
    CHECK(check_rack(tools, a, defined).empty());

    // the tools without a place take the free ones from the lowest: the drills before the cutters, the small before the big
    RackAssignment b;
    set_slot(b, false, 3, 1);  // the operator put tool 3 in the first place
    const std::vector<RackTool> more = {{false, 3, 1.0}, {false, 1, 0.6}, {true, 2, 0}, {false, 2, 0.8}};
    fill_in_order(more, b, defined);
    CHECK(slot_of(b, false, 3) == 1 && slot_of(b, false, 1) == 2 && slot_of(b, false, 2) == 3 && slot_of(b, true, 2) == 4);
    // when the places run out the rest stay without one
    RackAssignment c;
    fill_in_order(more, c, {1, 2});
    CHECK(slot_of(c, false, 1) == 1 && slot_of(c, false, 2) == 2 && slot_of(c, false, 3) == 0 && slot_of(c, true, 2) == 0);
    CHECK(selectable_places({14, 0, 3, 3, 15}) == std::vector<int>({3, 14}));
}

static void chaining_the_programs() {
    RackAssignment a;
    set_slot(a, false, 1, 3);
    set_slot(a, false, 2, 1);
    set_slot(a, true, 1, 5);
    ProgramTools drill;
    drill.loaded = true;
    drill.tools = {{false, 2, 0.8}, {false, 1, 0.6}};
    ProgramTools route;
    route.loaded = true;
    route.tools = {{true, 1, 0}};
    // the drilling leaves its last tool in the chuck when a routing is still to come
    RackPlan d = plan_rack(drill, route, false, a);
    CHECK(d.carry_out && !d.carry_in && d.first == 0 && d.slots.size() == 2 && d.slots[0] == std::make_pair(2, 1) && d.slots[1] == std::make_pair(1, 3));
    // no routing: nothing is carried, the places still go
    ProgramTools none;
    RackPlan alone = plan_rack(drill, none, false, a);
    CHECK(!alone.carry_out && alone.present());
    // the routing takes over the last drill (tool 1, in place 3) when the drilling ran chained, and its tools wait after the drills
    drill.done = true;
    drill.chained = true;
    RackPlan r = plan_rack(drill, route, true, a);
    CHECK(r.carry_in && !r.carry_out && r.first == 2 && r.carry_slot == 3 && r.slots.size() == 1 && r.slots[0] == std::make_pair(1, 5));
    // the drilling did not run chained (the server did not plan it so): no carry
    ProgramTools plain = drill;
    plain.chained = false;
    RackPlan q = plan_rack(plain, route, true, a);
    CHECK(!q.carry_in && q.first == 0 && q.carry_slot == 0);

    // the request: the first version has neither field, the second carries the bar
    Json j;
    CHECK(parse_json(job_request("m1", false, "a.ncd", "M48\n", "", {}, 0, nullptr).body, j) && j.get("rack") == nullptr && j.get("request_version") == nullptr);
    const RackPlan empty;
    CHECK(!empty.present() && job_request("m1", false, "a.ncd", "M48\n", "", {}, 0, &empty).body == job_request("m1", false, "a.ncd", "M48\n", "").body);
    CHECK(parse_json(job_request("m1", true, "a.plt", "IN;", "", {}, 0, &r).body, j));
    const Json* rk = j.get("rack");
    CHECK(j.num("request_version") == 2 && rk != nullptr && rk->num("first") == 2 && rk->get("carry_in")->b && !rk->get("carry_out")->b && rk->num("carry_slot") == 3);
    CHECK(rk->get("slots")->a.size() == 1 && rk->get("slots")->a[0].num("tool") == 1 && rk->get("slots")->a[0].num("place") == 5);
    CHECK(parse_rack_applied("{\"id\":\"j\",\"rack_applied\":true}") && !parse_rack_applied("{\"id\":\"j\",\"rack_applied\":false}"));
    CHECK(!parse_rack_applied("{\"id\":\"j\"}") && !parse_rack_applied("not json") && !parse_rack_applied("{\"rack_applied\":\"yes\"}"));
}

// ---- the machine with the tool changer in the workflow ----------------------------------------------------------------------------------

static void drilling_then_routing_on_the_tool_changer() {
    std::string why;
    Json j;
    Workflow w;
    CHECK(w.choose("board.NCD", kDrill, why) && w.choose("outline.PLT", kRoute, why));
    // before the fleet says the model nothing is known about the changer: the request is the one of the first version, and the places are not checked
    CHECK(!w.atc() && w.rack_problems().empty());
    CHECK(parse_json(w.create_request("m1").body, j) && j.get("rack") == nullptr);
    w.on_fleet(fleet_with("", "ccd-atc"), "m1");
    CHECK(w.atc());
    // the places were given in order: the drills small to big, then the pens
    CHECK(w.rack_slot(false, 1) == 1 && w.rack_slot(false, 2) == 2 && w.rack_slot(false, 3) == 3 && w.rack_slot(true, 1) == 4 && w.rack_slot(true, 2) == 5);
    CHECK(w.rack_problems().empty() && w.rack_tools().size() == 5);
    // the drilling request carries the places in the order of use and leaves its last tool in the chuck
    CHECK(parse_json(w.create_request("m1").body, j) && j.num("request_version") == 2);
    const Json* rk = j.get("rack");
    CHECK(rk != nullptr && rk->num("first") == 0 && !rk->get("carry_in")->b && rk->get("carry_out")->b && rk->get("slots")->a.size() == 3);
    CHECK(rk->get("slots")->a[0].num("tool") == 2 && rk->get("slots")->a[0].num("place") == 2 && rk->get("slots")->a[2].num("tool") == 3);
    // the server planned with it
    CHECK(w.on_created(answer(201, "{\"id\":\"d1\",\"rack_applied\":true}"), why) && w.rack_reply() == Workflow::RackReply::Applied);
    CHECK(w.on_started(answer(200, "{}"), why));
    // the squares of the bar while the drilling is at its last tool
    w.on_fleet(fleet_with("d1", "ccd-atc", 3, 4), "m1");
    std::vector<BarTool> bar = w.rack_bar();
    CHECK(bar.size() == 5 && bar[0].tool == 2 && bar[0].state == BarState::Done && bar[1].tool == 1 && bar[1].state == BarState::Done);
    CHECK(bar[2].tool == 3 && bar[2].state == BarState::InUse && bar[2].slot == 3 && bar[3].route && bar[3].state == BarState::Pending);
    // the place of a tool cannot be changed while the job runs
    CHECK(!w.set_rack_place(false, 1, 7) && w.rack_slot(false, 1) == 1);
    w.on_fleet(fleet_with("", "ccd-atc"), "m1");
    CHECK(w.stage() == Stage::Finished && w.want_report());
    CHECK(w.on_report(answer(200, "{\"state\":\"done\",\"holes_done\":4,\"holes_total\":4}")));
    // the drilling is done but the routing comes next: that is a stage, not the end of the job
    CHECK(w.ended_well() && !w.job_complete() && w.stages_done() == 0 && w.next_name() == "outline.PLT");
    CHECK(w.advance() && w.stages_done() == 1 && w.stage_next_name() == "outline.PLT" && w.route());
    // the routing takes over the last drill (tool 3, in place 3) and its pens wait after the three drills
    CHECK(parse_json(w.create_request("m1").body, j));
    rk = j.get("rack");
    CHECK(rk != nullptr && rk->num("first") == 3 && rk->get("carry_in")->b && !rk->get("carry_out")->b && rk->num("carry_slot") == 3);
    CHECK(rk->get("slots")->a.size() == 2 && rk->get("slots")->a[0].num("place") == 4 && rk->get("slots")->a[1].num("place") == 5);
    CHECK(w.on_created(answer(201, "{\"id\":\"r1\",\"rack_applied\":true}"), why) && w.on_started(answer(200, "{}"), why));
    w.on_fleet(fleet_with("r1", "ccd-atc", 1, 2), "m1");
    w.on_fleet(fleet_with("", "ccd-atc"), "m1");
    CHECK(w.on_report(answer(200, "{\"state\":\"done\"}")));
    // now the whole job is over: the board can come out
    CHECK(w.ended_well() && w.job_complete() && w.stages_done() == 1);
}

static void a_server_that_does_not_chain() {
    std::string why;
    Json j;
    Workflow w;
    CHECK(w.choose("board.NCD", kDrill, why) && w.choose("outline.PLT", kRoute, why));
    w.on_fleet(fleet_with("", "ccd-atc"), "m1");
    // the server makes the job but does not say it used the bar: the tools go back to their places after their work, and the screen says so
    CHECK(w.on_created(answer(201, "{\"id\":\"d1\"}"), why) && w.rack_reply() == Workflow::RackReply::NotApplied);
    CHECK(w.on_started(answer(200, "{}"), why));
    w.on_fleet(fleet_with("d1", "ccd-atc"), "m1");
    w.on_fleet(fleet_with("", "ccd-atc"), "m1");
    CHECK(w.on_report(answer(200, "{\"state\":\"done\"}")) && w.advance());
    CHECK(parse_json(w.create_request("m1").body, j));
    const Json* rk = j.get("rack");
    CHECK(rk != nullptr && rk->num("first") == 0 && !rk->get("carry_in")->b && rk->get("slots")->a.size() == 2);
    CHECK(w.rack_reply() == Workflow::RackReply::NotSent);

    // a server that turns the bar down with 400 or 422 gets the job again without it
    Workflow v;
    CHECK(v.choose("board.NCD", kDrill, why));
    v.on_fleet(fleet_with("", "ccd-atc"), "m1");
    CHECK(parse_json(v.create_request("m1").body, j) && j.get("rack") != nullptr);
    CHECK(!v.on_created(answer(422, "{\"error\":\"unknown_field\"}"), why) && v.take_rack_retry() && !v.take_rack_retry());
    CHECK(parse_json(v.create_request("m1").body, j) && j.get("rack") == nullptr && j.get("request_version") == nullptr);
    // an error that is not about the field does not drop it
    Workflow u;
    CHECK(u.choose("board.NCD", kDrill, why));
    u.on_fleet(fleet_with("", "ccd-atc"), "m1");
    CHECK(!u.on_created(answer(500, "{}"), why) && !u.take_rack_retry());
    CHECK(parse_json(u.create_request("m1").body, j) && j.get("rack") != nullptr);
    // the switch of the settings: no bar goes
    std::vector<Problem> pr;
    Settings off = parse_settings("rack_chain = off\n", pr);
    Workflow t;
    t.configure_rack(off);
    CHECK(t.choose("board.NCD", kDrill, why));
    t.on_fleet(fleet_with("", "ccd-atc"), "m1");
    CHECK(parse_json(t.create_request("m1").body, j) && j.get("rack") == nullptr);
    // a machine with the manual change gets none either
    Workflow m;
    CHECK(m.choose("board.NCD", kDrill, why));
    m.on_fleet(fleet_with("", "ccd-mtc"), "m1");
    CHECK(!m.atc() && parse_json(m.create_request("m1").body, j) && j.get("rack") == nullptr && m.rack_problems().empty());
}

static void the_start_waits_for_the_places() {
    std::string why;
    Workflow w;
    CHECK(w.choose("board.NCD", kDrill, why) && w.choose("outline.PLT", kRoute, why));
    w.on_fleet(fleet_with("", "ccd-atc"), "m1");
    CHECK(w.rack_problems().empty());
    // a place that two tools share
    CHECK(w.set_rack_place(false, 1, 2));
    std::vector<RackProblem> p = w.rack_problems();
    CHECK(p.size() == 1 && p[0].kind == RackProblem::Kind::Shared && p[0].tool == 1 && p[0].other_tool == 2 && p[0].slot == 2);
    const std::string text_en = rack_problem_text(p, Lang::En);
    CHECK(has(text_en, "Tool 1 @2") && has(text_en, "shares its place in the bar"));
    CHECK(has(rack_problem_text(p, Lang::Es), "comparte hueco en la barra") && has(rack_problem_text(p, Lang::Pt), "partilha o lugar na barra"));
    // a tool without a place
    CHECK(w.set_rack_place(false, 1, 0) && w.set_rack_place(true, 2, 0));
    p = w.rack_problems();
    CHECK(p.size() == 2 && p[0].kind == RackProblem::Kind::Unassigned && p[1].route && p[1].tool == 2);
    // a place the machine does not define (the setting says which it defines)
    std::vector<Problem> pr;
    Settings six = parse_settings("rack_places = 1-6\n", pr);
    CHECK(pr.empty());
    w.configure_rack(six);
    CHECK(w.rack_places().size() == 6);
    CHECK(w.set_rack_place(false, 1, 9) && w.set_rack_place(true, 2, 6));
    p = w.rack_problems();
    CHECK(p.size() == 1 && p[0].kind == RackProblem::Kind::Undefined && p[0].tool == 1 && p[0].slot == 9);
    CHECK(!w.set_rack_place(false, 77, 3));  // a tool the job does not have
    // a tool put in a free place that the machine defines
    CHECK(w.set_rack_place(false, 1, 1));
    CHECK(w.rack_problems().empty());
    // the check can be turned off (a bench setting): nothing stops the start
    Settings no_check = parse_settings("rack_check = off\n", pr);
    CHECK(w.set_rack_place(false, 1, 9));
    w.configure_rack(no_check);
    CHECK(w.rack_problems().empty());
    CHECK(w.set_rack_place(false, 1, 2) && w.set_rack_place(false, 2, 2) && w.rack_problems().empty());
}

// ---- the stages of a job ----------------------------------------------------------------------------------------------------------------

static void the_end_of_a_stage_is_not_the_end_of_the_job() {
    EndWatcher e;
    CHECK(e.update(0, false) == EndNotice::None);
    CHECK(e.update(1, false) == EndNotice::StageDone);
    CHECK(e.update(1, false) == EndNotice::None && e.update(1, false) == EndNotice::None);
    CHECK(e.update(1, true) == EndNotice::BoardOut);
    CHECK(e.update(1, true) == EndNotice::None);  // said once
    CHECK(e.update(1, false) == EndNotice::None);
    CHECK(e.update(2, false) == EndNotice::StageDone);  // the next job has its own
    CHECK(e.update(2, true) == EndNotice::BoardOut);
    e.reset();
    CHECK(e.update(0, true) == EndNotice::BoardOut);
    CHECK(has(stage_text("outline.PLT", Lang::En), "The drilling is done and the routing comes next") && has(stage_text("outline.PLT", Lang::En), "outline.PLT"));
    CHECK(has(stage_text("", Lang::Es), "fresado") && !has(stage_text("", Lang::Es), "\n"));

    // a job of one program is complete when it ends well
    std::string why;
    Workflow w;
    CHECK(w.choose("board.NCD", kDrill, why) && w.on_created(answer(201, "{\"id\":\"d1\"}"), why) && w.on_started(answer(200, "{}"), why));
    w.on_fleet(fleet_with("d1", "ccd-mtc"), "m1");
    w.on_fleet(fleet_with("", "ccd-mtc"), "m1");
    CHECK(!w.job_complete());  // the server has not said how it ended
    CHECK(w.on_report(answer(200, "{\"state\":\"done\"}")) && w.job_complete() && w.stages_done() == 0);
    // a job that was cancelled is neither a stage nor complete
    Workflow c;
    CHECK(c.choose("board.NCD", kDrill, why) && c.choose("outline.PLT", kRoute, why));
    CHECK(c.on_created(answer(201, "{\"id\":\"d1\"}"), why) && c.on_started(answer(200, "{}"), why));
    c.on_fleet(fleet_with("d1", "ccd-mtc"), "m1");
    c.on_fleet(fleet_with("", "ccd-mtc"), "m1");
    CHECK(c.on_report(answer(200, "{\"state\":\"cancelled\",\"holes_done\":1,\"holes_total\":4}")) && !c.job_complete() && !c.advance() && c.stages_done() == 0);
    // the notice of a stage is its own file
    NoticeSetting voice;
    CHECK(parse_notice("voice:es:f", voice) && notice_file("/sdcard", voice, Cue::Stage) == "/sdcard/voices/es/f/stage.wav");
}

// ---- the settings of the bar and the stages ----------------------------------------------------------------------------------------------

static void the_settings_of_the_bar_and_the_stages() {
    std::vector<Problem> p;
    const Settings d = parse_settings("", p);
    CHECK(p.empty());
    // every safety default is the conservative one
    CHECK(d.rack_check && d.rack_chain && d.stage_window);
    CHECK(d.rack_places == "1-14" && d.sound_stage.kind == NoticeSetting::Kind::Off);

    const Settings s = parse_settings("rack_places = 1-6,9\nrack_check = off\nrack_chain = off\nstage_window = off\nsound_stage = voice:es:f\n", p);
    CHECK(p.empty());
    CHECK(s.rack_places == "1-6,9" && !s.rack_check && !s.rack_chain && !s.stage_window);
    CHECK(s.sound_stage.kind == NoticeSetting::Kind::Voice && s.sound_stage.lang == Lang::Es && s.sound_stage.female);

    // a value that cannot be used is reported and the default stays: a typing mistake never opens a switch
    p.clear();
    const Settings bad = parse_settings("rack_places = 20\nrack_places = x\nrack_check = perhaps\nrack_chain = 2\nstage_window = no no\nsound_stage = voice:xx:f\n", p);
    CHECK(p.size() == 6);
    CHECK(bad.rack_places == "1-14" && bad.rack_check && bad.rack_chain && bad.stage_window && bad.sound_stage.kind == NoticeSetting::Kind::Off);
    CHECK(p[0].key == "rack_places" && p[0].reason == CfgReason::Range && p[2].reason == CfgReason::OnOff);
}

int main() {
    places_of_the_bar();
    chaining_the_programs();
    drilling_then_routing_on_the_tool_changer();
    a_server_that_does_not_chain();
    the_start_waits_for_the_places();
    the_end_of_a_stage_is_not_the_end_of_the_job();
    the_settings_of_the_bar_and_the_stages();
    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
