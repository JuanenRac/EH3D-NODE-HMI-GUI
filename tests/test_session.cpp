// EH3D-NODE-HMI-GUI - host tests of the client of the server: the login of the application hmi and its renewal, the tries again, the connection settings, the text of the refusals of
// the server by code in the eight languages of the tables, and the machines of the plugins with their detail. The clock is a number, so every run is the same.
// Nothing here has run on the screen or against a real server. Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../core/keyboard.hpp"
#include "../core/server_errors.hpp"
#include "../core/session.hpp"

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

static Response refusal(int status, const std::string& body) {
    Response r;
    r.status = status;
    r.body = body;
    return r;
}

// ---- the table of the refusals --------------------------------------------------------------------------------------------------------------

static void the_table() {
    CHECK(kServerErrorCount >= 100);
    for (std::size_t i = 0; i < kServerErrorCount; ++i) {
        const ServerErrorRow& row = kServerErrors[i];
        CHECK(i == 0 || std::strcmp(kServerErrors[i - 1].code, row.code) < 0);  // sorted, each code once
        CHECK(detail::looks_like_code(row.code));
        for (std::size_t l = 0; l < kBaseLangCount; ++l) CHECK(row.texts[l] != nullptr && row.texts[l][0] != '\0');
        CHECK(text_in_font(row.texts[0]));  // the English text can always be drawn
    }
    // the codes the screen meets first are there
    for (const char* code : {"unauthorized", "bad_credentials", "throttled", "unknown_session", "bad_app", "forbidden"}) CHECK(server_error_row(code) != nullptr);
    CHECK(server_error_row("no_such_code_at_all") == nullptr && server_error_row("") == nullptr && server_error_row("zzzz") == nullptr && server_error_row("0") == nullptr);
    // every code is found by the search by halves
    for (std::size_t i = 0; i < kServerErrorCount; ++i) CHECK(server_error_row(kServerErrors[i].code) == &kServerErrors[i]);
}

static void the_font_rule() {
    CHECK(text_in_font("") && text_in_font("Plain text 123 .,:;"));
    CHECK(!text_in_font("\xC3") && !text_in_font("\xE2\x82") && !text_in_font("\xFF") && !text_in_font("a\x80"));  // cut or malformed UTF-8 is not drawable
    int not_drawable = 0;
    for (std::size_t i = 0; i < kServerErrorCount; ++i) {
        for (std::size_t l = 0; l < kBaseLangCount; ++l) {
            const Lang lang = static_cast<Lang>(l);
            const std::string shown = server_error_text(kServerErrors[i].code, lang);
            CHECK(!shown.empty() && text_in_font(shown.c_str()));  // what is given can always be drawn
            if (!text_in_font(kServerErrors[i].texts[l])) {
                ++not_drawable;
                CHECK(shown == kServerErrors[i].texts[0]);  // a text the fonts cannot draw gives the English one
            } else {
                CHECK(shown == kServerErrors[i].texts[l]);
            }
        }
    }
    CHECK(not_drawable > 0);  // until the fonts are built with the characters of the table, Chinese and Japanese fall back to English
}

static void the_line_of_a_refusal() {
    const Response r = refusal(401, "{\"error\":\"bad_credentials\",\"detail\":\"no\"}");
    const ServerErrorRow* row = server_error_row("bad_credentials");
    CHECK(row != nullptr);
    CHECK(error_note(r, Lang::En) == row->texts[0]);
    CHECK(error_note(r, Lang::Es) == row->texts[1] && error_note(r, Lang::De) == row->texts[3] && error_note(r, Lang::Pt) == row->texts[5]);
    CHECK(error_note(r, Lang::En) == row->texts[0] && error_note(r, Lang::Pt) == row->texts[5] && error_note(r, Lang::Fr) == row->texts[4]);  // each of the eight says it in its own language
    // a code that is not in the table is shown as the code and its detail, so nothing is hidden
    CHECK(error_note(refusal(409, "{\"error\":\"brand_new_code\",\"detail\":\"why\"}"), Lang::Es) == "brand_new_code: why");
    CHECK(error_note(refusal(409, "{\"error\":\"brand_new_code\"}"), Lang::Es) == "brand_new_code");
    CHECK(error_note(refusal(502, "<html>bad gateway</html>"), Lang::De) == "HTTP 502" && error_note(refusal(500, ""), Lang::En) == "HTTP 500");
    // a reason that carries a code of the server is told in the language of the operator; the other reasons are not touched
    CHECK(reason_note("unauthorized: bad token", Lang::Es) == server_error_text("unauthorized", Lang::Es));
    CHECK(reason_note("unauthorized", Lang::Fr) == server_error_text("unauthorized", Lang::Fr));
    CHECK(reason_note("The file is empty", Lang::En) == "The file is empty");
    CHECK(reason_note("brand_new_code: why", Lang::En) == "brand_new_code: why");
    CHECK(reason_note("", Lang::En).empty() && reason_note("Some Sentence: with colon", Lang::Es) == "Some Sentence: with colon");
    CHECK(reason_note(reasons::kEmptyFile, Lang::Es) == reason_text(reasons::kEmptyFile, Lang::Es));
}

// ---- the login ------------------------------------------------------------------------------------------------------------------------------------

static void the_requests() {
    const Request login = login_request("ana", "p\"w\\d");
    CHECK(login.method == "POST" && login.path == "/api/v1/auth/login");
    CHECK(has(login.body, "\"name\":\"ana\"") && has(login.body, "\"app\":\"hmi\"") && has(login.body, "\"password\":\"p\\\"w\\\\d\""));
    Json j;
    CHECK(parse_json(login.body, j) && j.str("password") == "p\"w\\d");  // the quoting is read back whole
    CHECK(!request_carries_token(login) && request_carries_token(refresh_request()) && request_carries_token(logout_request()));
    CHECK(refresh_request().path == "/api/v1/auth/refresh" && logout_request().path == "/api/v1/auth/logout" && sessions_request().method == "GET");
    CHECK(close_session_request("abc-12_Z").method == "DELETE" && close_session_request("abc-12_Z").path == "/api/v1/auth/sessions/abc-12_Z");
    CHECK(close_session_request("../../x?y=1 z").path == "/api/v1/auth/sessions/xy1z");  // nothing else can ride in the path
    // the password never goes to an address that did not answer the health check, and the stop still does
    CHECK(!may_send(Health::Silent, login) && !may_send(Health::NotEh3d, login) && may_send(Health::Ok, login) && may_send(Health::Degraded, login));
    CHECK(!may_send(Health::Silent, refresh_request()));
}

static void the_answers() {
    LoginView v;
    CHECK(parse_login("{\"token\":\"tok123\",\"name\":\"ana\",\"role\":\"operator\",\"app\":\"hmi\",\"session_id\":\"s1\",\"expires_s\":86400}", v));
    CHECK(v.token == "tok123" && v.name == "ana" && v.role == "operator" && v.app == "hmi" && v.session_id == "s1" && v.expires_s == 86400);
    CHECK(!parse_login("{\"token\":\"\",\"expires_s\":10}", v) && !parse_login("{\"token\":\"t\"}", v) && !parse_login("{\"token\":\"t\",\"expires_s\":0}", v));
    CHECK(!parse_login("[]", v) && !parse_login("", v) && !parse_login("{\"error\":\"bad_credentials\"}", v));
    CHECK(parse_login("{\"token\":\"t\",\"expires_s\":1e99}", v) && v.expires_s == kMaxLoginLifeS);  // a huge time is brought into range, never converted as it is
    long left = 0;
    CHECK(parse_refresh("{\"expires_s\":600}", left) && left == 600);
    CHECK(!parse_refresh("{\"expires_s\":0}", left) && !parse_refresh("{}", left) && !parse_refresh("nope", left));
    std::vector<SessionRow> rows;
    CHECK(parse_sessions("{\"sessions\":[{\"id\":\"a1\",\"app\":\"hmi\",\"role\":\"operator\",\"ip\":\"192.168.0.5\",\"current\":true},{\"id\":\"b2\",\"app\":\"studio\"},{\"app\":\"x\"}]}", rows));
    CHECK(rows.size() == 2 && rows[0].id == "a1" && rows[0].current && rows[0].ip == "192.168.0.5" && rows[1].id == "b2" && !rows[1].current);
    CHECK(!parse_sessions("{}", rows) && !parse_sessions("[]", rows));
}

static Settings with_login() {
    std::vector<Problem> p;
    Settings s = parse_settings("server_host = 10.0.0.2\nserver_user = ana\nserver_password = secret-pass\ntoken = static-token-1\n", p);
    return s;
}

static void the_tracker() {
    Settings none;  // no name and no password: the screen never logs in
    SessionTracker t;
    CHECK(t.next_step(none, 0) == AuthStep::None && !t.logged_in() && t.effective_token(none).empty());
    const Settings s = with_login();
    CHECK(SessionTracker::has_credentials(s) && t.next_step(s, 5) == AuthStep::Login);
    CHECK(t.effective_token(s) == "static-token-1");  // before a login the token of the settings is used
    LoginView v;
    v.token = "tok-login";
    v.expires_s = 1000;  // 1,000,000 ms of life
    t.on_login(v, 1000);
    CHECK(t.logged_in() && t.effective_token(s) == "tok-login" && t.token() == "tok-login");
    CHECK(t.next_step(s, 1000) == AuthStep::None && t.next_step(s, 1000 + 749999) == AuthStep::None);   // three quarters of the life are not over
    CHECK(t.next_step(s, 1000 + 750000) == AuthStep::Refresh && t.next_step(s, 1000 + 999999) == AuthStep::Refresh);  // the last quarter: renew
    // a renewal that fails with a network error keeps the login and tries again 30 s later
    t.on_refresh_failed(0, 1000 + 750000);
    CHECK(t.logged_in() && t.next_step(s, 1000 + 750000 + 29999) == AuthStep::None && t.next_step(s, 1000 + 750000 + 30000) == AuthStep::Refresh);
    t.on_refresh_failed(503, 1000 + 780000);
    CHECK(t.logged_in());
    // a renewal that works starts the life again
    t.on_refresh(2000, 1000 + 790000);
    CHECK(t.next_step(s, 1000 + 790000 + 1499999) == AuthStep::None && t.next_step(s, 1000 + 790000 + 1500000) == AuthStep::Refresh);
    // a login that runs out is forgotten and the next step is a new login
    CHECK(t.next_step(s, 1000 + 790000 + 2000000) == AuthStep::Login && !t.logged_in());
    // a 401 anywhere forgets the token and the next round logs in again at once
    t.on_login(v, 5000);
    t.on_status(200);
    t.on_status(404);
    CHECK(t.logged_in());
    t.on_status(401);
    CHECK(!t.logged_in() && t.effective_token(s) == "static-token-1" && t.next_step(s, 5001) == AuthStep::Login);
    // a refusal of the renewal (401, 403, 404) means the login is gone
    t.on_login(v, 9000);
    t.on_refresh_failed(404, 9100);
    CHECK(!t.logged_in() && t.next_step(s, 9100) == AuthStep::Login);
    // a 401 with no login held does not wait for anything (the token of the settings is the server's problem)
    SessionTracker plain;
    plain.on_status(401);
    CHECK(plain.next_step(s, 1) == AuthStep::Login);
}

static void the_wait_after_a_refusal() {
    const Settings s = with_login();
    SessionTracker t;
    std::uint32_t at = 100;
    const std::uint32_t waits[] = {30000, 60000, 120000, 240000, 480000, 600000, 600000, 600000};
    for (std::uint32_t w : waits) {
        CHECK(t.next_step(s, at) == AuthStep::Login);
        t.on_login_failed(at);
        CHECK(t.next_step(s, at) == AuthStep::None && t.next_step(s, at + w - 1) == AuthStep::None && t.next_step(s, at + w) == AuthStep::Login);
        at += w;
    }
    // a good login clears the wait
    LoginView v;
    v.token = "t";
    v.expires_s = 100;
    t.on_login(v, at);
    t.forget();
    CHECK(t.next_step(s, at) == AuthStep::Login);
    // the settings lose the password: nothing to do
    Settings gone = s;
    gone.server_password.clear();
    CHECK(t.next_step(gone, at) == AuthStep::None);
}

static void the_clock_wraps() {
    const Settings s = with_login();
    SessionTracker t;
    LoginView v;
    v.token = "t";
    v.expires_s = 1000;
    const std::uint32_t start = 0xFFFFFF00u;  // about to wrap
    t.on_login(v, start);
    CHECK(t.next_step(s, start + 100000) == AuthStep::None);
    CHECK(t.next_step(s, start + 750000) == AuthStep::Refresh);  // wraps past zero
    CHECK(t.next_step(s, start + 1000000) == AuthStep::Login);
    // a clock far past the 24 days the signed difference covers: the very first login is not held back
    SessionTracker late;
    CHECK(late.next_step(s, 0xF0000000u) == AuthStep::Login);
    late.on_login_failed(0xF0000000u);
    CHECK(late.next_step(s, 0xF0000000u + 29999) == AuthStep::None && late.next_step(s, 0xF0000000u + 30000) == AuthStep::Login);
}

// ---- tries again ----------------------------------------------------------------------------------------------------------------------------------

static void the_tries_again() {
    // a connection that could not be made: nothing was sent, so any method goes again
    for (const char* m : {"GET", "POST", "PUT", "DELETE"}) CHECK(retry_allowed(m, CallError::NoAnswer, 0));
    // a time-out or a broken connection: only a method that does the same when sent twice; a POST (a job, a login, a start) never
    for (CallError e : {CallError::Broke, CallError::Send}) {
        CHECK(retry_allowed("GET", e, 0) && retry_allowed("HEAD", e, 0) && retry_allowed("PUT", e, 0) && retry_allowed("DELETE", e, 0) && retry_allowed("OPTIONS", e, 0));
        CHECK(!retry_allowed("POST", e, 0));
    }
    for (int status : {502, 503, 504}) CHECK(retry_allowed("GET", CallError::None, status) && !retry_allowed("POST", CallError::None, status));
    for (int status : {200, 201, 400, 401, 403, 404, 409, 422, 500, 501}) CHECK(!retry_allowed("GET", CallError::None, status));
    CHECK(!retry_allowed("GET", CallError::TooLong, 0) && !retry_allowed("GET", CallError::NoServer, 0) && !retry_allowed("GET", CallError::Client, 0));
    CHECK(retry_wait_ms(0) == 0 && retry_wait_ms(1) == 200 && retry_wait_ms(2) == 400 && retry_wait_ms(3) == 800 && retry_wait_ms(4) == 1600 && retry_wait_ms(5) == 2000 && retry_wait_ms(1000) == 2000);
}

// ---- the connection settings ----------------------------------------------------------------------------------------------------------------

static void the_connection_settings() {
    std::vector<Problem> p;
    const Settings d = parse_settings("", p);
    CHECK(p.empty() && d.server_timeout_s == 15 && d.server_retries == 2 && !d.server_mdns && d.server_user.empty() && d.server_password.empty());
    Settings s = parse_settings("server_timeout_s = 120\nserver_retries = 5\nserver_mdns = on\nserver_user = ana\nserver_password = a long pass\n", p);
    CHECK(p.empty() && s.server_timeout_s == 120 && s.server_retries == 5 && s.server_mdns && s.server_user == "ana" && s.server_password == "a long pass");
    s = parse_settings("server_timeout_s = 1\nserver_retries = 0\nserver_mdns = off\n", p);
    CHECK(p.empty() && s.server_timeout_s == 1 && s.server_retries == 0 && !s.server_mdns);
    // a value out of its range changes nothing and says which key it is
    p.clear();
    s = parse_settings("server_timeout_s = 0\nserver_timeout_s = 121\nserver_retries = 6\nserver_retries = -1\nserver_mdns = maybe\n", p);
    CHECK(p.size() == 5 && s.server_timeout_s == 15 && s.server_retries == 2 && !s.server_mdns);
    CHECK(p[0].key == "server_timeout_s" && p[2].key == "server_retries" && p[4].key == "server_mdns" && p[4].reason == CfgReason::OnOff);
    // a name or a password that is too long is refused, and the line that says so does not hold the password
    p.clear();
    const std::string long_pass(129, 'q');
    s = parse_settings("server_password = " + long_pass + "\nserver_user = " + std::string(65, 'u') + "\n", p);
    CHECK(p.size() == 2 && s.server_password.empty() && s.server_user.empty());
    CHECK(p[0].what.find("qqqq") == std::string::npos && p[0].key == "server_password");
    // the keyboard describes the five keys with the same ranges
    CHECK(field_spec("server_timeout_s") != nullptr && field_spec("server_retries") != nullptr && field_spec("server_mdns") != nullptr);
    CHECK(field_spec("server_password") != nullptr && field_spec("server_password")->kind == FieldKind::Secret && field_spec("server_user")->kind == FieldKind::Text);
}

// ---- the machines of the plugins ------------------------------------------------------------------------------------------------------------

static void the_machines_of_the_plugins() {
    const char* body =
        "{\"machines\":["
        "{\"id\":\"cnc1\",\"name\":\"Drill\",\"plugin\":\"cnc\",\"state\":\"idle\",\"job\":null,\"capabilities\":{},\"detail\":{\"latched\":true,\"position_mm\":{\"x\":1},\"rpm\":0}},"
        "{\"id\":\"laser1\",\"name\":\"Laser\",\"plugin\":\"laser\",\"state\":\"busy\",\"job\":\"j7\",\"detail\":{\"estop_latched\":false,\"beam_on\":true,\"evidence\":\"simulator\",\"simulator_only\":true}},"
        "{\"id\":\"p1\",\"name\":\"Printer\",\"plugin\":\"printer3d\",\"state\":\"idle\",\"detail\":{\"alarm\":\"thermal runaway\",\"bed_c\":60.5}},"
        "{\"id\":\"p2\",\"plugin\":\"printer3d\",\"state\":\"idle\",\"detail\":{\"alarm\":\"\"}},"
        "{\"id\":\"p3\",\"name\":\"Calm\",\"plugin\":\"demo\",\"state\":\"idle\",\"detail\":{\"alarm\":0,\"latched\":\"none\"}},"
        "{\"name\":\"no id\"},5]}";
    std::vector<WorkMachine> w;
    CHECK(parse_work_machines(body, w));
    CHECK(w.size() == 5);
    CHECK(w[0].id == "cnc1" && w[0].plugin == "cnc" && w[0].attention && w[0].job.empty() && has(w[0].detail, "latched true") && has(w[0].detail, "rpm 0"));
    CHECK(w[0].detail.find("position_mm") == std::string::npos);  // an object is not a plain value
    CHECK(w[0].detail.find("latched") < w[0].detail.find("rpm"));  // what raises an alarm comes first
    CHECK(w[1].id == "laser1" && !w[1].attention && w[1].job == "j7" && has(w[1].detail, "beam_on true") && has(w[1].detail, "estop_latched false"));
    CHECK(w[1].detail.find("evidence") == std::string::npos && w[1].detail.find("simulator_only") == std::string::npos);
    CHECK(w[2].attention && has(w[2].detail, "alarm thermal runaway") && has(w[2].detail, "bed_c 60.5"));
    CHECK(w[3].id == "p2" && w[3].name == "p2" && !w[3].attention);  // an empty text is no alarm; a missing name is the id
    CHECK(w[4].id == "p3" && !w[4].attention);                        // 0 and "none" are no alarm
    CHECK(!parse_work_machines("{}", w) && !parse_work_machines("[]", w) && !parse_work_machines("not json", w) && !parse_work_machines("{\"machines\":3}", w));
    CHECK(parse_work_machines("{\"machines\":[]}", w) && w.empty());
    CHECK(work_machines_request().path == "/api/v1/work/machines" && work_machines_request().method == "GET" && request_carries_token(work_machines_request()));
    // at most 32 machines are kept, and at most 4 entries of the detail per machine
    std::string many = "{\"machines\":[";
    for (int i = 0; i < 40; ++i) many += std::string(i ? "," : "") + "{\"id\":\"m" + std::to_string(i) + "\",\"detail\":{\"a\":1,\"b\":2,\"c\":3,\"d\":4,\"e\":5,\"f\":6}}";
    many += "]}";
    CHECK(parse_work_machines(many, w) && w.size() == 32 && w[0].detail == "a 1   b 2   c 3   d 4");
    // the fleet carries them to the screen
    FleetView fv;
    CHECK(fv.work.empty());
}

int main() {
    the_table();
    the_font_rule();
    the_line_of_a_refusal();
    the_requests();
    the_answers();
    the_tracker();
    the_wait_after_a_refusal();
    the_clock_wraps();
    the_tries_again();
    the_connection_settings();
    the_machines_of_the_plugins();
    std::printf("session: %d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
