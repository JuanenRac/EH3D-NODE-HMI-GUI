// EH3D-NODE-HMI-GUI - host tests of the orientation, the files of the microSD card and the client of the server.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../core/hmi_config.hpp"
#include "../core/job_memory.hpp"
#include "../core/orientation.hpp"
#include "../core/sdfiles.hpp"
#include "../core/server_link.hpp"
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

static void orientation_rules() {
    Orientation o;
    CHECK(orientation_from_text("portrait", o) && o == Orientation::Portrait);
    CHECK(!orientation_from_text("sideways", o));
    CHECK(std::string(orientation_text(Orientation::Auto)) == "auto");
    Layout land = layout_for(Orientation::Landscape, true);  // the setting wins over the way the module is mounted
    CHECK(!land.portrait && land.width == 1280 && land.height == 800 && land.columns == 3 && land.rotation == 90);
    Layout port = layout_for(Orientation::Portrait, false);
    CHECK(port.portrait && port.width == 800 && port.height == 1280 && port.columns == 1 && port.rotation == 0);
    CHECK(layout_for(Orientation::Auto, true).portrait);
    CHECK(!layout_for(Orientation::Auto, false).portrait);
    CHECK(column_width(land, 14) == (1280 - 14 * 4) / 3);
    // a touch lands where the screen is touched
    int x = -1, y = -1;
    touch_to_screen(port, 5, 7, x, y);
    CHECK(x == 5 && y == 7);
    touch_to_screen(land, 0, 0, x, y);
    CHECK(x == 0 && y == 799);
    touch_to_screen(land, 799, 1279, x, y);
    CHECK(x == 1279 && y == 0);

    std::vector<Problem> p;
    Settings s = parse_settings("orientation = portrait\npoll_s = 10\n", p);
    CHECK(p.empty() && s.orientation == Orientation::Portrait && s.poll_s == 10);
    p.clear();
    s = parse_settings("orientation = diagonal\npoll_s = 0\n", p);
    CHECK(p.size() == 2 && s.orientation == Orientation::Auto && s.poll_s == 3);
}

static void sd_files() {
    CHECK(kind_of_file("BOARD.NCD") == FileKind::Drill);
    CHECK(kind_of_file("outline.plt") == FileKind::Route);
    CHECK(kind_of_file("top.GKO") == FileKind::Gerber);
    CHECK(kind_of_file("drawing.dxf") == FileKind::Dxf);
    CHECK(kind_of_file("panel.zip") == FileKind::Archive);
    CHECK(kind_of_file("cut.nc") == FileKind::Gcode);
    CHECK(kind_of_file("hmi.cfg") == FileKind::Settings && !is_job_file("hmi.cfg"));
    CHECK(kind_of_file("notes.txt") == FileKind::Other && !is_job_file("notes.txt"));
    CHECK(kind_of_file("noext") == FileKind::Other && kind_of_file(".ncd") == FileKind::Other);
    CHECK(is_job_file("a.ncd"));

    // the screen writes only plain names inside the card
    CHECK(safe_file_name("report-1.txt"));
    for (const char* bad : {"", ".hidden", "../x", "a/b", "a\\b", "c:x", "a*b", "a?b", "a|b", "ends.", "tab\tname"}) CHECK(!safe_file_name(bad));
    CHECK(!safe_file_name(std::string(65, 'a')));
    CHECK(sd_path("a.ncd") == "/sdcard/a.ncd");
    CHECK(sd_path("../etc/passwd").empty());

    std::vector<Entry> list = {{"b.ncd", false, 10}, {"Zeta", true, 0}, {"a.plt", false, 5}, {"..", true, 0}, {"alpha", true, 0}, {"A.ncd", false, 1}};
    list = sorted_listing(list);
    CHECK(list.size() == 5 && list[0].name == "alpha" && list[1].name == "Zeta");
    CHECK(list[2].name == "A.ncd" && list[3].name == "a.plt" && list[4].name == "b.ncd");
    CHECK(size_text(812) == "812 B" && size_text(12698) == "12.4 KB" && size_text(3250586) == "3.1 MB");
    CHECK(free_name("report", "txt", [](const std::string&) { return false; }) == "report.txt");
    CHECK(free_name("report", "txt", [](const std::string& n) { return n != "report-3.txt"; }) == "report-3.txt");
}

static const char* kFleet =
    "{\"stations\":[{\"id\":\"bench\",\"machines\":[\"m1\",\"m2\"]}],\"machines\":["
    "{\"id\":\"m1\",\"name\":\"M\\u00e1quina 1\",\"station\":\"bench\",\"state\":\"waiting\","
    "\"job\":{\"id\":\"j9\",\"name\":\"board\",\"percent\":82,\"pause\":{\"kind\":\"tool_change\",\"tool\":3}},\"ready\":0},"
    "{\"id\":\"m2\",\"name\":\"M2\",\"station\":\"bench\",\"state\":\"idle\",\"job\":null,\"ready\":2}],"
    "\"summary\":{\"machines\":2}}";

static void server_client() {
    // the JSON reader
    Json j;
    CHECK(parse_json("{\"a\":[1,2.5,-3e2,true,false,null],\"s\":\"x\\ny\\\"z\\u20ac\"}", j));
    CHECK(j.get("a")->a.size() == 6 && j.get("a")->a[2].n == -300.0);
    CHECK(j.str("s") == "x\ny\"z\xE2\x82\xAC");
    for (const char* bad : {"", "{", "{\"a\":}", "[1,]", "{\"a\" 1}", "tru", "\"open", "{} extra", "[1 2]"}) CHECK(!parse_json(bad, j));
    CHECK(!parse_json(std::string(40, '[') + std::string(40, ']'), j));  // nested too deep
    CHECK(json_quote("a\"b\\c\n") == "\"a\\\"b\\\\c\\n\"");

    // the requests
    std::string w = wire(fleet_request(), "192.168.0.180:18200", "tok-1234");
    CHECK(w.rfind("GET /api/v1/fleet HTTP/1.1\r\n", 0) == 0);
    CHECK(w.find("Authorization: Bearer tok-1234\r\n") != std::string::npos);
    CHECK(w.find("Content-Length") == std::string::npos);
    Request job = job_request("m1", false, "board 1", "M48\nT01\n", "T01C0.8\n");
    CHECK(job.path == "/api/v1/machines/m1/jobs" && job.method == "POST");
    Json body;
    CHECK(parse_json(job.body, body) && body.str("kind") == "drill" && body.str("name") == "board 1");
    CHECK(body.get("files")->str("ncd") == "M48\nT01\n" && body.get("files")->str("dtd") == "T01C0.8\n");
    CHECK(parse_json(job_request("m1", true, "outline", "IN;", "").body, body) && body.str("kind") == "route" && body.get("files")->get("rtd") == nullptr);
    CHECK(job_start_request("j9").path == "/api/v1/jobs/j9/start" && job_cancel_request("j9").method == "POST");
    CHECK(job_confirm_request("j9").path == "/api/v1/jobs/j9/confirm");
    CHECK(wire(job, "h", "t").find("Content-Length: " + std::to_string(job.body.size())) != std::string::npos);

    // the answers: whole, cut and refused
    std::string reply = std::string("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: ") + std::to_string(std::strlen(kFleet)) + "\r\n\r\n" + kFleet;
    Response r;
    CHECK(!parse_response(reply.substr(0, reply.size() - 5), r));  // the body is not whole yet
    CHECK(!parse_response("HTTP/1.1 200 OK\r\nContent-Le", r));
    CHECK(!parse_response("garbage\r\n\r\n", r));
    CHECK(parse_response(reply + "trailing", r) && r.status == 200 && r.body == kFleet);
    FleetView f;
    CHECK(parse_fleet(r.body, f) && f.machines.size() == 2);
    CHECK(f.machines[0].name == "M\xC3\xA1quina 1" && f.machines[0].state == "waiting" && f.machines[0].percent == 82);
    CHECK(f.machines[0].pause_kind == "tool_change" && f.machines[0].job_id == "j9");
    CHECK(f.machines[1].percent == -1 && f.machines[1].ready == 2 && f.machines[1].station == "bench");
    CHECK(!parse_fleet("{\"machines\":[{\"name\":\"no id\"}]}", f));
    CHECK(!parse_fleet("{\"stations\":[]}", f) && !parse_fleet("[]", f));
    Response denied;
    const char* refusal = "{\"error\":\"unauthorized\",\"detail\":\"no token given\"}";
    CHECK(parse_response(std::string("HTTP/1.1 401 Unauthorized\r\nContent-Length: ") + std::to_string(std::strlen(refusal)) + "\r\n\r\n" + refusal, denied));
    CHECK(denied.status == 401 && error_line(denied) == "unauthorized: no token given");
    denied.body = "<html>";
    CHECK(error_line(denied) == "HTTP 401");
}

static hmi::Response ok(int status, const std::string& body) { return {status, body}; }

static FleetView fleet_of(const std::string& job_id, const std::string& state, int percent, const std::string& pause) {
    FleetView f;
    MachineRow m;
    m.id = "m1";
    m.state = state;
    if (!job_id.empty()) {
        m.job_id = job_id;
        m.percent = percent;
        m.pause_kind = pause;
    }
    f.machines.push_back(m);
    return f;
}

static void the_flow_of_a_job() {
    Workflow w;
    std::string why;
    CHECK(w.stage() == Stage::Nothing && w.create_request("m1").path.empty());
    // a file that is not a job, an empty file
    CHECK(!w.choose("notes.txt", "x", why) && !why.empty());
    CHECK(!w.choose("a.ncd", "", why));
    CHECK(!w.choose("top.gbr", "G04*", why));  // a Gerber file is opened by the desktop programs, not sent from the card
    CHECK(w.stage() == Stage::Nothing);

    CHECK(w.choose("board.NCD", "M48\n", why) && w.stage() == Stage::FileChosen);
    CHECK(w.create_request("").path.empty());  // no machine set in the settings
    Request create = w.create_request("m1");
    CHECK(create.path == "/api/v1/machines/m1/jobs");
    CHECK(w.start_request().path.empty());  // nothing starts before the server has the job ready

    // the server refuses, then accepts
    CHECK(!w.on_created(ok(422, "{\"error\":\"unknown_tool\",\"detail\":\"tool 7\"}"), why) && why == "unknown_tool: tool 7");
    CHECK(w.stage() == Stage::FileChosen);
    CHECK(w.on_created(ok(201, "{\"id\":\"j42\",\"state\":\"ready\"}"), why) && w.stage() == Stage::Ready && w.job_id() == "j42");
    CHECK(w.report_base() == "board-job");
    CHECK(w.confirm_request().path.empty());

    // started by the operator (the gate decides before this is sent); a busy machine refuses
    CHECK(w.start_request().path == "/api/v1/jobs/j42/start");
    CHECK(!w.on_started(ok(409, "{\"error\":\"machine_busy\"}"), why) && why == "machine_busy" && w.stage() == Stage::Ready);
    CHECK(w.on_started(ok(200, "{}"), why) && w.stage() == Stage::Running);

    // the fleet shows the job, from this screen or from another program
    w.on_fleet(fleet_of("j42", "busy", 40, ""), "m1");
    CHECK(w.stage() == Stage::Running && w.percent() == 40);
    w.on_fleet(fleet_of("j42", "waiting", 41, "tool_change"), "m1");
    CHECK(w.stage() == Stage::Waiting && w.pause() == "tool_change" && w.confirm_request().path == "/api/v1/jobs/j42/confirm");
    w.on_fleet(fleet_of("j42", "busy", 60, ""), "m1");
    CHECK(w.stage() == Stage::Running && w.pause().empty());
    w.on_fleet(fleet_of("other", "busy", 5, ""), "other-machine");  // another machine: nothing changes
    CHECK(w.stage() == Stage::Running);
    w.on_fleet(fleet_of("", "idle", 0, ""), "m1");  // the machine has no job: ours is over
    CHECK(w.stage() == Stage::Finished && w.percent() == 100);
    CHECK(w.cancel_request().path.empty());

    // a new file starts the flow again
    CHECK(w.choose("outline.plt", "IN;", why) && w.stage() == Stage::FileChosen && w.job_id().empty());
    Json b;
    CHECK(parse_json(w.create_request("m1").body, b) && b.str("kind") == "route");
}

static void a_job_is_followed_again_after_a_reset() {
    // what is kept is the id, the machine and the name; a damaged text follows nothing
    JobMemory m{"j42", "m1", "board.NCD"}, back;
    CHECK(memory_parse(memory_text(m), back) && back.job_id == "j42" && back.machine == "m1" && back.name == "board.NCD");
    CHECK(!memory_parse("", back) && !memory_parse("machine=m1\nname=x\n", back) && !memory_parse("job=\n", back));
    JobMemory sneaky{"j1", "m1", "a\njob=other"};
    CHECK(memory_parse(memory_text(sneaky), back) && back.job_id == "j1");
    CHECK(!memory_parse(std::string("job=") + std::string(80, 'x') + "\n", back));

    // after the reset the screen adopts it and the next reading of the fleet says what it is
    Workflow w;
    CHECK(!w.adopt("", "x") && w.stage() == Stage::Nothing);
    CHECK(w.adopt("j42", "board.NCD") && w.stage() == Stage::Running && w.job_id() == "j42" && w.name() == "board.NCD");
    CHECK(w.cancel_request().path == "/api/v1/jobs/j42/cancel");  // the operator may leave it
    w.on_fleet(fleet_of("j42", "waiting", 41, "tool_change"), "m1");
    CHECK(w.stage() == Stage::Waiting && w.confirm_request().path == "/api/v1/jobs/j42/confirm");
    // or the server no longer has it on the machine: it is over
    Workflow v;
    CHECK(v.adopt("j7", "x.PLT"));
    v.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(v.stage() == Stage::Finished);
}

static void tools_of_a_file_and_the_spindle() {
    // the summary of a drilling file: the tools declared, their diameters (inches are turned into mm) and the holes of each
    const std::string drl = "M48\nMETRIC,TZ\nT1C0.8\nT02C1.0\nT3C3.0\n%\nT1\nX1.0Y1.0\nX2.0Y1.0\nT2\nX3.0Y3.0\nT0\nM30\n";
    FileSummary s = summarize_drill(drl);
    CHECK(s.tools.size() == 3 && s.total == 3 && !s.route);
    CHECK(s.tools[0].number == 1 && s.tools[0].diameter_mm == 0.8 && s.tools[0].count == 2);
    CHECK(s.tools[1].number == 2 && s.tools[1].count == 1 && s.tools[2].number == 3 && s.tools[2].count == 0);
    FileSummary inch = summarize_drill("M48\nINCH,LZ\nT1C0.0315\n%\nT1\nX1Y1\n");
    CHECK(inch.tools.size() == 1 && inch.tools[0].diameter_mm > 0.79 && inch.tools[0].diameter_mm < 0.81 && inch.total == 1);
    CHECK(summarize_drill("").tools.empty() && summarize_drill("garbage\n\n").total == 0);

    // a routing file: the pens and their cuts
    FileSummary r = summarize_route("IN;SP1;PU0,0;PD10,0,10,10;PU;SP2;PU5,5;PD6,6;PU;PD7,7;SP0;");
    CHECK(r.route && r.tools.size() == 2 && r.tools[0].number == 1 && r.tools[0].count == 1 && r.tools[1].number == 2 && r.tools[1].count == 2 && r.total == 3);
    CHECK(summarize_route("IN;PU0,0;PD1,1;").tools.empty());  // no pen taken: nothing to switch

    // switching tools off
    ToolSwitch sw;
    sw.reset(s);
    CHECK(sw.off().empty() && sw.remaining() == 3 && !sw.nothing_left());
    CHECK(sw.toggle(2) && sw.is_off(2) && sw.remaining() == 2 && sw.off().size() == 1);
    CHECK(!sw.toggle(9));  // the file has no such tool
    CHECK(sw.toggle(2) && !sw.is_off(2) && sw.off().empty());
    CHECK(sw.toggle(1) && sw.toggle(2) && sw.nothing_left() && sw.remaining() == 0);

    // the flow: the switches go to the server as `skip_tools`, and a job with nothing left is not sent
    Workflow w;
    std::string why;
    CHECK(w.choose("board.NCD", drl, why) && w.tool_switch().summary().tools.size() == 3);
    Json b;
    CHECK(parse_json(w.create_request("m1").body, b) && b.get("skip_tools") == nullptr);  // all on: the request is the same as before
    CHECK(w.toggle_tool(3) && w.toggle_tool(1));
    CHECK(parse_json(w.create_request("m1").body, b) && b.get("skip_tools") && b.get("skip_tools")->a.size() == 2 && b.get("skip_tools")->a[0].n == 1 && b.get("skip_tools")->a[1].n == 3);
    CHECK(w.toggle_tool(2) && w.nothing_to_do() && w.create_request("m1").path.empty());
    CHECK(w.toggle_tool(2) && !w.nothing_to_do() && !w.create_request("m1").path.empty());
    CHECK(w.choose("other.NCD", drl, why) && w.tool_switch().off().empty());  // a new file starts with every tool on
    CHECK(w.on_created(ok(201, "{\"id\":\"j1\"}"), why) && !w.toggle_tool(1));  // once the job is made it is the server's

    // the spindle of the machine
    const char* spindle = "{\"spindle\":\"vfd\",\"guard\":\"reject\",\"limits\":{},\"active\":{\"min_rpm\":6000,\"max_rpm\":24000,\"max_bit_mm\":6},\"node_max_rpm\":24000}";
    SpindleView v;
    CHECK(spindle_request("m1").path == "/api/v1/machines/m1/spindle" && spindle_request("m1").method == "GET");
    CHECK(parse_spindle(spindle, v) && v.known && v.kind == "vfd" && v.min_rpm == 6000 && v.max_rpm == 24000 && v.guard == "reject");
    CHECK(spindle_ceiling(60000, v) == 24000 && spindle_ceiling(12000, v) == 12000 && spindle_ceiling(0, v) == 0);
    CHECK(parse_spindle("{\"spindle\":\"\",\"guard\":\"reject\",\"active\":null,\"node_max_rpm\":null}", v) && !v.known && spindle_ceiling(24000, v) == 24000);
    CHECK(!parse_spindle("[]", v) && !parse_spindle("nope", v));
}

static void a_stopped_job_goes_on_and_two_programs_follow_each_other() {
    const std::string drl = "M48\nMETRIC,TZ\nT1C0.8\nT2C1.0\n%\nT1\nX1Y1\nX2Y1\nT2\nX3Y3\nX4Y4\nT0\nM30\n";
    const std::string plt = "IN;SP1;PU0,0;PD10,0,10,10;PU;SP2;PU5,5;PD6,6;PU;";
    std::string why;

    // the report of the server about a job: how it ended and what it did
    JobReport jr;
    CHECK(job_report_request("j1").path == "/api/v1/jobs/j1" && job_report_request("j1").method == "GET");
    CHECK(parse_job_report("{\"state\":\"cancelled\",\"message\":\"\",\"holes_done\":2,\"holes_total\":4,\"items_done\":9}", jr) && jr.state == "cancelled" && jr.holes_done == 2 && jr.holes_total == 4);
    CHECK(parse_job_report("{\"state\":\"done\",\"holes_done\":5,\"holes_total\":3}", jr) && jr.holes_total == 5);
    CHECK(!parse_job_report("{\"id\":\"x\"}", jr) && !parse_job_report("[]", jr));
    // the job carries the holes that are already done
    Json b;
    CHECK(parse_json(job_request("m1", false, "a.ncd", "M48\n", "", {}, 0).body, b) && b.get("skip") == nullptr);
    CHECK(parse_json(job_request("m1", false, "a.ncd", "M48\n", "", {2}, 3).body, b) && b.get("skip") && b.get("skip")->n == 3 && b.get("skip_tools")->a.size() == 1);

    // a drilling job is started, the operator stops it after two holes: it ends as stopped, with the holes it did
    Workflow w;
    CHECK(w.choose("board.NCD", drl, why) && w.on_created(ok(201, "{\"id\":\"j1\"}"), why) && w.on_started(ok(200, "{}"), why));
    CHECK(!w.want_report());
    w.on_fleet(fleet_of("j1", "busy", 50, ""), "m1");
    w.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(w.stage() == Stage::Finished && w.want_report() && w.report_request().path == "/api/v1/jobs/j1");
    CHECK(w.on_report(ok(200, "{\"state\":\"cancelled\",\"message\":\"stopped\",\"holes_done\":2,\"holes_total\":4}")));
    CHECK(w.stage() == Stage::Stopped && !w.want_report() && w.done() == 2 && w.total() == 4 && w.can_resume() && w.message() == "stopped");
    CHECK(w.cancel_request().path.empty() && !w.advance());
    // go on: the same file is sent again leaving out the two holes that are done, and once more after a second stop
    CHECK(w.resume(why) && w.stage() == Stage::FileChosen && w.skipped() == 2);
    CHECK(parse_json(w.create_request("m1").body, b) && b.get("skip") && b.get("skip")->n == 2 && b.str("kind") == "drill");
    CHECK(w.on_created(ok(201, "{\"id\":\"j2\"}"), why) && w.on_started(ok(200, "{}"), why));
    w.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(w.on_report(ok(200, "{\"state\":\"failed\",\"message\":\"alarm\",\"holes_done\":1,\"holes_total\":2}")) && w.stage() == Stage::Stopped);
    CHECK(w.resume(why) && w.skipped() == 3 && parse_json(w.create_request("m1").body, b) && b.get("skip")->n == 3);
    // the tools switched off stay off when the job goes on
    CHECK(w.stage() == Stage::FileChosen);

    // a job stopped before any hole cannot go on (it is sent again from the start), and one that was not stopped cannot either
    Workflow z;
    CHECK(z.choose("board.NCD", drl, why) && z.on_created(ok(201, "{\"id\":\"j3\"}"), why) && z.on_started(ok(200, "{}"), why));
    CHECK(!z.can_resume() && !z.resume(why) && !why.empty());
    z.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(z.on_report(ok(200, "{\"state\":\"cancelled\",\"holes_done\":0,\"holes_total\":4}")) && z.stage() == Stage::Stopped && !z.can_resume());
    // a report that cannot be read or comes late leaves the job finished and stops the asking
    Workflow y;
    CHECK(y.choose("board.NCD", drl, why) && y.on_created(ok(201, "{\"id\":\"j4\"}"), why) && y.on_started(ok(200, "{}"), why));
    y.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(!y.on_report(ok(502, "")) && y.stage() == Stage::Finished && !y.want_report() && !y.can_resume());
    // a job followed again after a reset has no file: it can only go on through the server (see the test of resume-from), not by sending a file again
    Workflow a;
    CHECK(a.adopt("j9", "board.NCD"));
    a.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(a.on_report(ok(200, "{\"state\":\"cancelled\",\"holes_done\":3,\"holes_total\":4}")) && a.stage() == Stage::Stopped && a.can_resume() && a.resumes_by_server());

    // two programs: the drilling first whatever the order the files were chosen in, then the routing
    Workflow c;
    CHECK(c.choose("outline.plt", plt, why) && c.route() && c.next_name().empty());
    CHECK(c.choose("board.NCD", drl, why) && !c.route() && c.name() == "board.NCD" && c.next_name() == "outline.plt");
    c.set_tools("T1C0.8\n");  // the table goes with the drilling file just chosen
    CHECK(parse_json(c.create_request("m1").body, b) && b.str("kind") == "drill" && b.get("files")->str("dtd") == "T1C0.8\n");
    CHECK(!c.advance());  // nothing has run yet
    CHECK(c.on_created(ok(201, "{\"id\":\"d1\"}"), why) && c.on_started(ok(200, "{}"), why));
    c.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(!c.advance());  // it ended, but the server has not said it ended well
    CHECK(c.on_report(ok(200, "{\"state\":\"done\",\"holes_done\":4,\"holes_total\":4}")) && c.stage() == Stage::Finished);
    CHECK(c.advance() && c.stage() == Stage::FileChosen && c.route() && c.name() == "outline.plt" && c.job_id().empty() && c.next_name().empty() && c.percent() == 0);
    CHECK(parse_json(c.create_request("m1").body, b) && b.str("kind") == "route" && b.get("skip") == nullptr);
    CHECK(c.on_created(ok(201, "{\"id\":\"r1\"}"), why) && c.start_request().path == "/api/v1/jobs/r1/start");  // started by the operator through the gate
    CHECK(c.on_started(ok(200, "{}"), why));
    c.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(c.on_report(ok(200, "{\"state\":\"done\",\"holes_done\":3,\"holes_total\":3}")) && !c.advance());  // the end of the job
    // the routing can be stopped and go on too
    Workflow r;
    CHECK(r.choose("outline.plt", plt, why) && r.on_created(ok(201, "{\"id\":\"r2\"}"), why) && r.on_started(ok(200, "{}"), why));
    r.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(r.on_report(ok(200, "{\"state\":\"cancelled\",\"holes_done\":1,\"holes_total\":3}")) && r.resume(why));
    CHECK(parse_json(r.create_request("m1").body, b) && b.str("kind") == "route" && b.get("skip")->n == 1);

    // a drilling file whose tools are all off: the routing is next
    Workflow e;
    CHECK(e.choose("board.NCD", drl, why) && e.choose("outline.plt", plt, why));
    CHECK(e.toggle_tool(1) && e.toggle_tool(2) && e.nothing_to_do() && e.create_request("m1").path.empty());
    CHECK(e.skip_empty() && e.route() && !e.nothing_to_do() && !e.create_request("m1").path.empty());
    Workflow f;
    CHECK(f.choose("board.NCD", drl, why) && f.toggle_tool(1) && f.toggle_tool(2) && !f.skip_empty());  // no program after it
    // choosing a file once a job was sent starts all over again
    CHECK(c.choose("another.NCD", drl, why) && c.next_name().empty() && !c.route() && c.skipped() == 0);
}

static void a_stopped_job_goes_on_through_the_server() {
    const std::string drl = "M48\nMETRIC,TZ\nT1C0.8\nT2C1.0\n%\nT1\nX1Y1\nX2Y1\nT2\nX3Y3\nX4Y4\nT0\nM30\n";
    std::string why;
    Json b;
    // the request is the route of the server, with no file in it
    CHECK(job_resume_from_request("j1").method == "POST" && job_resume_from_request("j1").path == "/api/v1/jobs/j1/resume-from" && job_resume_from_request("j1").body == "{}");
    JobReport jr;
    CHECK(parse_job_report("{\"state\":\"cancelled\",\"kind\":\"route\",\"holes_done\":1,\"holes_total\":3}", jr) && jr.kind == "route");

    // a job stopped after two holes: the server makes the new one, ready and not started, and says where it comes from
    Workflow w;
    CHECK(w.choose("board.NCD", drl, why) && w.on_created(ok(201, "{\"id\":\"j1\"}"), why) && w.on_started(ok(200, "{}"), why));
    CHECK(w.resume_request().path.empty());  // it is not stopped yet
    w.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(w.on_report(ok(200, "{\"state\":\"cancelled\",\"kind\":\"drill\",\"holes_done\":2,\"holes_total\":4}")) && w.can_resume() && w.resumes_by_server());
    Request q = w.resume_request();
    CHECK(q.method == "POST" && q.path == "/api/v1/jobs/j1/resume-from");
    CHECK(w.resume_request().path.empty());  // a second touch does not ask again while the answer is awaited
    CHECK(w.on_resume_from(ok(201, "{\"id\":\"j2\",\"state\":\"ready\",\"resumed_from\":\"j1\",\"kind\":\"drill\"}"), why) == ResumeStep::Made);
    CHECK(w.stage() == Stage::Ready && w.job_id() == "j2" && w.resumed_from() == "j1" && w.skipped() == 2 && !w.can_resume());
    CHECK(w.create_request("m1").path.empty());                         // nothing is sent again: the server has the file
    CHECK(w.start_request().path == "/api/v1/jobs/j2/start");           // the operator starts it, through the gate; nothing starts by itself
    CHECK(w.on_started(ok(200, "{}"), why) && w.stage() == Stage::Running);
    // a second stop goes on from the job in turn, and the sums add up
    w.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(w.on_report(ok(200, "{\"state\":\"failed\",\"holes_done\":1,\"holes_total\":2}")) && w.resume_request().path == "/api/v1/jobs/j2/resume-from");
    CHECK(w.on_resume_from(ok(200, "{\"id\":\"j3\",\"resumed_from\":\"j2\"}"), why) == ResumeStep::Made && w.skipped() == 3 && w.resumed_from() == "j2" && w.job_id() == "j3");
    // choosing another file forgets where the job came from
    CHECK(w.choose("other.NCD", drl, why) && w.resumed_from().empty());

    // the link did not get through: it may be asked again
    Workflow n;
    CHECK(n.choose("board.NCD", drl, why) && n.on_created(ok(201, "{\"id\":\"j4\"}"), why) && n.on_started(ok(200, "{}"), why));
    n.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(n.on_report(ok(200, "{\"state\":\"cancelled\",\"holes_done\":1,\"holes_total\":4}")) && !n.resume_request().path.empty());
    n.resume_failed();
    CHECK(!n.resume_request().path.empty());

    // 409 not_stopped: the server says the job is not stopped; the screen stays as it was and can ask again
    CHECK(n.on_resume_from(ok(409, "{\"error\":\"not_stopped\",\"detail\":\"the job is running\"}"), why) == ResumeStep::Refused && why == "not_stopped: the job is running");
    CHECK(n.stage() == Stage::Stopped && n.can_resume() && !n.resume_request().path.empty());
    // 404 for a job the server no longer has is not an old server
    CHECK(n.on_resume_from(ok(404, "{\"error\":\"unknown_job\"}"), why) == ResumeStep::Refused && n.resumes_by_server());
    // 422 no_work: nothing is left to do, the button goes away
    CHECK(n.on_resume_from(ok(422, "{\"error\":\"no_work\",\"detail\":\"every hole is done\"}"), why) == ResumeStep::Refused && why == "no_work: every hole is done");
    CHECK(!n.can_resume() && n.resume_request().path.empty() && n.stage() == Stage::Stopped);
    // an answer that is a success but has no job in it is not taken for one
    Workflow m;
    CHECK(m.choose("board.NCD", drl, why) && m.on_created(ok(201, "{\"id\":\"j5\"}"), why) && m.on_started(ok(200, "{}"), why));
    m.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(m.on_report(ok(200, "{\"state\":\"cancelled\",\"holes_done\":1,\"holes_total\":4}")) && m.resume_request().path == "/api/v1/jobs/j5/resume-from");
    CHECK(m.on_resume_from(ok(200, "{}"), why) == ResumeStep::Refused && m.stage() == Stage::Stopped && m.skipped() == 0);

    // an old server (404 or 405 on the route): the screen falls back to sending the file it keeps, with the skip it works out
    Workflow o;
    CHECK(o.choose("board.NCD", drl, why) && o.on_created(ok(201, "{\"id\":\"j6\"}"), why) && o.on_started(ok(200, "{}"), why));
    o.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(o.on_report(ok(200, "{\"state\":\"cancelled\",\"holes_done\":2,\"holes_total\":4}")) && !o.resume_request().path.empty());
    CHECK(o.on_resume_from(ok(404, ""), why) == ResumeStep::Fallback);
    CHECK(o.stage() == Stage::FileChosen && o.skipped() == 2 && parse_json(o.create_request("m1").body, b) && b.get("skip") && b.get("skip")->n == 2);
    CHECK(o.on_created(ok(201, "{\"id\":\"j7\"}"), why) && o.on_started(ok(200, "{}"), why));
    o.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(o.on_report(ok(200, "{\"state\":\"cancelled\",\"holes_done\":1,\"holes_total\":2}")));
    CHECK(!o.resumes_by_server() && o.resume_request().path.empty() && o.can_resume());  // the screen remembers the server is an old one
    CHECK(o.resume(why) && o.skipped() == 3);
    Workflow o2;
    CHECK(o2.choose("board.NCD", drl, why) && o2.on_created(ok(201, "{\"id\":\"j8\"}"), why) && o2.on_started(ok(200, "{}"), why));
    o2.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(o2.on_report(ok(200, "{\"state\":\"failed\",\"holes_done\":1,\"holes_total\":4}")) && o2.resume_request().path == "/api/v1/jobs/j8/resume-from");
    CHECK(o2.on_resume_from(ok(405, "Method Not Allowed"), why) == ResumeStep::Fallback && o2.stage() == Stage::FileChosen && o2.skipped() == 1);

    // after a reset the screen has no file: with the new server it goes on all the same; with an old one it cannot
    Workflow a;
    CHECK(a.adopt("j9", "outline.PLT"));
    a.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(a.on_report(ok(200, "{\"state\":\"cancelled\",\"kind\":\"route\",\"holes_done\":1,\"holes_total\":3}")) && a.route() && a.can_resume());
    CHECK(a.resume_request().path == "/api/v1/jobs/j9/resume-from");
    CHECK(a.on_resume_from(ok(200, "{\"id\":\"j10\",\"resumed_from\":\"j9\",\"kind\":\"route\"}"), why) == ResumeStep::Made);
    CHECK(a.stage() == Stage::Ready && a.job_id() == "j10" && a.resumed_from() == "j9" && a.route() && a.skipped() == 1 && a.start_request().path == "/api/v1/jobs/j10/start");
    Workflow a2;
    CHECK(a2.adopt("j11", "board.NCD"));
    a2.on_fleet(fleet_of("", "idle", 0, ""), "m1");
    CHECK(a2.on_report(ok(200, "{\"state\":\"failed\",\"holes_done\":2,\"holes_total\":4}")) && !a2.resume_request().path.empty());
    CHECK(a2.on_resume_from(ok(404, ""), why) == ResumeStep::Refused && !why.empty() && a2.stage() == Stage::Stopped && !a2.can_resume());
}

static void a_job_is_held_and_a_stop_reaches_the_job_of_the_machine() {
    Workflow w;
    std::string why;
    CHECK(w.choose("board.NCD", "M48\n", why));
    CHECK(w.on_created(ok(201, "{\"id\":\"j42\"}"), why));
    CHECK(w.pause_request().path.empty() && w.release_request().path.empty());  // nothing runs yet
    CHECK(w.on_started(ok(200, "{}"), why));
    // a pause holds the job on the server and never cancels it
    CHECK(w.pause_request().path == "/api/v1/jobs/j42/pause" && w.pause_request().method == "POST");
    CHECK(w.cancel_request().path == "/api/v1/jobs/j42/cancel");
    w.on_fleet(fleet_of("j42", "waiting", 30, "hold"), "m1");
    CHECK(w.stage() == Stage::Waiting && w.held() && !w.tool_wait());
    CHECK(w.release_request().path == "/api/v1/jobs/j42/resume" && w.pause_request().path.empty());
    CHECK(w.confirm_request().path.empty());  // a held job is not at a tool change
    w.on_fleet(fleet_of("j42", "waiting", 30, "resume"), "m1");
    CHECK(w.held() && w.release_request().path == "/api/v1/jobs/j42/resume");
    w.on_fleet(fleet_of("j42", "busy", 31, ""), "m1");
    CHECK(w.stage() == Stage::Running && !w.held() && w.release_request().path.empty());
    // the tool change is answered with confirm, not with resume
    w.on_fleet(fleet_of("j42", "waiting", 50, "tool_change"), "m1");
    CHECK(w.tool_wait() && !w.held());
    CHECK(w.confirm_request().path == "/api/v1/jobs/j42/confirm" && w.release_request().path.empty());

    // the stop reaches the job the fleet shows on the machine, also when this screen did not start it
    Workflow other;
    CHECK(other.stop_requests(FleetView{}, "m1").empty());  // nothing known
    FleetView desk = fleet_of("desk7", "busy", 10, "");
    std::vector<Request> stops = other.stop_requests(desk, "m1");
    CHECK(stops.size() == 1 && stops[0].path == "/api/v1/jobs/desk7/cancel");
    CHECK(other.stop_requests(desk, "another").empty());
    // both the job of the fleet and the own one when they differ; once when they are the same
    stops = w.stop_requests(desk, "m1");
    CHECK(stops.size() == 2 && stops[0].path == "/api/v1/jobs/desk7/cancel" && stops[1].path == "/api/v1/jobs/j42/cancel");
    stops = w.stop_requests(fleet_of("j42", "busy", 50, ""), "m1");
    CHECK(stops.size() == 1 && stops[0].path == "/api/v1/jobs/j42/cancel");
    // an own job that the fleet has not shown yet is stopped too, and so is one that is only ready
    stops = w.stop_requests(FleetView{}, "m1");
    CHECK(stops.size() == 1 && stops[0].path == "/api/v1/jobs/j42/cancel");
    Workflow ready;
    CHECK(ready.choose("board.NCD", "M48\n", why) && ready.on_created(ok(201, "{\"id\":\"r1\"}"), why));
    stops = ready.stop_requests(FleetView{}, "m1");
    CHECK(stops.size() == 1 && stops[0].path == "/api/v1/jobs/r1/cancel");
}

int main() {
    orientation_rules();
    sd_files();
    server_client();
    the_flow_of_a_job();
    a_job_is_followed_again_after_a_reset();
    tools_of_a_file_and_the_spindle();
    a_stopped_job_goes_on_and_two_programs_follow_each_other();
    a_stopped_job_goes_on_through_the_server();
    a_job_is_held_and_a_stop_reaches_the_job_of_the_machine();
    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
