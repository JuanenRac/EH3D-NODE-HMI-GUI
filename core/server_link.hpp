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

/// The request as the text that goes on the wire; the token of the screen goes as a bearer token.
inline std::string wire(const Request& r, const std::string& host, const std::string& token) {
    std::string s = r.method + " " + r.path + " HTTP/1.1\r\nHost: " + host + "\r\nAuthorization: Bearer " + token + "\r\nConnection: close\r\n";
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
inline Request job_start_request(const std::string& id) { return {"POST", "/api/v1/jobs/" + id + "/start", "{}"}; }
inline Request job_cancel_request(const std::string& id) { return {"POST", "/api/v1/jobs/" + id + "/cancel", "{}"}; }
inline Request job_confirm_request(const std::string& id) { return {"POST", "/api/v1/jobs/" + id + "/confirm", "{}"}; }

/// A drilling or routing job made of the text of the file of the card (and its tool table when there is one); the server plans it and keeps
/// it ready: nothing starts until `job_start_request` is sent, and that goes through the gate of the screen.
inline Request job_request(const std::string& machine, bool route, const std::string& name, const std::string& file_text, const std::string& tools_text) {
    std::string files = std::string("{") + json_quote(route ? "plt" : "ncd") + ":" + json_quote(file_text);
    if (!tools_text.empty()) files += std::string(",") + json_quote(route ? "rtd" : "dtd") + ":" + json_quote(tools_text);
    files += "}";
    std::string body = std::string("{\"kind\":") + json_quote(route ? "route" : "drill") + ",\"name\":" + json_quote(name) + ",\"files\":" + files + "}";
    return {"POST", "/api/v1/machines/" + machine + "/jobs", body};
}

struct MachineRow {
    std::string id, name, station, state;  // state: idle, busy or waiting
    std::string job_name, job_id;
    int percent = -1;                      // -1: no job
    int ready = 0;
    std::string pause_kind;                // `tool_change` while a job waits for the operator
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
        r.ready = static_cast<int>(m.num("ready"));
        const Json* job = m.get("job");
        if (job && job->type == Json::Type::Object) {
            r.job_id = job->str("id");
            r.job_name = job->str("name");
            r.percent = static_cast<int>(job->num("percent"));
            if (r.percent < 0) r.percent = 0;
            if (r.percent > 100) r.percent = 100;
            const Json* pause = job->get("pause");
            if (pause && pause->type == Json::Type::Object) r.pause_kind = pause->str("kind");
        }
        v.machines.push_back(std::move(r));
    }
    out = std::move(v);
    return true;
}

}  // namespace hmi
