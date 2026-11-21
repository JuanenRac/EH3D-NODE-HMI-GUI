// EH3D-NODE-HMI-GUI - core/settings_contract.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The settings of the server drawn from its contract (`GET /api/v1/settings/contract`, docs/settings-contract.json and docs/CLIENT_SYNC.md of EH3D-SERVER): every key of server.cfg with its
// group, type, range, default, unit, whether a change applies at once or after a restart, the route that writes it, the role it needs, whether it is a secret, its risk and its label, help and
// warning in eight languages. The screen keeps a copy of that document (settings_contract_gen.hpp, written by tools/gen_settings_contract.py) for when the live one cannot be read; the page
// then shows it read only. Nothing here holds a secret: the value of a secret key is never asked for, shown or written. The rules are the ones of EH3D-STUDIO, EH3D-ANDROID-CONTROL and
// EH3D-IOS-CONTROL (the same document, the same checks), with the stricter ones a touch screen next to a machine needs (see settings_page.hpp). The sockets are the firmware's
// (main/server_link.cpp); everything else is tested on a computer.
#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "lang.hpp"
#include "server_errors.hpp"
#include "server_link.hpp"
#include "settings_contract_gen.hpp"
#include "settings_text.hpp"

namespace hmi {

/// The version of the document that the screen understands.
inline constexpr const char* kContractSchema = "eh3d.settings.contract/1";
/// The start of the name of every version of the document (`eh3d.settings.contract/2` is a newer one: the screen says so and shows its copy).
inline constexpr const char* kContractSchemaFamily = "eh3d.settings.contract/";

/// The route that reads the contract and the only route the screen writes a setting through. A contract that names another route for a key makes the key read only here.
inline constexpr const char* kContractPath = "/api/v1/settings/contract";
/// The light, read-only view of the values in force (a few kilobytes, any login): see settings_values.hpp. The server never sends more than `kLightMaxBytes` of it.
inline constexpr const char* kLightPath = "/api/v1/settings/values";
inline constexpr std::size_t kLightMaxBytes = 16u * 1024u;
inline constexpr const char* kPolicyPath = "/api/v1/auth/policy";

enum class SType { Boolean, Integer, String, List, Enum, Unknown };
enum class Apply { Now, Restart };
enum class Risk { None, Security, Exposure, Capacity, Machine, Data };

/// A value of a setting as plain data: nothing, a boolean, a whole number, a text or a list of texts.
struct SValue {
    enum class Kind { None, Bool, Int, Text, List } kind = Kind::None;
    bool b = false;
    long long n = 0;
    std::string s;
    std::vector<std::string> list;

    static SValue of_bool(bool v) { SValue x; x.kind = Kind::Bool; x.b = v; return x; }
    static SValue of_int(long long v) { SValue x; x.kind = Kind::Int; x.n = v; return x; }
    static SValue of_text(std::string v) { SValue x; x.kind = Kind::Text; x.s = std::move(v); return x; }
    static SValue of_list(std::vector<std::string> v) { SValue x; x.kind = Kind::List; x.list = std::move(v); return x; }

    bool operator==(const SValue& o) const { return kind == o.kind && b == o.b && n == o.n && s == o.s && list == o.list; }
    bool operator!=(const SValue& o) const { return !(*this == o); }
};

/// The texts of a field in the eight languages, in the order of lang.hpp.
using Texts = std::array<std::string, kBaseLangCount>;

/// One setting of the server as its contract describes it.
struct Setting {
    std::string key, group;
    SType type = SType::Unknown;
    bool has_default = false;
    SValue def;
    Apply apply = Apply::Restart;
    std::string via, method, path, file;  // how it is written: `policy` (a method and a route) or `file` (set in server.cfg, no route)
    std::string role, scope;              // empty: none asked
    bool confirm_when_looser = false;
    bool secret = false;
    Risk risk = Risk::None;
    bool has_min = false, has_max = false;
    long long min = 0, max = 0;
    std::string unit, format;
    int max_length = 0;  // 0: none
    std::vector<std::string> choices;
    Texts label, help, warning;
};

struct ContractGroup {
    std::string id;
    Texts label;
};

struct Contract {
    std::string schema;
    std::vector<ContractGroup> groups;
    std::vector<Setting> settings;

    const Setting* find(const std::string& key) const {
        for (const Setting& s : settings)
            if (s.key == key) return &s;
        return nullptr;
    }
};

// ---- reading the document ---------------------------------------------------------------------------------------------------------------------

namespace detail {

inline SValue value_of(const Json& j) {
    switch (j.type) {
        case Json::Type::Bool: return SValue::of_bool(j.b);
        case Json::Type::Number:
            if (std::fabs(j.n) < 1e15 && j.n == std::floor(j.n)) return SValue::of_int(static_cast<long long>(j.n));
            { char buf[40]; std::snprintf(buf, sizeof buf, "%g", j.n); return SValue::of_text(buf); }
        case Json::Type::String: return SValue::of_text(j.s);
        case Json::Type::Array: {
            std::vector<std::string> items;
            for (const Json& item : j.a) {
                if (item.type == Json::Type::String) items.push_back(item.s);
                else if (item.type == Json::Type::Number) { const SValue v = value_of(item); items.push_back(v.kind == SValue::Kind::Int ? std::to_string(v.n) : v.s); }
                else if (item.type == Json::Type::Bool) items.push_back(item.b ? "true" : "false");
            }
            return SValue::of_list(std::move(items));
        }
        default: return SValue();
    }
}

/// A whole number of the document; false for anything else (a number with a fraction, a text, null).
inline bool whole_of(const Json* j, long long& out) {
    if (j == nullptr || j->type != Json::Type::Number || !(std::fabs(j->n) < 1e15) || j->n != std::floor(j->n)) return false;
    out = static_cast<long long>(j->n);
    return true;
}

inline SType type_of(const std::string& t) {
    if (t == "boolean") return SType::Boolean;
    if (t == "integer") return SType::Integer;
    if (t == "string") return SType::String;
    if (t == "list") return SType::List;
    if (t == "enum") return SType::Enum;
    return SType::Unknown;
}

inline Risk risk_of(const std::string& t) {
    if (t == "security") return Risk::Security;
    if (t == "exposure") return Risk::Exposure;
    if (t == "capacity") return Risk::Capacity;
    if (t == "machine") return Risk::Machine;
    if (t == "data") return Risk::Data;
    return Risk::None;
}

/// The eight texts of a field of a live document: the contract names its languages by code (en, es, de, fr, it, ja, zh, pt), the screen keeps them in the order of lang.hpp.
inline Texts texts_of(const Json* table) {
    Texts out;
    if (table == nullptr || table->type != Json::Type::Object) return out;
    for (std::size_t i = 0; i < kBaseLangCount; ++i) out[i] = table->str(lang_code(static_cast<Lang>(i)));
    return out;
}

inline bool parse_setting(const Json& o, Setting& out) {
    if (o.type != Json::Type::Object) return false;
    Setting s;
    s.key = o.str("key");
    const Json* write = o.get("write");
    if (s.key.empty() || write == nullptr || write->type != Json::Type::Object) return false;
    s.group = o.str("group");
    s.type = type_of(o.str("type"));
    if (const Json* d = o.get("default")) {
        s.has_default = true;
        s.def = value_of(*d);
    }
    s.apply = o.str("apply", "restart") == "now" ? Apply::Now : Apply::Restart;
    s.via = write->str("via");
    s.method = write->str("method");
    s.path = write->str("path");
    s.file = write->str("file");
    s.role = o.str("role");
    s.scope = o.str("scope");
    const Json* looser = o.get("confirm_when_looser");
    s.confirm_when_looser = looser != nullptr && looser->type == Json::Type::Bool && looser->b;
    const Json* secret = o.get("secret");
    s.secret = secret != nullptr && secret->type == Json::Type::Bool && secret->b;
    s.risk = risk_of(o.str("risk", "none"));
    s.has_min = whole_of(o.get("min"), s.min);
    s.has_max = whole_of(o.get("max"), s.max);
    s.unit = o.str("unit");
    s.format = o.str("format");
    long long len = 0;
    if (whole_of(o.get("max_length"), len) && len > 0 && len < 1000000) s.max_length = static_cast<int>(len);
    if (const Json* values = o.get("values")) {
        if (values->type == Json::Type::Array)
            for (const Json& v : values->a) s.choices.push_back(v.type == Json::Type::String ? v.s : value_of(v).s);
    }
    s.label = texts_of(o.get("label"));
    s.help = texts_of(o.get("help"));
    s.warning = texts_of(o.get("warning"));
    out = std::move(s);
    return true;
}

}  // namespace detail

/// What reading a document found.
enum class ContractParse { Ok, NotJson, OtherSchema, NewerSchema, Malformed };

/// Reads a contract document (the file of the server or the answer of `GET /settings/contract`, which adds `values` and `server_version`). A document of another schema is not drawn: the
/// screen shows its copy; one of a newer schema of the same family is told apart so that the page can say it may be newer. A setting with no key or no `write` makes the whole document
/// malformed (nothing half-understood is drawn).
inline ContractParse parse_contract(const std::string& text, Contract& out) {
    Json root;
    if (!parse_json(text, root) || root.type != Json::Type::Object) return ContractParse::NotJson;
    const std::string schema = root.str("schema");
    if (schema != kContractSchema) return schema.compare(0, std::char_traits<char>::length(kContractSchemaFamily), kContractSchemaFamily) == 0 ? ContractParse::NewerSchema : ContractParse::OtherSchema;
    const Json* groups = root.get("groups");
    const Json* settings = root.get("settings");
    if (groups == nullptr || groups->type != Json::Type::Array || settings == nullptr || settings->type != Json::Type::Array) return ContractParse::Malformed;
    Contract c;
    c.schema = schema;
    for (const Json& g : groups->a) {
        if (g.type != Json::Type::Object || g.str("id").empty()) continue;
        c.groups.push_back({g.str("id"), detail::texts_of(g.get("label"))});
    }
    for (const Json& s : settings->a) {
        Setting parsed;
        if (!detail::parse_setting(s, parsed)) return ContractParse::Malformed;
        c.settings.push_back(std::move(parsed));
    }
    out = std::move(c);
    return ContractParse::Ok;
}

/// The copy of the contract that the screen carries (the table of settings_contract_gen.hpp), built once.
inline const Contract& embedded_contract() {
    static const Contract copy = [] {
        Contract c;
        c.schema = kContractCopySchema;
        auto fill_texts = [](const Row& row) {
            Texts t;
            for (std::size_t i = 0; i < kBaseLangCount; ++i) t[i] = row[i];
            return t;
        };
        for (std::size_t i = 0; i < kContractCopyGroupCount; ++i) c.groups.push_back({kContractCopyGroups[i].id, fill_texts(kContractCopyGroups[i].label)});
        for (std::size_t i = 0; i < kContractCopySettingCount; ++i) {
            const ContractRow& r = kContractCopySettings[i];
            Setting s;
            s.key = r.key;
            s.group = r.group;
            s.type = detail::type_of(r.type);
            if (r.default_json != nullptr) {
                Json d;
                if (parse_json(r.default_json, d)) { s.has_default = true; s.def = detail::value_of(d); }
            }
            s.apply = std::string(r.apply) == "now" ? Apply::Now : Apply::Restart;
            s.via = r.via;
            s.method = r.method != nullptr ? r.method : "";
            s.path = r.path != nullptr ? r.path : "";
            s.file = r.file != nullptr ? r.file : "";
            s.role = r.role != nullptr ? r.role : "";
            s.scope = r.scope != nullptr ? r.scope : "";
            s.confirm_when_looser = r.confirm_when_looser;
            s.secret = r.secret;
            s.risk = detail::risk_of(r.risk);
            s.has_min = r.has_min;
            s.min = r.min;
            s.has_max = r.has_max;
            s.max = r.max;
            s.unit = r.unit != nullptr ? r.unit : "";
            s.format = r.format != nullptr ? r.format : "";
            s.max_length = r.max_length;
            if (r.values_json != nullptr) {
                Json v;
                if (parse_json(r.values_json, v) && v.type == Json::Type::Array)
                    for (const Json& item : v.a) s.choices.push_back(item.type == Json::Type::String ? item.s : detail::value_of(item).s);
            }
            s.label = fill_texts(r.label);
            s.help = fill_texts(r.help);
            s.warning = fill_texts(r.warning);
            c.settings.push_back(std::move(s));
        }
        return c;
    }();
    return copy;
}

/// Whether the contract of the server is the copy the screen carries: `Same` (the `contract_hash` of the server is the SHA-256 of the copy), `Differs` (the copy is out of date) or `Unknown`
/// (the server gave none, or one that is not a SHA-256).
enum class HashState { Unknown, Same, Differs };

/// A SHA-256 as the server writes it: 64 lower-case hexadecimal characters.
inline bool is_sha256_hex(const std::string& t) {
    if (t.size() != 64) return false;
    for (char c : t)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

inline HashState compare_hash(const std::string& server_hash) {
    if (!is_sha256_hex(server_hash)) return HashState::Unknown;
    return server_hash == kContractCopyHash ? HashState::Same : HashState::Differs;
}

/// The contract the page uses and where it came from: the live one (with the values in force), the copy of the screen with the values of the light view (`light`) or the copy of the screen
/// alone (no values), and why the live one was not used.
struct Loaded {
    Contract contract;
    bool live = false;
    bool light = false;  // the values are those of GET /settings/values (any login) and the texts those of the copy of the screen
    std::map<std::string, SValue> values;  // never holds a secret or a hidden key
    std::set<std::string> secret_keys;     // light view: the server marked them secret (their value is never kept)
    std::set<std::string> hidden;          // light view: the server does not give their value (a path of the computer)
    std::set<std::string> truncated;       // light view: a long text or list was cut by the server
    std::set<std::string> overridden;      // light view: changed through the security page, differs from server.cfg
    std::string server_hash;               // the contract_hash the server gave (64 hex characters), empty when none or not valid
    HashState hash = HashState::Unknown;
    std::string server_version;
    bool has_values() const { return live || light; }
    /// Empty for a live contract. Else why the copy is shown: `unreachable` (no answer), `toolong` (the answer did not fit), `shape` (not a document the screen draws), `newer` (a newer
    /// schema), the code of a refusal of the server (`forbidden`, `unauthorized`, ...) or `http_<status>`.
    std::string why;
};

inline Request contract_request() { return {"GET", kContractPath, ""}; }

/// How many bytes of an answer the sockets of the link keep for a request: the contract with its texts in eight languages is far longer than a fleet, so it has a larger room.
inline std::size_t answer_limit(const Request& r) {
    if (r.path == kContractPath) return 1024u * 1024u;
    if (r.path.compare(0, std::char_traits<char>::length(kLightPath), kLightPath) == 0) return kLightMaxBytes;  // with or without `?keys=` / `?group=`
    return 256u * 1024u;
}

/// What the page uses after a try to read the contract: the live document when the server gave one the screen understands, else the copy of the screen and the reason.
inline Loaded load_contract(bool reached, CallError error, const Response& r) {
    Loaded out;
    out.contract = embedded_contract();
    if (!reached) {
        out.why = error == CallError::TooLong ? "toolong" : "unreachable";
        return out;
    }
    if (r.status != 200) {
        Json j;
        std::string code;
        if (parse_json(r.body, j) && j.type == Json::Type::Object) code = j.str("error");
        out.why = detail::looks_like_code(code) ? code : "http_" + std::to_string(r.status);
        return out;
    }
    Contract live;
    switch (parse_contract(r.body, live)) {
        case ContractParse::Ok: break;
        case ContractParse::NewerSchema: out.why = "newer"; return out;
        default: out.why = "shape"; return out;
    }
    Json root;
    parse_json(r.body, root);  // it parsed a moment ago
    out.contract = std::move(live);
    out.live = true;
    out.server_version = root.str("server_version");
    const std::string server_hash = root.str("contract_hash");
    if (is_sha256_hex(server_hash)) out.server_hash = server_hash;
    out.hash = compare_hash(out.server_hash);
    if (const Json* values = root.get("values")) {
        if (values->type == Json::Type::Object)
            for (const auto& kv : values->o) out.values[kv.first] = detail::value_of(kv.second);
    }
    return out;
}

/// The text of a field of the contract in the language of the operator: the text of the language (the fallback language for the twenty-nine and the two variants), English when that is
/// blank or cannot be drawn with the fonts of the screen, then any text there is, then `fallback` (the key).
inline std::string contract_text(const Texts& t, Lang l, const std::string& fallback = "") {
    const std::string& wanted = t[static_cast<std::size_t>(lang_fallback(l))];
    if (!wanted.empty() && detail::drawable(wanted.c_str())) return wanted;
    if (!t[0].empty()) return t[0];
    if (!wanted.empty()) return wanted;
    for (const std::string& any : t)
        if (!any.empty()) return any;
    return fallback;
}

// ---- groups, generic settings and what changed between the copy and the server ------------------------------------------------------------------

struct SettingsGroup {
    std::string id;
    Texts label;
    std::vector<Setting> settings;
};

/// A setting made for a key nobody described: no label but its name, nothing the screen may change.
inline Setting generic_setting(const std::string& key, const SValue& value) {
    Setting s;
    s.key = key;
    s.group = "other";
    s.type = value.kind == SValue::Kind::Bool ? SType::Boolean : value.kind == SValue::Kind::Int ? SType::Integer : SType::Unknown;
    s.via = "none";
    s.label[0] = key;
    return s;
}

/// The keys the server reports a value for and the contract does not list (newer than both): drawn read only, so nothing the server holds is hidden.
inline std::vector<Setting> unlisted_settings(const Contract& c, const std::map<std::string, SValue>& values) {
    std::vector<Setting> out;
    for (const auto& kv : values)
        if (c.find(kv.first) == nullptr) out.push_back(generic_setting(kv.first, kv.second));
    return out;
}

/// The groups in the order of the contract. A setting whose group the contract does not list goes into a group made from its id after the others; a group with no settings is left out.
inline std::vector<SettingsGroup> group_settings(const Contract& c) {
    std::vector<SettingsGroup> out;
    for (const ContractGroup& g : c.groups) out.push_back({g.id, g.label, {}});
    for (const Setting& s : c.settings) {
        SettingsGroup* home = nullptr;
        for (SettingsGroup& g : out)
            if (g.id == s.group) home = &g;
        if (home == nullptr) {
            Texts t;
            t[0] = s.group;
            out.push_back({s.group, t, {}});
            home = &out.back();
        }
        home->settings.push_back(s);
    }
    std::vector<SettingsGroup> kept;
    for (SettingsGroup& g : out)
        if (!g.settings.empty()) kept.push_back(std::move(g));
    return kept;
}

/// What the live server has that the copy of the screen does not, and the other way round (keys, sorted). Only a live contract is compared.
struct Drift {
    std::vector<std::string> newer_in_server;  // in the live contract (or its values), not in the copy
    std::vector<std::string> gone_from_server;  // in the copy, not in the live contract
};

inline Drift contract_drift(const Loaded& loaded) {
    Drift d;
    if (!loaded.has_values()) return d;
    const Contract& copy = embedded_contract();
    for (const Setting& s : loaded.contract.settings)
        if (copy.find(s.key) == nullptr) d.newer_in_server.push_back(s.key);
    std::set<std::string> reported;  // a key of the values, or one the light view marks secret or hidden, that no contract lists
    for (const auto& kv : loaded.values) reported.insert(kv.first);
    for (const std::string& k : loaded.secret_keys) reported.insert(k);
    for (const std::string& k : loaded.hidden) reported.insert(k);
    for (const std::string& k : reported)
        if (loaded.contract.find(k) == nullptr && copy.find(k) == nullptr) d.newer_in_server.push_back(k);
    if (loaded.live)
        for (const Setting& s : copy.settings)
            if (loaded.contract.find(s.key) == nullptr) d.gone_from_server.push_back(s.key);
    return d;
}

// ---- the control of each type, who may write, what is checked before anything is sent ---------------------------------------------------------------

/// The kind of control a setting is drawn with. Everything the screen does not know how to edit is `Generic`: a line with the value as the server gives it.
enum class ControlKind { Switch, Number, Text, Select, ListOf, Generic };

inline ControlKind control_kind(const Setting& s) {
    switch (s.type) {
        case SType::Boolean: return ControlKind::Switch;
        case SType::Integer: return s.has_min && s.has_max ? ControlKind::Number : ControlKind::Text;
        case SType::String: return ControlKind::Text;
        case SType::Enum: return s.choices.empty() ? ControlKind::Text : ControlKind::Select;
        case SType::List: return ControlKind::ListOf;
        default: return ControlKind::Generic;
    }
}

/// Whether a change of this setting needs a second touch: its risk is one that can open the server, weaken its security or touch a machine or data.
inline bool is_high_risk(const Setting& s) { return s.risk == Risk::Security || s.risk == Risk::Exposure || s.risk == Risk::Machine || s.risk == Risk::Data; }

/// The roles in order. An unknown role is below every role; a role the contract asks for that is unknown is never reached.
inline int role_rank(const std::string& role) {
    if (role == "viewer") return 0;
    if (role == "operator") return 1;
    if (role == "admin") return 2;
    return -1;
}
inline bool role_at_least(const std::string& have, const std::string& need) {
    const int want = role_rank(need);
    return want >= 0 && role_rank(have) >= want;
}

/// Whether the contract describes a way to write the setting that the screen trusts: the route is `PUT /api/v1/auth/policy` and nothing else (the token of the screen goes only there), it
/// is no secret and the screen knows how to edit the type.
inline bool has_trusted_route(const Setting& s) { return s.via == "policy" && s.method == "PUT" && s.path == kPolicyPath; }

/// What was wrong with what a person typed.
enum class InputProblem { None, Number, Whole, Text, Choice };

struct Typed {
    bool ok = false;
    SValue value;
    InputProblem problem = InputProblem::None;
};

namespace detail {
inline bool is_whole_text(const std::string& t) {
    std::size_t i = (!t.empty() && t[0] == '-') ? 1 : 0;
    if (i >= t.size() || t.size() - i > 15) return false;
    for (; i < t.size(); ++i)
        if (t[i] < '0' || t[i] > '9') return false;
    return true;
}
inline std::string trimmed(const std::string& s) {
    std::size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t')) --b;
    return s.substr(a, b - a);
}
inline std::size_t code_points(const std::string& s) {
    std::size_t n = 0;
    for (unsigned char c : s)
        if ((c & 0xC0) != 0x80) ++n;
    return n;
}
}  // namespace detail

/// Checks what was typed against the contract before anything is sent: the type, the range of a number, the length of a text and the choices of a list of options.
inline Typed parse_typed(const Setting& s, const std::string& input) {
    Typed t;
    switch (control_kind(s)) {
        case ControlKind::Number: {
            const std::string in = detail::trimmed(input);
            if (!detail::is_whole_text(in)) { t.problem = InputProblem::Number; return t; }
            const long long n = std::stoll(in);
            if (n < s.min || n > s.max) { t.problem = InputProblem::Number; return t; }
            t.ok = true;
            t.value = SValue::of_int(n);
            return t;
        }
        case ControlKind::Text:
            if (s.type == SType::Integer) {
                const std::string in = detail::trimmed(input);
                if (!detail::is_whole_text(in)) { t.problem = InputProblem::Whole; return t; }
                t.ok = true;
                t.value = SValue::of_int(std::stoll(in));
                return t;
            }
            if (s.max_length > 0 && detail::code_points(input) > static_cast<std::size_t>(s.max_length)) { t.problem = InputProblem::Text; return t; }
            t.ok = true;
            t.value = SValue::of_text(input);
            return t;
        case ControlKind::Select:
            for (const std::string& c : s.choices)
                if (c == input) { t.ok = true; t.value = SValue::of_text(c); return t; }
            t.problem = InputProblem::Choice;
            return t;
        case ControlKind::ListOf: {
            std::vector<std::string> items;
            std::size_t pos = 0;
            while (pos <= input.size()) {
                std::size_t comma = input.find(',', pos);
                if (comma == std::string::npos) comma = input.size();
                const std::string item = detail::trimmed(input.substr(pos, comma - pos));
                if (!item.empty()) items.push_back(item);
                pos = comma + 1;
            }
            t.ok = true;
            t.value = SValue::of_list(std::move(items));
            return t;
        }
        default: t.problem = InputProblem::Text; return t;
    }
}

/// The value a switch is turned to.
inline Typed typed_switch(const Setting& s, bool on) {
    Typed t;
    if (control_kind(s) != ControlKind::Switch) { t.problem = InputProblem::Choice; return t; }
    t.ok = true;
    t.value = SValue::of_bool(on);
    return t;
}

/// A value as a line of text: nothing for none, `a, b` for a list, `true` or `false` for a boolean and the plain text otherwise.
inline std::string value_text(const SValue& v) {
    switch (v.kind) {
        case SValue::Kind::Bool: return v.b ? "true" : "false";
        case SValue::Kind::Int: return std::to_string(v.n);
        case SValue::Kind::Text: return v.s;
        case SValue::Kind::List: {
            std::string out;
            for (const std::string& item : v.list) out += (out.empty() ? "" : ", ") + item;
            return out;
        }
        default: return "";
    }
}

// ---- writing --------------------------------------------------------------------------------------------------------------------------------

namespace detail {
inline std::string value_json(const SValue& v) {
    switch (v.kind) {
        case SValue::Kind::Bool: return v.b ? "true" : "false";
        case SValue::Kind::Int: return std::to_string(v.n);
        case SValue::Kind::Text: return json_quote(v.s);
        case SValue::Kind::List: {
            std::string out = "[";
            for (std::size_t i = 0; i < v.list.size(); ++i) out += (i ? "," : "") + json_quote(v.list[i]);
            return out + "]";
        }
        default: return "null";
    }
}
}  // namespace detail

/// The write of one setting: `PUT /api/v1/auth/policy` with `{"key": ..., "value": ..., "confirm_weak": true}` (the last member only when the server asked for it).
inline Request write_request(const Setting& s, const SValue& value, bool confirm_weak) {
    std::string body = "{\"key\":" + json_quote(s.key) + ",\"value\":" + detail::value_json(value);
    if (confirm_weak) body += ",\"confirm_weak\":true";
    return {"PUT", kPolicyPath, body + "}"};
}

/// What became of a change that was sent.
enum class SaveKind { Saved, NeedsWeakConfirm, Refused, Unreachable };

struct SaveResult {
    SaveKind kind = SaveKind::Unreachable;
    bool restart_required = false;  // Saved: the key is read when the server starts
    std::string code;               // Refused: the code of the refusal (`bad_policy`, `forbidden`, ...) or `http_<status>`
    std::string detail;             // Refused: what the server says besides
    CallError error = CallError::None;  // Unreachable: why no answer came
};

/// Reads the answer of a write. A 200 is saved (the server says in `changed.restart_required` whether the key is read when it starts; without that the `apply` of the contract decides);
/// `409 weak_confirm_required` asks the screen to ask the operator again and send the same change with `confirm_weak`; any other answer is a refusal with the code of the server.
inline SaveResult interpret_save(const Setting& s, bool reached, CallError error, const Response& r) {
    SaveResult out;
    if (!reached) {
        out.kind = SaveKind::Unreachable;
        out.error = error;
        return out;
    }
    Json j;
    const bool parsed = parse_json(r.body, j) && j.type == Json::Type::Object;
    if (r.status == 200 && !parsed) {
        // an answer of 200 that is not the JSON of the server (a router page, a proxy) is no proof that anything was saved
        out.kind = SaveKind::Refused;
        out.code = "bad_answer";
        return out;
    }
    if (r.status == 200) {
        out.kind = SaveKind::Saved;
        out.restart_required = s.apply == Apply::Restart;
        if (const Json* changed = j.get("changed")) {
            const Json* rr = changed->type == Json::Type::Object ? changed->get("restart_required") : nullptr;
            if (rr != nullptr && rr->type == Json::Type::Bool) out.restart_required = rr->b;
        }
        return out;
    }
    const std::string code = parsed ? j.str("error") : "";
    out.kind = code == "weak_confirm_required" ? SaveKind::NeedsWeakConfirm : SaveKind::Refused;
    out.code = code.empty() ? "http_" + std::to_string(r.status) : code;
    out.detail = parsed ? j.str("detail") : "";
    return out;
}

}  // namespace hmi
