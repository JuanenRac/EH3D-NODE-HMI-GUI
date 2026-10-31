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
    std::string w = wire(fleet_request(), "192.168.0.180:8080", "tok-1234");
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

int main() {
    orientation_rules();
    sd_files();
    server_client();
    the_flow_of_a_job();
    a_job_is_followed_again_after_a_reset();
    std::printf("%d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
