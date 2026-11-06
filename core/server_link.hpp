// EH3D-NODE-HMI-GUI - core/server_link.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The screen is one more client of EH3D-SERVER, like the desktop programs: it reads the fleet (every machine, its station, its job and how
// far it has got), sends the file of the microSD card as a job and starts, confirms or cancels a job. This file builds the requests and
// reads the answers; the sockets are the firmware's (main/server_link.cpp), so all of the rest is tested on a computer.
#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace hmi {

// ---- a small JSON reader: objects, arrays, strings, numbers, booleans and null; enough for the answers of the server -------------------------

struct Json {
    enum class Type { Null, Bool, Number, String, Array, Object } type = Type::Null;
    bool b = false;
    double n = 0;
    std::string s;
    std::vector<Json> a;
    std::map<std::string, Json> o;

    const Json* get(const std::string& key) const {
        auto it = o.find(key);
        return it == o.end() ? nullptr : &it->second;
    }
    std::string str(const std::string& key, const std::string& fallback = "") const {
        const Json* v = get(key);
        return v && v->type == Type::String ? v->s : fallback;
    }
    double num(const std::string& key, double fallback = 0) const {
        const Json* v = get(key);
        return v && v->type == Type::Number ? v->n : fallback;
    }
};

namespace detail {

struct Reader {
    const std::string& t;
    std::size_t i = 0;
    int depth = 0;
    bool ok = true;

    void ws() {
        while (i < t.size() && (t[i] == ' ' || t[i] == '\t' || t[i] == '\n' || t[i] == '\r')) ++i;
    }
    bool lit(const char* w) {
        std::size_t n = std::char_traits<char>::length(w);
        if (t.compare(i, n, w) == 0) { i += n; return true; }
        return false;
    }
    std::string string() {
        std::string out;
        ++i;  // the opening quote
        while (i < t.size() && t[i] != '"') {
            char c = t[i++];
            if (c == '\\' && i < t.size()) {
                char e = t[i++];
                switch (e) {
                    case 'n': out += '\n'; break;
                    case 't': out += '\t'; break;
                    case 'r': out += '\r'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'u': {
                        // a code point of the basic plane, written as UTF-8
                        if (i + 4 > t.size()) { ok = false; return out; }
                        unsigned cp = 0;
                        for (int k = 0; k < 4; ++k) {
                            char h = t[i++];
                            cp <<= 4;
                            if (h >= '0' && h <= '9') cp |= static_cast<unsigned>(h - '0');
                            else if (h >= 'a' && h <= 'f') cp |= static_cast<unsigned>(h - 'a' + 10);
                            else if (h >= 'A' && h <= 'F') cp |= static_cast<unsigned>(h - 'A' + 10);
                            else { ok = false; return out; }
                        }
                        if (cp < 0x80) out += static_cast<char>(cp);
                        else if (cp < 0x800) { out += static_cast<char>(0xC0 | (cp >> 6)); out += static_cast<char>(0x80 | (cp & 0x3F)); }
                        else { out += static_cast<char>(0xE0 | (cp >> 12)); out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); out += static_cast<char>(0x80 | (cp & 0x3F)); }
                        break;
                    }
                    default: out += e;  // `"`, `\` and `/`
                }
            } else {
                out += c;
            }
        }
        if (i >= t.size()) { ok = false; return out; }
        ++i;  // the closing quote
        return out;
    }
    Json value() {
        Json v;
        ws();
        if (i >= t.size() || ++depth > 32) { ok = false; return v; }
        char c = t[i];
        if (c == '{') {
            v.type = Json::Type::Object;
            ++i;
            ws();
            if (i < t.size() && t[i] == '}') { ++i; --depth; return v; }
            while (ok) {
                ws();
                if (i >= t.size() || t[i] != '"') { ok = false; break; }
                std::string k = string();
                ws();
                if (i >= t.size() || t[i] != ':') { ok = false; break; }
                ++i;
                v.o[k] = value();
                ws();
                if (i < t.size() && t[i] == ',') { ++i; continue; }
                if (i < t.size() && t[i] == '}') { ++i; break; }
                ok = false;
            }
        } else if (c == '[') {
            v.type = Json::Type::Array;
            ++i;
            ws();
            if (i < t.size() && t[i] == ']') { ++i; --depth; return v; }
            while (ok) {
                v.a.push_back(value());
                ws();
                if (i < t.size() && t[i] == ',') { ++i; continue; }
                if (i < t.size() && t[i] == ']') { ++i; break; }
                ok = false;
            }
        } else if (c == '"') {
            v.type = Json::Type::String;
            v.s = string();
        } else if (lit("true")) {
            v.type = Json::Type::Bool;
            v.b = true;
        } else if (lit("false")) {
            v.type = Json::Type::Bool;
        } else if (lit("null")) {
            v.type = Json::Type::Null;
        } else {
            std::size_t j = i;
            while (j < t.size() && (t[j] == '-' || t[j] == '+' || t[j] == '.' || t[j] == 'e' || t[j] == 'E' || (t[j] >= '0' && t[j] <= '9'))) ++j;
            if (j == i) { ok = false; return v; }
            v.type = Json::Type::Number;
            v.n = std::strtod(t.substr(i, j - i).c_str(), nullptr);
            i = j;
        }
        --depth;
        return v;
    }
};

}  // namespace detail

/// Reads a JSON text; false when it is not one (or nests more than 32 levels).
inline bool parse_json(const std::string& text, Json& out) {
    detail::Reader r{text};
    Json v = r.value();
    r.ws();
    if (!r.ok || r.i != text.size()) return false;
    out = std::move(v);
    return true;
}

/// A string as JSON text, with its quotes.
inline std::string json_quote(const std::string& s) {
    std::string out = "\"";
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) { char b[8]; std::snprintf(b, sizeof b, "\\u%04x", c); out += b; }
                else out += static_cast<char>(c);
        }
    }
    return out + "\"";
}

// ---- the requests and the answers ----------------------------------------------------------------------------------------------------------

struct Request {
    std::string method;
    std::string path;
    std::string body;  // JSON text, empty for none
};

struct Response {
    int status = 0;
    std::string body;
};

/// Why a call to the server did not come back with an answer (the screen says it in the language of the operator: see status.hpp).
enum class CallError { None, NoServer, Client, TooLong, Broke, Send, NoAnswer };

/// Who the screen says it is in every request (`name/version`, for example `eh3d-node-hmi-gui/0.0.5`): the server can tell the screens apart in its log.
inline std::string client_id(const std::string& name, const std::string& version) { return version.empty() ? name : name + "/" + version; }

/// The request as the text that goes on the wire; the token of the screen goes as a bearer token, and `client` (see `client_id`) as User-Agent and X-EH3D-Client.
inline std::string wire(const Request& r, const std::string& host, const std::string& token, const std::string& client = "") {
    std::string s = r.method + " " + r.path + " HTTP/1.1\r\nHost: " + host + "\r\nAuthorization: Bearer " + token + "\r\nConnection: close\r\n";
    if (!client.empty()) s += "User-Agent: " + client + "\r\nX-EH3D-Client: " + client + "\r\n";
    if (!r.body.empty()) s += "Content-Type: application/json\r\nContent-Length: " + std::to_string(r.body.size()) + "\r\n";
    return s + "\r\n" + r.body;
}

/// Reads the answer: the status and the body (cut at Content-Length when it is there). False while the answer is not whole yet.
inline bool parse_response(const std::string& raw, Response& out) {
    std::size_t split = raw.find("\r\n\r\n");
    if (split == std::string::npos || raw.compare(0, 5, "HTTP/") != 0) return false;
    std::size_t sp = raw.find(' ');
    if (sp == std::string::npos || sp + 4 > raw.size()) return false;
    int status = std::atoi(raw.substr(sp + 1, 3).c_str());
    if (status < 100 || status > 599) return false;
    std::string head = raw.substr(0, split);
    std::string body = raw.substr(split + 4);
    for (char& c : head)
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    std::size_t cl = head.find("content-length:");
    if (cl != std::string::npos) {
        std::size_t want = static_cast<std::size_t>(std::strtoul(head.c_str() + cl + 15, nullptr, 10));
        if (body.size() < want) return false;
        body.resize(want);
    }
    out.status = status;
    out.body = body;
    return true;
}

/// The error of the server as one line (`unauthorized: no token`), or the status when the body says nothing.
inline std::string error_line(const Response& r) {
    Json j;
    if (parse_json(r.body, j) && j.type == Json::Type::Object) {
        std::string e = j.str("error"), d = j.str("detail");
        if (!e.empty()) return d.empty() ? e : e + ": " + d;
    }
    return "HTTP " + std::to_string(r.status);
}

inline Request fleet_request() { return {"GET", "/api/v1/fleet", ""}; }
inline Request job_report_request(const std::string& id) { return {"GET", "/api/v1/jobs/" + id, ""}; }
inline Request job_start_request(const std::string& id) { return {"POST", "/api/v1/jobs/" + id + "/start", "{}"}; }
inline Request job_cancel_request(const std::string& id) { return {"POST", "/api/v1/jobs/" + id + "/cancel", "{}"}; }
inline Request job_confirm_request(const std::string& id) { return {"POST", "/api/v1/jobs/" + id + "/confirm", "{}"}; }
/// Holds a running job where it is (`POST /jobs/:id/pause`) and lets a held one go on (`POST /jobs/:id/resume`); the server holds only G-code jobs.
inline Request job_pause_request(const std::string& id) { return {"POST", "/api/v1/jobs/" + id + "/pause", "{}"}; }
inline Request job_resume_request(const std::string& id) { return {"POST", "/api/v1/jobs/" + id + "/resume", "{}"}; }
/// Asks the server to make a new job, ready and not started, from the saved request of a stopped one (`POST /jobs/:id/resume-from`): it brings the `skip` forward
/// by itself, so no file is sent. An old server has no such route (404 or 405).
inline Request job_resume_from_request(const std::string& id) { return {"POST", "/api/v1/jobs/" + id + "/resume-from", "{}"}; }

/// A drilling or routing job made of the text of the file of the card (and its tool table when there is one); the server plans it and keeps
/// it ready: nothing starts until `job_start_request` is sent, and that goes through the gate of the screen.
inline Request job_request(const std::string& machine, bool route, const std::string& name, const std::string& file_text, const std::string& tools_text,
                           const std::vector<int>& skip_tools = {}, int skip = 0) {
    std::string files = std::string("{") + json_quote(route ? "plt" : "ncd") + ":" + json_quote(file_text);
    if (!tools_text.empty()) files += std::string(",") + json_quote(route ? "rtd" : "dtd") + ":" + json_quote(tools_text);
    files += "}";
    std::string body = std::string("{\"kind\":") + json_quote(route ? "route" : "drill") + ",\"name\":" + json_quote(name) + ",\"files\":" + files;
    // the tools (the pens of a routing job) that are switched off: the server leaves their holes or paths out of the job and they need not be in the tool table
    if (!skip_tools.empty()) {
        body += ",\"skip_tools\":[";
        for (std::size_t i = 0; i < skip_tools.size(); ++i) body += (i ? "," : "") + std::to_string(skip_tools[i]);
        body += "]";
    }
    // how many holes (or paths) are already done: a job that goes on after a stop does only the rest
    if (skip > 0) body += ",\"skip\":" + std::to_string(skip);
    body += "}";
    return {"POST", "/api/v1/machines/" + machine + "/jobs", body};
}

/// A G-code job made of the text of a program of the card (`.nc`, `.gcode`, `.tap`): the server checks it against the machine (it refuses a machine that takes drilling and routing
/// jobs only, a program it cannot read, or a spindle order beyond the spindle), plans it and keeps it ready. `skip` is the number of lines already done, for a job that goes on.
inline Request job_gcode_request(const std::string& machine, const std::string& name, const std::string& program, int skip = 0) {
    std::string body = std::string("{\"kind\":\"gcode\",\"name\":") + json_quote(name) + ",\"gcode\":" + json_quote(program);
    if (skip > 0) body += ",\"skip\":" + std::to_string(skip);
    body += "}";
    return {"POST", "/api/v1/machines/" + machine + "/jobs", body};
}

/// How a job ended, as `GET /jobs/:id` says it: its state (`done`, `cancelled`, `failed`, ...), why, and the holes (the paths of a routing job) it did of the ones it had.
struct JobReport {
    std::string state, message, kind;  // kind: `drill`, `route` or `gcode`
    int holes_done = 0;  // for a G-code job: the lines done (`items_done`), when the server gives no holes
    int holes_total = 0;
};

inline bool parse_job_report(const std::string& body, JobReport& out) {
    Json j;
    if (!parse_json(body, j) || j.type != Json::Type::Object || j.str("state").empty()) return false;
    JobReport r;
    r.state = j.str("state");
    r.message = j.str("message");
    r.kind = j.str("kind");
    r.holes_done = static_cast<int>(j.num("holes_done"));
    r.holes_total = static_cast<int>(j.num("holes_total"));
    if (r.kind == "gcode") {
        r.holes_done = static_cast<int>(j.num("items_done"));
        r.holes_total = static_cast<int>(j.num("items_total"));
    }
    if (r.holes_done < 0) r.holes_done = 0;
    if (r.holes_total < r.holes_done) r.holes_total = r.holes_done;
    out = std::move(r);
    return true;
}

/// The spindle of the machine and its limits (`GET /machines/:id/spindle`): which one it carries (`original` or `vfd`; empty when none is said), the slowest and
/// the fastest turns of the one in use, and the top speed the node of the machine is told to keep to (0 when none is known).
inline Request spindle_request(const std::string& machine) { return {"GET", "/api/v1/machines/" + machine + "/spindle", ""}; }

struct SpindleView {
    bool known = false;
    std::string kind;
    int min_rpm = 0;
    int max_rpm = 0;
    int node_max_rpm = 0;
    std::string guard;  // `reject` or `warn`
};

inline bool parse_spindle(const std::string& body, SpindleView& out) {
    Json j;
    if (!parse_json(body, j) || j.type != Json::Type::Object) return false;
    SpindleView v;
    v.kind = j.str("spindle");
    v.guard = j.str("guard");
    v.node_max_rpm = static_cast<int>(j.num("node_max_rpm"));
    const Json* active = j.get("active");
    if (active && active->type == Json::Type::Object) {
        v.min_rpm = static_cast<int>(active->num("min_rpm"));
        v.max_rpm = static_cast<int>(active->num("max_rpm"));
    }
    v.known = !v.kind.empty() && v.max_rpm > 0;
    out = std::move(v);
    return true;
}

/// The most turns the screen may ask of the spindle: what the settings allow, and not more than the spindle of the machine gives (0 = the screen never starts it).
inline int spindle_ceiling(int settings_max_rpm, const SpindleView& s) {
    if (settings_max_rpm <= 0) return 0;
    if (!s.known) return settings_max_rpm;
    return settings_max_rpm < s.max_rpm ? settings_max_rpm : s.max_rpm;
}

/// The machine as the server holds it (`GET /machines/:id`): its configuration file is where the table (the course of the machine) is written.
inline Request machine_request(const std::string& machine) { return {"GET", "/api/v1/machines/" + machine, ""}; }

/// The table of the machine in mm: the course of the machine less the offset of the work, which is what a board has to fit in (the server refuses a hole outside it with
/// `out_of_table`).
struct TableSize {
    double width = 0;
    double depth = 0;
    bool known() const { return width > 0 && depth > 0; }
};

namespace detail {
/// A data line of a configuration file is `value   * comment`; the comment starts at a `*` that follows a blank (or starts the line) and is followed by a blank.
inline std::string cfg_value(const std::string& raw) {
    std::string line = raw;
    bool prev_blank = true;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (c == '*' && prev_blank && (i + 1 >= line.size() || line[i + 1] == ' ' || line[i + 1] == '\t' || line[i + 1] == '\r')) {
            line.resize(i);
            break;
        }
        prev_blank = c == ' ' || c == '\t' || c == '\r';
    }
    std::size_t b = line.find_first_not_of(" \t\r");
    if (b == std::string::npos) return "";
    std::size_t e = line.find_last_not_of(" \t\r");
    return line.substr(b, e - b + 1);
}
}  // namespace detail

/// The table from the answer of `machine_request`: the data lines of its `cfg` are the serial number, the version, the directory and the extension, then the values in
/// order (scale 0-1, offset 2-3, course 4-5). False when the answer has no configuration or its lines are not those numbers.
inline bool parse_machine_table(const std::string& body, TableSize& out) {
    Json j;
    if (!parse_json(body, j) || j.type != Json::Type::Object) return false;
    const std::string cfg = j.str("cfg");
    std::vector<std::string> data;
    std::size_t pos = 0;
    while (pos <= cfg.size() && data.size() < 12) {
        std::size_t end = cfg.find('\n', pos);
        if (end == std::string::npos) end = cfg.size();
        const std::string v = detail::cfg_value(cfg.substr(pos, end - pos));
        if (!v.empty()) data.push_back(v);
        pos = end + 1;
    }
    if (data.size() < 4 + 6) return false;
    auto number = [&](std::size_t i, double& v) {
        char* e = nullptr;
        v = std::strtod(data[4 + i].c_str(), &e);
        return e != data[4 + i].c_str();
    };
    double ox = 0, oy = 0, cx = 0, cy = 0;
    if (!number(2, ox) || !number(3, oy) || !number(4, cx) || !number(5, cy)) return false;
    if (!(cx - ox > 0) || !(cy - oy > 0)) return false;
    out.width = cx - ox;
    out.depth = cy - oy;
    return true;
}

/// What the server says about a job it has just made that the operator should know and that does not stop it: what the job asks for beyond the spindle the machine carries
/// (when the machine's switch is on `warn`; on `reject` the job is refused instead). `code` is `rpm_above_max`, `rpm_below_min`, `feed_xy_above_max` or `feed_z_above_max`.
struct SpindleNote {
    std::string code;
    int tool = 0;
    long value = 0;
    long limit = 0;
};

/// The notes of the answer to the creation of a job (`view.spindle_warnings`); empty when there are none or the text is not an answer of the server.
inline std::vector<SpindleNote> parse_job_notes(const std::string& body) {
    std::vector<SpindleNote> out;
    Json j;
    if (!parse_json(body, j) || j.type != Json::Type::Object) return out;
    const Json* view = j.get("view");
    const Json* list = view && view->type == Json::Type::Object ? view->get("spindle_warnings") : j.get("spindle_warnings");
    if (!list || list->type != Json::Type::Array) return out;
    for (const Json& w : list->a) {
        if (w.type != Json::Type::Object || w.str("code").empty() || out.size() >= 16) continue;
        SpindleNote n;
        n.code = w.str("code");
        n.tool = static_cast<int>(w.num("tool"));
        n.value = static_cast<long>(w.num("value"));
        n.limit = static_cast<long>(w.num("limit"));
        out.push_back(n);
    }
    return out;
}

/// One thing that looks wrong in the work the server planned (`view.warnings`): `tiny`, `too_big`, `far`, `scale` or `out_of_course`.
struct PlanWarning {
    std::string code;
    std::string axis;  // out_of_course: X, Y or Z
};

/// What the server says about the job it has just made (`view` of the answer to `POST /machines/:id/jobs`): how long the movements take, how many steps there are and what looks
/// wrong in the work. The operator sees it before the start.
struct PlanView {
    bool known = false;
    double estimate_s = 0;
    int items = 0;
    bool course_known = false;
    bool within_course = true;  // the dry run of the exact plan stays inside the course of every axis
    std::vector<PlanWarning> warnings;
};

inline PlanView parse_plan_view(const std::string& body) {
    PlanView p;
    Json j;
    if (!parse_json(body, j) || j.type != Json::Type::Object) return p;
    const Json* view = j.get("view");
    if (!view || view->type != Json::Type::Object) return p;
    p.known = true;
    p.estimate_s = view->num("estimate_s");
    if (!(p.estimate_s > 0)) p.estimate_s = 0;
    p.items = static_cast<int>(view->num("items"));
    const Json* dry = view->get("dry_run");
    if (dry && dry->type == Json::Type::Object) {
        const Json* within = dry->get("within_course");
        if (within && within->type == Json::Type::Bool) {
            p.course_known = true;
            p.within_course = within->b;
        }
    }
    const Json* list = view->get("warnings");
    if (list && list->type == Json::Type::Array) {
        for (const Json& w : list->a) {
            if (w.type != Json::Type::Object || w.str("code").empty() || p.warnings.size() >= 8) continue;
            p.warnings.push_back({w.str("code"), w.str("axis")});
        }
    }
    return p;
}

struct MachineRow {
    std::string id, name, station, state;  // state: idle, busy or waiting
    std::string model;                     // what the machine is, as the server holds it (`ccd-mtc`, `ccd-atc`...); empty when it says none
    std::string job_name, job_id;
    int percent = -1;                      // -1: no job
    int ready = 0;
    std::string pause_kind;                // `tool_change` while a job waits for the operator
    int pause_tool = 0;                    // the tool the job waits for at a tool change (0 when the server does not say it)
    int steps_done = 0, steps_total = 0;   // the holes (paths) of the job done and in all; the lines of a G-code program
    double estimate_s = 0;                 // how long the movements of the job take, as the server estimated it (0: not said)
};

struct FleetView {
    std::vector<MachineRow> machines;
};

/// The fleet of the answer of `GET /fleet`; false when the text is not an answer of the server.
inline bool parse_fleet(const std::string& body, FleetView& out) {
    Json j;
    if (!parse_json(body, j) || j.type != Json::Type::Object) return false;
    const Json* machines = j.get("machines");
    if (!machines || machines->type != Json::Type::Array) return false;
    FleetView v;
    for (const Json& m : machines->a) {
        if (m.type != Json::Type::Object || m.str("id").empty()) return false;
        MachineRow r;
        r.id = m.str("id");
        r.name = m.str("name", r.id);
        r.station = m.str("station");
        r.state = m.str("state", "idle");
        r.model = m.str("model");
        r.ready = static_cast<int>(m.num("ready"));
        const Json* job = m.get("job");
        if (job && job->type == Json::Type::Object) {
            r.job_id = job->str("id");
            r.job_name = job->str("name");
            r.percent = static_cast<int>(job->num("percent"));
            if (r.percent < 0) r.percent = 0;
            if (r.percent > 100) r.percent = 100;
            const Json* pause = job->get("pause");
            if (pause && pause->type == Json::Type::Object) {
                r.pause_kind = pause->str("kind");
                r.pause_tool = static_cast<int>(pause->num("tool"));
            }
            const bool lines = job->str("kind") == "gcode";
            r.steps_done = static_cast<int>(job->num(lines ? "items_done" : "holes_done"));
            r.steps_total = static_cast<int>(job->num(lines ? "items_total" : "holes_total"));
            if (r.steps_done < 0) r.steps_done = 0;
            if (r.steps_total < r.steps_done) r.steps_total = r.steps_done;
            r.estimate_s = job->num("estimate_s");
            if (!(r.estimate_s > 0)) r.estimate_s = 0;
        }
        v.machines.push_back(std::move(r));
    }
    out = std::move(v);
    return true;
}

}  // namespace hmi
