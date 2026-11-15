// EH3D-NODE-HMI-GUI - host tests of the screen against a simulated EH3D-SERVER: a server in memory that answers the requests the screen builds, and that can be made to fail the ways a workshop
// server fails (no answer, a router page, a refusal, a 503, an answer cut short). The screen is driven with its own requests, its own reading of the answers and its own state of the
// link; the clock is a number and the storms come from a seeded generator, so every run is the same. Nothing here has run on the screen (a Guition 10.1" with the ESP32-P4 and the ESP32-C6)
// or against a real server. Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "../core/status.hpp"

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

/// The ways the simulated server answers.
enum class Mode { Up, Silent, RouterPage, NoToken, Unavailable, CutShort, Degraded };

/// A small deterministic generator (SplitMix64): the same seed always gives the same numbers.
struct Seeded {
    std::uint64_t s;
    std::uint64_t next() {
        s += 0x9E3779B97F4A7C15ull;
        std::uint64_t z = s;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }
    std::size_t below(std::size_t n) { return static_cast<std::size_t>(next() % n); }
};

/// What came back from the wire: whether any answer came, and the text of it.
struct Wire {
    bool reached = false;
    std::string raw;
};

/// A server with a memory: one machine, `m1`, with one job, `j1`, that stops when it is told to.
struct SimServer {
    Mode mode = Mode::Up;
    bool job = true;
    int percent = 30;

    static std::string http(int status, const std::string& body) {
        const char* reason = status == 200 ? "OK" : status == 401 ? "Unauthorized" : "Service Unavailable";
        return "HTTP/1.1 " + std::to_string(status) + " " + reason + "\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
    }

    std::string fleet() const {
        std::string j = job ? "{\"id\":\"j1\",\"name\":\"board\",\"kind\":\"drill\",\"state\":\"running\",\"percent\":" + std::to_string(percent) + ",\"holes_done\":3,\"holes_total\":10}" : "null";
        return "{\"machines\":[{\"id\":\"m1\",\"name\":\"Drill 1\",\"model\":\"ccd-mtc\",\"station\":\"bench\",\"state\":\"" + std::string(job ? "busy" : "idle") + "\",\"ready\":0,\"job\":" + j + "}],\"time_s\":1000}";
    }

    Wire answer(const std::string& request) const {
        if (mode == Mode::Silent) return {false, ""};
        if (mode == Mode::RouterPage) {
            const std::string page = "<html>router</html>";
            return {true, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: " + std::to_string(page.size()) + "\r\n\r\n" + page};
        }
        if (mode == Mode::Unavailable) return {true, http(503, "{\"error\":\"unavailable\"}")};
        const bool health = has(request, "GET /api/v1/health ");
        const bool authorised = has(request, "Authorization: Bearer good-token\r\n");
        std::string raw;
        if (health) raw = http(200, std::string("{\"name\":\"eh3d-server\",\"status\":\"") + (mode == Mode::Degraded ? "degraded" : "ok") + "\"}");
        else if (mode == Mode::NoToken || !authorised) raw = http(401, "{\"error\":\"unauthorized\",\"detail\":\"no token\"}");
        else if (has(request, "GET /api/v1/fleet ")) raw = http(200, fleet());
        else raw = http(404, "{\"error\":\"not_found\"}");
        if (mode == Mode::CutShort) raw.resize(raw.size() / 2);
        return {true, raw};
    }

    /// The server is told to stop the job: it does, and the machine is free at the next look.
    void cancel() { job = false; }
};

/// The screen: it builds its requests, reads what comes back and keeps the state of the link, the way the firmware does with a socket in the middle.
struct Screen {
    explicit Screen(SimServer* s) : server(s) {}
    SimServer* server = nullptr;
    std::string token = "good-token";
    std::string machine = "m1";
    LinkTracker link;
    FleetView view;
    Answer last = Answer::Silent;
    Response response;

    void look(std::uint32_t now_ms) {
        const std::string request = wire(fleet_request(), "workshop:18200", token, client_id("eh3d-node-hmi", "0.0.0"));
        const Wire w = server->answer(request);
        Response r;
        bool reached = w.reached && parse_response(w.raw, r);
        FleetView v;
        const bool parsed = reached && parse_fleet(r.body, v);
        if (parsed) view = v;
        response = r;
        last = classify_fleet(reached, r, parsed);
        link.update(last, last != Answer::Fresh || fleet_has_machine(view, machine), now_ms);
    }

    Health health() const {
        const Wire w = server->answer(wire(health_request(), "workshop:18200", ""));
        Response r;
        const bool reached = w.reached && parse_response(w.raw, r);
        return classify_health(reached, r);
    }
};

static void a_server_that_is_up_is_read_and_the_job_stops_when_it_is_told_to() {
    SimServer server;
    Screen screen(&server);
    screen.look(1000);
    CHECK(screen.last == Answer::Fresh && screen.link.state() == Link::Connected && screen.link.has_data());
    CHECK(screen.view.machines.size() == 1 && screen.view.machines[0].id == "m1" && screen.view.machines[0].percent == 30 && screen.view.machines[0].state == "busy");
    CHECK(screen.health() == Health::Ok);
    // the screen sends the stop through its own request; the server stops the job and the next look shows the machine free
    CHECK(has(wire(job_cancel_request("j1"), "workshop:18200", "good-token"), "POST /api/v1/jobs/j1/cancel HTTP/1.1"));
    server.cancel();
    screen.look(2500);
    CHECK(screen.view.machines[0].state == "idle" && screen.view.machines[0].percent == -1);
}

static void no_answer_is_no_connection_and_the_data_that_were_there_are_called_old_until_the_server_is_back() {
    SimServer server;
    Screen screen(&server);
    screen.look(1000);
    server.mode = Mode::Silent;
    screen.look(4000);
    CHECK(screen.last == Answer::Silent && screen.link.state() == Link::Lost);
    CHECK(screen.health() == Health::Silent);
    // the rows that were read stay, and they are old only after the time the settings give
    CHECK(screen.link.has_data() && !screen.link.stale(10000, 15) && screen.link.stale(16000, 15));
    CHECK(screen.view.machines.size() == 1);
    const std::string label = link_label(screen.link.state(), screen.link.stale(31000, 15), screen.link.age_s(31000), Lang::En);
    CHECK(has(label, "30 s") && label.find('|') != std::string::npos);
    server.mode = Mode::Up;
    screen.look(40000);
    CHECK(screen.link.state() == Link::Connected && !screen.link.stale(40000, 15) && screen.link.age_s(40000) == 0);
}

static void a_router_page_is_another_server_and_a_missing_token_is_a_server_that_gave_no_data() {
    SimServer server;
    Screen screen(&server);
    screen.look(1000);
    server.mode = Mode::RouterPage;
    screen.look(2500);
    CHECK(screen.last == Answer::NotEh3d && screen.link.state() == Link::OtherServer && screen.health() == Health::NotEh3d);
    server.mode = Mode::Up;
    screen.token = "wrong";
    screen.look(4000);
    CHECK(screen.last == Answer::ServerError && screen.link.state() == Link::Connected);
    CHECK(error_line(screen.response) == "unauthorized: no token");
    // the data that came before are still the last that came: they age, and the screen does not take them for current
    CHECK(screen.link.stale(30000, 15));
    screen.token = "good-token";
    screen.look(5500);
    CHECK(screen.last == Answer::Fresh && !screen.link.stale(5500, 15));
}

static void a_503_a_cut_answer_and_a_degraded_server_are_told_apart() {
    SimServer server;
    Screen screen(&server);
    screen.look(1000);
    server.mode = Mode::Unavailable;
    screen.look(2500);
    CHECK(screen.last == Answer::ServerError && screen.link.state() == Link::Connected && error_line(screen.response) == "unavailable");
    server.mode = Mode::CutShort;
    screen.look(4000);
    CHECK(screen.last == Answer::Silent && screen.link.state() == Link::Lost);
    server.mode = Mode::Degraded;
    CHECK(screen.health() == Health::Degraded);
    screen.look(5500);
    CHECK(screen.last == Answer::Fresh && screen.link.state() == Link::Connected);
    // a machine that the server does not list is another server for this screen
    screen.machine = "other";
    screen.look(7000);
    CHECK(screen.link.state() == Link::OtherServer);
}

static std::vector<int> storm(std::uint64_t seed) {
    SimServer server;
    Screen screen(&server);
    Seeded rng{seed};
    const Mode modes[] = {Mode::Up, Mode::Up, Mode::Silent, Mode::RouterPage, Mode::NoToken, Mode::Unavailable, Mode::CutShort, Mode::Degraded};
    std::vector<int> states;
    std::uint32_t now = 0;
    for (int i = 0; i < 300; ++i) {
        now += 1500;
        server.mode = modes[rng.below(8)];
        screen.look(now);
        const Link l = screen.link.state();
        switch (server.mode) {
            case Mode::Up: case Mode::Degraded: CHECK(l == Link::Connected && screen.last == Answer::Fresh); break;
            case Mode::Silent: case Mode::CutShort: CHECK(l == Link::Lost); break;
            case Mode::RouterPage: CHECK(l == Link::OtherServer); break;
            case Mode::NoToken: case Mode::Unavailable: CHECK(l == Link::Connected && screen.last == Answer::ServerError); break;
        }
        // old data is never called current: the age is the time since the last fresh answer
        CHECK(screen.link.age_s(now) * 1000u <= now);
        states.push_back(static_cast<int>(l));
    }
    return states;
}

static void a_seeded_storm_of_failures_always_gives_the_state_of_the_cause_and_the_same_run_every_time() {
    const std::vector<int> a = storm(20261115), b = storm(20261115);
    CHECK(a == b && a.size() == 300);
    CHECK(a != storm(20261116));
    bool lost = false, other = false, connected = false;
    for (int s : a) {
        lost = lost || s == static_cast<int>(Link::Lost);
        other = other || s == static_cast<int>(Link::OtherServer);
        connected = connected || s == static_cast<int>(Link::Connected);
    }
    CHECK(lost && other && connected);
}

int main() {
    a_server_that_is_up_is_read_and_the_job_stops_when_it_is_told_to();
    no_answer_is_no_connection_and_the_data_that_were_there_are_called_old_until_the_server_is_back();
    a_router_page_is_another_server_and_a_missing_token_is_a_server_that_gave_no_data();
    a_503_a_cut_answer_and_a_degraded_server_are_told_apart();
    a_seeded_storm_of_failures_always_gives_the_state_of_the_cause_and_the_same_run_every_time();
    std::printf("sim: %d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
