// EH3D-NODE-HMI-GUI - host tests of the settings of the server drawn from its contract: the copy the screen carries (106 settings in 17 groups), the live document and what the screen does
// when it is not one it understands, the control of each type, what is checked before anything is sent, who may change what on a touch screen next to a machine, the steps of a change
// (twice for a risky setting, a third time for a value the server calls looser than recommended, never by voice), the answers of the server by code, the texts in the eight languages
// of the tables and a seeded storm of failures against a simulated server. The clock is a number and the seed is fixed, so every run is the same.
// Nothing here has run on the screen or against a real server. Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "../core/session.hpp"
#include "../core/settings_page.hpp"

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

// A small generator with a fixed seed: the same numbers on every machine.
struct Lcg {
    std::uint32_t s;
    std::uint32_t next() { s = s * 1664525u + 1013904223u; return s >> 8; }
    std::uint32_t below(std::uint32_t n) { return next() % n; }
};

static Response answer(int status, const std::string& body) {
    Response r;
    r.status = status;
    r.body = body;
    return r;
}

// ---- a contract written as JSON, the way the server writes it ------------------------------------------------------------------------------------

static std::string texts_json(const Texts& t) {
    std::string out = "{";
    for (std::size_t i = 0; i < kBaseLangCount; ++i) out += (i ? "," : "") + json_quote(lang_code(static_cast<Lang>(i))) + ":" + json_quote(t[i]);
    return out + "}";
}

static const char* type_name(SType t) {
    switch (t) {
        case SType::Boolean: return "boolean";
        case SType::Integer: return "integer";
        case SType::String: return "string";
        case SType::List: return "list";
        case SType::Enum: return "enum";
        default: return "float";
    }
}
static const char* risk_name(Risk r) {
    switch (r) {
        case Risk::Security: return "security";
        case Risk::Exposure: return "exposure";
        case Risk::Capacity: return "capacity";
        case Risk::Machine: return "machine";
        case Risk::Data: return "data";
        default: return "none";
    }
}
static std::string value_json_of(const SValue& v) {
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
static std::string opt(const std::string& s) { return s.empty() ? "null" : json_quote(s); }

static std::string setting_json(const Setting& s) {
    std::string o = "{\"key\":" + json_quote(s.key) + ",\"group\":" + json_quote(s.group) + ",\"type\":" + json_quote(type_name(s.type));
    if (s.has_default) o += ",\"default\":" + value_json_of(s.def);
    o += std::string(",\"apply\":") + (s.apply == Apply::Now ? "\"now\"" : "\"restart\"");
    o += ",\"write\":{\"via\":" + json_quote(s.via) + ",\"method\":" + opt(s.method) + ",\"path\":" + opt(s.path) + ",\"file\":" + opt(s.file) + "}";
    o += ",\"role\":" + opt(s.role) + ",\"scope\":" + opt(s.scope);
    o += std::string(",\"confirm_when_looser\":") + (s.confirm_when_looser ? "true" : "false") + ",\"secret\":" + (s.secret ? "true" : "false") + ",\"risk\":" + json_quote(risk_name(s.risk));
    if (s.has_min) o += ",\"min\":" + std::to_string(s.min);
    if (s.has_max) o += ",\"max\":" + std::to_string(s.max);
    if (!s.unit.empty()) o += ",\"unit\":" + json_quote(s.unit);
    if (s.max_length > 0) o += ",\"max_length\":" + std::to_string(s.max_length);
    if (!s.format.empty()) o += ",\"format\":" + json_quote(s.format);
    if (!s.choices.empty()) {
        o += ",\"values\":[";
        for (std::size_t i = 0; i < s.choices.size(); ++i) o += (i ? "," : "") + json_quote(s.choices[i]);
        o += "]";
    }
    return o + ",\"label\":" + texts_json(s.label) + ",\"help\":" + texts_json(s.help) + ",\"warning\":" + texts_json(s.warning) + "}";
}

/// A contract as the server writes it; `tail` is more members of the top level (`"values":{...}`).
static std::string contract_json(const Contract& c, const std::string& schema, const std::string& tail = "") {
    std::string o = "{\"schema\":" + json_quote(schema) + ",\"languages\":[\"en\",\"es\",\"de\",\"fr\",\"it\",\"ja\",\"zh\",\"pt\"],\"groups\":[";
    for (std::size_t i = 0; i < c.groups.size(); ++i) o += (i ? "," : "") + std::string("{\"id\":") + json_quote(c.groups[i].id) + ",\"label\":" + texts_json(c.groups[i].label) + "}";
    o += "],\"settings\":[";
    for (std::size_t i = 0; i < c.settings.size(); ++i) o += (i ? "," : "") + setting_json(c.settings[i]);
    return o + "]" + (tail.empty() ? "" : "," + tail) + "}";
}

static Texts words(const std::string& w) {
    Texts t;
    for (std::size_t i = 0; i < kBaseLangCount; ++i) t[i] = w + " " + lang_code(static_cast<Lang>(i));
    return t;
}

/// A setting of a small contract written here.
static Setting make(const std::string& key, SType type = SType::Boolean, const std::string& group = "network") {
    Setting s;
    s.key = key;
    s.group = group;
    s.type = type;
    s.apply = Apply::Now;
    s.via = "policy";
    s.method = "PUT";
    s.path = kPolicyPath;
    s.role = "admin";
    s.scope = "admin:policy";
    s.risk = Risk::None;
    s.label = words("Label " + key);
    s.help = words("Help " + key);
    s.warning = words("Warning " + key);
    return s;
}

static Contract small_contract(const std::vector<Setting>& settings) {
    Contract c;
    c.schema = kContractSchema;
    c.groups = {{"network", words("Network")}, {"security", words("Security")}};
    c.settings = settings;
    return c;
}

static Loaded live_loaded(const Contract& c, const std::map<std::string, SValue>& values) {
    Loaded l;
    l.contract = c;
    l.live = true;
    l.values = values;
    l.server_version = "9.9.9";
    return l;
}

// ---- the copy of the screen -------------------------------------------------------------------------------------------------------------------------

static void the_copy() {
    const Contract& c = embedded_contract();
    CHECK(c.schema == kContractSchema);
    CHECK(c.settings.size() == 106 && kContractCopySettingCount == 106);
    CHECK(c.groups.size() == 17 && kContractCopyGroupCount == 17);
    std::set<std::string> keys, groups;
    for (const ContractGroup& g : c.groups) {
        groups.insert(g.id);
        for (const std::string& t : g.label) CHECK(!t.empty());
    }
    CHECK(groups.size() == 17);
    int policy = 0, file = 0;
    for (const Setting& s : c.settings) {
        CHECK(keys.insert(s.key).second);  // each key once
        CHECK(groups.count(s.group) == 1);
        CHECK(s.type != SType::Unknown);
        CHECK(control_kind(s) != ControlKind::Generic);  // every setting of the copy has a control the screen knows
        CHECK(!s.secret);
        for (std::size_t i = 0; i < kBaseLangCount; ++i) CHECK(!s.label[i].empty() && !s.help[i].empty() && !s.warning[i].empty());
        for (std::size_t i = 0; i < kBaseLangCount; ++i)  // every language of the copy can be drawn
            CHECK(text_in_font(s.label[i].c_str()) && text_in_font(s.help[i].c_str()) && text_in_font(s.warning[i].c_str()));
        if (s.via == "policy") {
            ++policy;
            CHECK(has_trusted_route(s) && s.role == "admin" && s.scope == "admin:policy");
            CHECK(s.type == SType::Boolean || s.type == SType::Integer);
            CHECK(s.risk == Risk::Security || s.risk == Risk::Exposure);  // all the keys of the security page are risky: each asks twice
            CHECK(s.confirm_when_looser);
        } else {
            ++file;
            CHECK(s.via == "file" && s.file == "server.cfg" && s.method.empty() && s.path.empty());
        }
        if (s.type == SType::Integer && s.has_min && s.has_max) {
            CHECK(s.min <= s.max);
            if (s.has_default && s.def.kind == SValue::Kind::Int) CHECK(s.def.n >= s.min && s.def.n <= s.max);
        }
        if (s.type == SType::Boolean && s.has_default) CHECK(s.def.kind == SValue::Kind::Bool);
        if (s.type == SType::Integer && s.has_default) CHECK(s.def.kind == SValue::Kind::Int);
    }
    CHECK(policy == 19 && file == 86);
    // the keys the screen reasons about are there with the type it expects
    const Setting* body = c.find("max_body_mb");
    CHECK(body != nullptr && body->type == SType::Integer && body->min == 1 && body->max == 256 && body->unit == "megabytes");
    const Setting* slots = c.find("rack_max_slots");
    CHECK(slots != nullptr && slots->max == 14 && slots->def.n == 14);
    const Setting* looser = c.find("network_open");
    CHECK(looser != nullptr && looser->apply == Apply::Restart && looser->risk == Risk::Exposure && !looser->def.b);
    CHECK(c.find("no_such_key") == nullptr);
    // the settings of a stop and of going on are in the copy, and their lengths are written with the international symbols
    for (const char* k : {"grbl_stop_wait_s", "resume_safe_z_mm", "resume_spindle_wait_s", "resume_plunge_feed_mm_min", "resume_position_tolerance_um"}) CHECK(c.find(k) != nullptr);
    const Setting* z = c.find("resume_safe_z_mm");
    CHECK(z != nullptr && unit_short(z->unit) == "mm" && unit_short("micrometres") == "\xC2\xB5m" && unit_short("mm/min") == "mm/min" && text_in_font("\xC2\xB5m"));
    // the groups come in the order of the contract and hold every setting
    const std::vector<SettingsGroup> grouped = group_settings(c);
    CHECK(grouped.size() == 17);
    std::size_t total = 0;
    for (const SettingsGroup& g : grouped) total += g.settings.size();
    CHECK(total == 106);
    CHECK(grouped[0].id == c.groups[0].id && grouped[0].id == "network");
}

static void the_round_trip() {
    // the copy written as the server writes it and read back by the code that reads the live document: nothing is lost in either way
    const Contract& copy = embedded_contract();
    Contract again;
    CHECK(parse_contract(contract_json(copy, kContractSchema), again) == ContractParse::Ok);
    CHECK(again.settings.size() == copy.settings.size() && again.groups.size() == copy.groups.size());
    for (std::size_t i = 0; i < copy.settings.size() && i < again.settings.size(); ++i) {
        const Setting &a = copy.settings[i], &b = again.settings[i];
        CHECK(a.key == b.key && a.group == b.group && a.type == b.type && a.apply == b.apply && a.via == b.via && a.method == b.method && a.path == b.path && a.file == b.file);
        CHECK(a.role == b.role && a.scope == b.scope && a.confirm_when_looser == b.confirm_when_looser && a.secret == b.secret && a.risk == b.risk);
        CHECK(a.has_min == b.has_min && a.min == b.min && a.has_max == b.has_max && a.max == b.max && a.unit == b.unit && a.max_length == b.max_length && a.format == b.format);
        CHECK(a.has_default == b.has_default && a.def == b.def && a.choices == b.choices);
        CHECK(a.label == b.label && a.help == b.help && a.warning == b.warning);
    }
    // the answer of the server adds the values in force and its version
    std::string values = "\"server_version\":\"0.2.0\",\"values\":{\"network_open\":false,\"max_body_mb\":8,\"auth_extra_apps\":[\"a\",\"b\"],\"mdns_name\":\"eh3d\",\"newer_key\":true}";
    const Loaded l = load_contract(true, CallError::None, answer(200, contract_json(copy, kContractSchema, values)));
    CHECK(l.live && l.why.empty() && l.server_version == "0.2.0");
    CHECK(l.values.size() == 5 && l.values.at("max_body_mb").n == 8 && !l.values.at("network_open").b && l.values.at("auth_extra_apps").list.size() == 2 && l.values.at("mdns_name").s == "eh3d");
    CHECK(l.contract.settings.size() == 106);
}

// ---- documents the screen does not draw ---------------------------------------------------------------------------------------------------------

static void documents_it_does_not_understand() {
    Contract c;
    const std::string good = contract_json(small_contract({make("a")}), kContractSchema);
    CHECK(parse_contract(good, c) == ContractParse::Ok && c.settings.size() == 1);
    CHECK(parse_contract("", c) == ContractParse::NotJson);
    CHECK(parse_contract("<html>router</html>", c) == ContractParse::NotJson);
    CHECK(parse_contract("[1,2]", c) == ContractParse::NotJson);
    CHECK(parse_contract("{\"schema\":\"something/1\"}", c) == ContractParse::OtherSchema);
    CHECK(parse_contract("{}", c) == ContractParse::OtherSchema);
    CHECK(parse_contract(contract_json(small_contract({make("a")}), "eh3d.settings.contract/2"), c) == ContractParse::NewerSchema);
    CHECK(parse_contract("{\"schema\":\"eh3d.settings.contract/1\"}", c) == ContractParse::Malformed);
    CHECK(parse_contract("{\"schema\":\"eh3d.settings.contract/1\",\"groups\":[],\"settings\":{}}", c) == ContractParse::Malformed);
    // a setting with no key, or with no `write`, makes the whole document malformed: nothing half-understood is drawn
    CHECK(parse_contract("{\"schema\":\"eh3d.settings.contract/1\",\"groups\":[],\"settings\":[{\"key\":\"a\"}]}", c) == ContractParse::Malformed);
    CHECK(parse_contract("{\"schema\":\"eh3d.settings.contract/1\",\"groups\":[],\"settings\":[{\"write\":{\"via\":\"file\"}}]}", c) == ContractParse::Malformed);
    CHECK(parse_contract("{\"schema\":\"eh3d.settings.contract/1\",\"groups\":[],\"settings\":[3]}", c) == ContractParse::Malformed);
    // a type the screen has never heard of is read, and drawn as a read-only line
    Setting odd = make("odd", SType::Unknown);
    Contract oc;
    CHECK(parse_contract(contract_json(small_contract({odd}), kContractSchema), oc) == ContractParse::Ok && oc.settings[0].type == SType::Unknown);
    CHECK(control_kind(oc.settings[0]) == ControlKind::Generic);
    // a number with a fraction is no whole bound
    Contract fc;
    CHECK(parse_contract("{\"schema\":\"eh3d.settings.contract/1\",\"groups\":[],\"settings\":[{\"key\":\"a\",\"type\":\"integer\",\"min\":1.5,\"max\":\"9\",\"write\":{\"via\":\"file\"}}]}", fc) == ContractParse::Ok);
    CHECK(!fc.settings[0].has_min && !fc.settings[0].has_max && control_kind(fc.settings[0]) == ControlKind::Text);
}

static void loading_the_live_contract() {
    const Contract& copy = embedded_contract();
    // nobody answered, the answer was too long, a refusal, an answer that is no document: the copy of the screen, read only, and the reason
    Loaded l = load_contract(false, CallError::NoAnswer, Response());
    CHECK(!l.live && l.why == "unreachable" && l.contract.settings.size() == 106 && l.values.empty());
    l = load_contract(false, CallError::NoServer, Response());
    CHECK(!l.live && l.why == "unreachable");
    l = load_contract(false, CallError::TooLong, Response());
    CHECK(!l.live && l.why == "toolong");
    l = load_contract(true, CallError::None, answer(403, "{\"error\":\"forbidden\",\"detail\":\"admin only\"}"));
    CHECK(!l.live && l.why == "forbidden");
    l = load_contract(true, CallError::None, answer(403, "{\"error\":\"forbidden_scope\"}"));
    CHECK(!l.live && l.why == "forbidden_scope");
    l = load_contract(true, CallError::None, answer(401, "{\"error\":\"unauthorized\"}"));
    CHECK(l.why == "unauthorized");
    l = load_contract(true, CallError::None, answer(404, "not found"));
    CHECK(l.why == "http_404");
    l = load_contract(true, CallError::None, answer(500, "{\"error\":\"Bad Thing!\"}"));
    CHECK(l.why == "http_500");  // not shaped like a code
    l = load_contract(true, CallError::None, answer(200, "<html>a router</html>"));
    CHECK(!l.live && l.why == "shape");
    l = load_contract(true, CallError::None, answer(200, contract_json(copy, "eh3d.settings.contract/2")));
    CHECK(!l.live && l.why == "newer" && l.contract.settings.size() == 106);
    l = load_contract(true, CallError::None, answer(200, "{\"schema\":\"eh3d.settings.contract/1\",\"groups\":[],\"settings\":[{\"key\":\"a\"}]}"));
    CHECK(!l.live && l.why == "shape");
    // the reasons are told in the language of the operator
    CHECK(why_copy_text("unreachable", Lang::Es) == "No se puede contactar con el servidor.");
    CHECK(why_copy_text("forbidden", Lang::Pt) == "O servidor só dá as suas definições a um administrador.");
    CHECK(why_copy_text("toolong", Lang::De) == "Die Antwort des Servers ist für den Bildschirm zu lang.");
    CHECK(why_copy_text("newer", Lang::It) == why_copy_text("shape", Lang::It) && !why_copy_text("shape", Lang::It).empty());
    CHECK(why_copy_text("throttled", Lang::En) == server_error_text("throttled", Lang::En) && !server_error_text("throttled", Lang::En).empty());
    CHECK(why_copy_text("http_404", Lang::En) == "http_404" && why_copy_text("", Lang::En).empty());
    // the contract is asked for with a room larger than any other answer
    CHECK(contract_request().method == "GET" && contract_request().path == "/api/v1/settings/contract");
    CHECK(answer_limit(contract_request()) >= 1024u * 1024u && answer_limit(fleet_request()) == 256u * 1024u);
    CHECK(request_carries_token(contract_request()));  // asked with the token like every request but the health check
}

static void groups_and_unlisted_keys() {
    Contract c = small_contract({make("a"), make("z", SType::Boolean, "future"), make("m", SType::Boolean, "security")});
    const std::vector<SettingsGroup> g = group_settings(c);
    CHECK(g.size() == 3 && g[0].id == "network" && g[1].id == "security" && g[2].id == "future");  // a group the contract does not list goes last, nothing is hidden
    CHECK(g[2].label[0] == "future");
    CHECK(group_settings(small_contract({make("a")})).size() == 1);  // a group with nothing in it is left out
    CHECK(group_settings(small_contract({})).empty());
    // a key the server has a value for and the contract does not list is a read-only line
    std::map<std::string, SValue> values = {{"a", SValue::of_bool(true)}, {"zzz", SValue::of_bool(false)}, {"yyy", SValue::of_int(3)}, {"xxx", SValue::of_text("t")}};
    const std::vector<Setting> extra = unlisted_settings(c, values);
    CHECK(extra.size() == 3 && extra[0].key == "xxx" && extra[1].key == "yyy" && extra[2].key == "zzz");  // sorted, the listed one is not there
    CHECK(extra[1].type == SType::Integer && extra[2].type == SType::Boolean && extra[0].type == SType::Unknown);
    for (const Setting& e : extra) {
        CHECK(e.via == "none" && !e.secret);
        CHECK(why_read_only(e, true, "admin", WritePolicy{true, {e.key}, false}, false) == ReadOnlyWhy::Generic);  // never written, whatever hmi.cfg says
    }
}

static void drift_between_the_copy_and_the_server() {
    const Contract& copy = embedded_contract();
    // a server that is exactly the copy: nothing differs
    Loaded same = live_loaded(copy, {});
    CHECK(contract_drift(same).newer_in_server.empty() && contract_drift(same).gone_from_server.empty());
    // a newer server: one key more in the contract, one more only in the values, one key gone
    Contract newer = copy;
    newer.settings.push_back(make("brand_new"));
    newer.settings.erase(newer.settings.begin());
    Loaded l = live_loaded(newer, {{"only_value", SValue::of_bool(true)}});
    const Drift d = contract_drift(l);
    CHECK(d.newer_in_server.size() == 2 && d.newer_in_server[0] == "brand_new" && d.newer_in_server[1] == "only_value");
    CHECK(d.gone_from_server.size() == 1 && d.gone_from_server[0] == copy.settings[0].key);
    // the copy shown has no drift to report
    Loaded notlive;
    notlive.contract = copy;
    CHECK(contract_drift(notlive).newer_in_server.empty());
    // the page says how many keys the copy does not have
    const PageView page = build_page(l, Settings(), Lang::En, "admin", false);
    CHECK(page.warnings.size() == 1 && has(page.warnings[0], "2 settings of the server are not in the copy"));
    CHECK(has(build_page(l, Settings(), Lang::Pt, "admin", false).warnings[0], "2 definições do servidor não estão na cópia"));
}

// ---- the control of each type and what is checked before anything is sent ---------------------------------------------------------------------

static void controls_by_type() {
    Setting sw = make("on");
    Setting num = make("n", SType::Integer);
    num.has_min = num.has_max = true; num.min = 1; num.max = 9;
    Setting free_num = make("free", SType::Integer);
    Setting text = make("t", SType::String);
    Setting list = make("l", SType::List);
    Setting en = make("e", SType::Enum);
    Setting en_empty = make("e2", SType::Enum);
    en.choices = {"a", "b"};
    Setting odd = make("odd", SType::Unknown);
    CHECK(control_kind(sw) == ControlKind::Switch && control_kind(num) == ControlKind::Number && control_kind(free_num) == ControlKind::Text && control_kind(text) == ControlKind::Text);
    CHECK(control_kind(list) == ControlKind::ListOf && control_kind(en) == ControlKind::Select && control_kind(en_empty) == ControlKind::Text && control_kind(odd) == ControlKind::Generic);
    // risk
    for (Risk r : {Risk::Security, Risk::Exposure, Risk::Machine, Risk::Data}) { Setting s = make("r"); s.risk = r; CHECK(is_high_risk(s)); }
    for (Risk r : {Risk::None, Risk::Capacity}) { Setting s = make("r"); s.risk = r; CHECK(!is_high_risk(s)); }
    // roles
    CHECK(role_at_least("admin", "viewer") && role_at_least("operator", "operator") && !role_at_least("viewer", "operator"));
    CHECK(!role_at_least("stranger", "viewer") && !role_at_least("admin", "stranger") && !role_at_least("", "viewer") && !role_at_least("admin", ""));
}

static void typed_input() {
    Setting num = make("n", SType::Integer);
    num.has_min = num.has_max = true; num.min = 1; num.max = 9;
    CHECK(parse_typed(num, "5").ok && parse_typed(num, "5").value == SValue::of_int(5) && parse_typed(num, " 7 ").value.n == 7);
    CHECK(parse_typed(num, "1").ok && parse_typed(num, "9").ok);
    for (const char* bad : {"0", "10", "-1", "", " ", "-", "x", "5x", "5.5", "1e3", "+5", "9999999999999999", "٥"}) {
        const Typed t = parse_typed(num, bad);
        CHECK(!t.ok && t.problem == InputProblem::Number);
    }
    Setting neg = make("neg", SType::Integer);
    neg.has_min = neg.has_max = true; neg.min = -5; neg.max = 5;
    CHECK(parse_typed(neg, "-5").ok && parse_typed(neg, "-5").value.n == -5 && !parse_typed(neg, "-6").ok);
    Setting free_num = make("free", SType::Integer);
    CHECK(parse_typed(free_num, "123456789012345").ok && !parse_typed(free_num, "1234567890123456").ok && parse_typed(free_num, "abc").problem == InputProblem::Whole);
    Setting text = make("t", SType::String);
    text.max_length = 5;
    CHECK(parse_typed(text, "hello").ok && parse_typed(text, "hello").value.s == "hello" && !parse_typed(text, "hellos").ok && parse_typed(text, "hellos").problem == InputProblem::Text);
    CHECK(parse_typed(text, "ñññññ").ok && !parse_typed(text, "ñññññ!").ok);  // the length is in characters, not bytes
    CHECK(parse_typed(text, "").ok);
    Setting en = make("e", SType::Enum);
    en.choices = {"a", "b"};
    CHECK(parse_typed(en, "a").ok && parse_typed(en, "b").value.s == "b" && !parse_typed(en, "c").ok && parse_typed(en, "c").problem == InputProblem::Choice && !parse_typed(en, "").ok);
    Setting list = make("l", SType::List);
    const Typed lt = parse_typed(list, " a , b,, c ");
    CHECK(lt.ok && lt.value.list.size() == 3 && lt.value.list[0] == "a" && lt.value.list[2] == "c");
    CHECK(parse_typed(list, "").ok && parse_typed(list, "").value.list.empty());
    CHECK(!parse_typed(make("odd", SType::Unknown), "x").ok);
    CHECK(typed_switch(make("s"), true).ok && typed_switch(make("s"), true).value.b && !typed_switch(num, true).ok);
    // what the operator is told
    CHECK(problem_text(InputProblem::Number, num, Lang::En) == "Enter a whole number from 1 to 9.");
    CHECK(problem_text(InputProblem::Number, num, Lang::Pt) == "Introduza um número inteiro de 1 a 9.");
    CHECK(problem_text(InputProblem::Text, text, Lang::Fr) == "Trop long : 5 caractères au maximum.");
    CHECK(!problem_text(InputProblem::Choice, en, Lang::De).empty() && problem_text(InputProblem::None, en, Lang::De).empty());
}

static void the_write_request() {
    Setting sw = make("slice_enabled");
    const Request on = write_request(sw, SValue::of_bool(true), false);
    CHECK(on.method == "PUT" && on.path == "/api/v1/auth/policy" && on.body == "{\"key\":\"slice_enabled\",\"value\":true}");
    CHECK(request_carries_token(on) && !is_stop_order(on));
    CHECK(write_request(sw, SValue::of_bool(false), true).body == "{\"key\":\"slice_enabled\",\"value\":false,\"confirm_weak\":true}");
    CHECK(write_request(sw, SValue::of_int(-3), false).body == "{\"key\":\"slice_enabled\",\"value\":-3}");
    CHECK(write_request(sw, SValue::of_text("a\"b\\c"), false).body == "{\"key\":\"slice_enabled\",\"value\":\"a\\\"b\\\\c\"}");
    CHECK(write_request(sw, SValue::of_list({"x", "y"}), false).body == "{\"key\":\"slice_enabled\",\"value\":[\"x\",\"y\"]}");
    Json j;
    CHECK(parse_json(write_request(sw, SValue::of_text("quote\" and \\ and \n"), true).body, j) && j.str("key") == "slice_enabled" && j.str("value") == "quote\" and \\ and \n");  // what is sent is JSON
    // a key that is none of the 19 never gets a body, because the screen does not even try (see the policy tests)
    const std::string wire_text = wire(on, "10.0.0.5:18200", "tok", "eh3d-node-hmi-gui/0.1.1");
    CHECK(has(wire_text, "PUT /api/v1/auth/policy HTTP/1.1") && has(wire_text, "Content-Type: application/json") && has(wire_text, "Authorization: Bearer tok"));
    CHECK(retry_allowed("PUT", CallError::NoAnswer, 0) && retry_allowed("PUT", CallError::Broke, 0));  // the same change twice is the same change
}

static void the_answer_of_a_write() {
    Setting now = make("a");
    Setting later = make("b");
    later.apply = Apply::Restart;
    SaveResult r = interpret_save(now, true, CallError::None, answer(200, "{\"policy\":{},\"changed\":{\"key\":\"a\",\"old\":false,\"new\":true,\"apply\":\"now\",\"restart_required\":false}}"));
    CHECK(r.kind == SaveKind::Saved && !r.restart_required);
    r = interpret_save(now, true, CallError::None, answer(200, "{\"changed\":{\"key\":\"a\",\"restart_required\":true}}"));
    CHECK(r.kind == SaveKind::Saved && r.restart_required);  // the server says so
    r = interpret_save(later, true, CallError::None, answer(200, "{\"changed\":{\"key\":\"b\"}}"));
    CHECK(r.kind == SaveKind::Saved && r.restart_required);  // it does not say: the apply of the contract decides
    r = interpret_save(now, true, CallError::None, answer(200, "{}"));
    CHECK(r.kind == SaveKind::Saved && !r.restart_required);
    r = interpret_save(later, true, CallError::None, answer(200, "not json"));
    CHECK(r.kind == SaveKind::Refused && r.code == "bad_answer");  // a router page that says 200 saved nothing
    r = interpret_save(later, true, CallError::None, answer(200, "<html>ok</html>"));
    CHECK(r.kind == SaveKind::Refused && r.code == "bad_answer");
    r = interpret_save(later, true, CallError::None, answer(200, ""));
    CHECK(r.kind == SaveKind::Refused);
    r = interpret_save(later, true, CallError::None, answer(200, "[1]"));
    CHECK(r.kind == SaveKind::Refused);
    r = interpret_save(now, true, CallError::None, answer(200, "{\"changed\":5}"));
    CHECK(r.kind == SaveKind::Saved);
    r = interpret_save(now, true, CallError::None, answer(409, "{\"error\":\"weak_confirm_required\",\"detail\":\"looser than recommended\"}"));
    CHECK(r.kind == SaveKind::NeedsWeakConfirm && r.code == "weak_confirm_required");
    r = interpret_save(now, true, CallError::None, answer(422, "{\"error\":\"bad_policy\",\"detail\":\"password_min_len: 3 is below 8\"}"));
    CHECK(r.kind == SaveKind::Refused && r.code == "bad_policy" && r.detail == "password_min_len: 3 is below 8");
    r = interpret_save(now, true, CallError::None, answer(403, "{\"error\":\"forbidden_scope\"}"));
    CHECK(r.kind == SaveKind::Refused && r.code == "forbidden_scope");
    r = interpret_save(now, true, CallError::None, answer(503, "<html>gateway</html>"));
    CHECK(r.kind == SaveKind::Refused && r.code == "http_503" && r.detail.empty());
    r = interpret_save(now, false, CallError::NoAnswer, Response());
    CHECK(r.kind == SaveKind::Unreachable && r.error == CallError::NoAnswer);
    // the sentences
    Settings cfg;
    cfg.language = Lang::Es;
    SaveResult saved;
    saved.kind = SaveKind::Saved;
    CHECK(save_text(saved, now, Lang::En, cfg) == "Saved: Label a en");
    saved.restart_required = true;
    CHECK(save_text(saved, now, Lang::Es, cfg) == "Guardado: Label a es. Se aplica al reiniciar el servidor.");
    SaveResult refused = interpret_save(now, true, CallError::None, answer(403, "{\"error\":\"forbidden\"}"));
    CHECK(has(save_text(refused, now, Lang::En, cfg), "The server refused it: ") && save_text(refused, now, Lang::En, cfg).size() > 25);
    SaveResult odd = interpret_save(now, true, CallError::None, answer(422, "{\"error\":\"a_code_of_the_future\",\"detail\":\"x\"}"));
    CHECK(has(save_text(odd, now, Lang::En, cfg), "a_code_of_the_future: x"));  // a code the table does not have is shown as it is
    SaveResult gone = interpret_save(now, false, CallError::NoAnswer, Response());
    CHECK(has(save_text(gone, now, Lang::En, cfg), "Nothing was sent: "));
    CHECK(save_text(interpret_save(now, true, CallError::None, answer(409, "{\"error\":\"weak_confirm_required\"}")), now, Lang::En, cfg) == settings_text(ST::AskWeak, Lang::En));
}

// ---- who may change what ----------------------------------------------------------------------------------------------------------------------

static void the_write_policy() {
    // by default the screen changes nothing
    Settings d;
    CHECK(d.settings_contract && d.settings_poll_s == 600 && !d.settings_write && d.settings_write_keys.empty() && d.settings_block_running);
    const WritePolicy none = write_policy(d);
    CHECK(!none.enabled && none.keys.empty() && none.block_running);
    // the file
    std::vector<Problem> problems;
    Settings s = parse_settings("settings_write = on\nsettings_write_keys = slice_enabled, design_run\nsettings_poll_s = 60\nsettings_contract = off\nsettings_block_running = off\n", problems);
    CHECK(problems.empty() && s.settings_write && s.settings_write_keys == "slice_enabled, design_run" && s.settings_poll_s == 60 && !s.settings_contract && !s.settings_block_running);
    const WritePolicy p = write_policy(s);
    CHECK(p.enabled && p.keys.size() == 2 && p.keys.count("design_run") == 1 && !p.block_running);
    // a line that cannot be used changes nothing, so a typing mistake never opens the writing
    for (const char* bad : {"settings_write = maybe", "settings_write = 2", "settings_poll_s = 29", "settings_poll_s = 86401", "settings_poll_s = x", "settings_write_keys = Slice",
                            "settings_write_keys = a b", "settings_write_keys = a,,b", "settings_write_keys = a,a", "settings_write_keys = a-b", "settings_contract = 7", "settings_block_running = no no"}) {
        std::vector<Problem> pr;
        const Settings b = parse_settings(std::string(bad) + "\n", pr);
        CHECK(pr.size() == 1);
        CHECK(!b.settings_write && b.settings_write_keys.empty() && b.settings_poll_s == 600 && b.settings_contract && b.settings_block_running);
    }
    {
        std::vector<Problem> pr;
        const Settings many = parse_settings("settings_write_keys = " + std::string("k1,k2,k3,k4,k5,k6,k7,k8,k9,k10,k11,k12,k13,k14,k15,k16,k17,k18,k19,k20,k21,k22,k23,k24,k25,k26,k27,k28,k29,k30,k31,k32,k33") + "\n", pr);
        CHECK(pr.size() == 1 && many.settings_write_keys.empty());  // up to 32 keys
        std::vector<Problem> pr2;
        const Settings empty = parse_settings("settings_write_keys =\n", pr2);
        CHECK(pr2.empty() && empty.settings_write_keys.empty());
    }
    std::vector<std::string> keys;
    CHECK(parse_key_list("", keys) && keys.empty() && parse_key_list(" a_1 , b2 ", keys) && keys.size() == 2 && !parse_key_list("A", keys) && !parse_key_list(",", keys));
    // the matrix of the reasons
    const Contract& c = embedded_contract();
    const Setting& policy_key = *c.find("slice_enabled");
    const Setting& file_key = *c.find("listen");
    const WritePolicy open{true, {"slice_enabled", "listen"}, true};
    CHECK(why_read_only(file_key, true, "admin", open, false) == ReadOnlyWhy::File);   // a key set in server.cfg is said to be so whatever else holds
    CHECK(why_read_only(file_key, false, "", none, false) == ReadOnlyWhy::File);
    CHECK(why_read_only(policy_key, false, "admin", open, false) == ReadOnlyWhy::Copy);  // the copy shows, never writes
    CHECK(why_read_only(policy_key, true, "operator", open, false) == ReadOnlyWhy::Role);
    CHECK(why_read_only(policy_key, true, "viewer", open, false) == ReadOnlyWhy::Role);
    CHECK(why_read_only(policy_key, true, "", open, false) == ReadOnlyWhy::Role);        // a screen that never logged in does not know its role
    CHECK(why_read_only(policy_key, true, "root", open, false) == ReadOnlyWhy::Role);
    CHECK(why_read_only(policy_key, true, "admin", none, false) == ReadOnlyWhy::Disabled);
    CHECK(why_read_only(policy_key, true, "admin", WritePolicy{true, {}, true}, false) == ReadOnlyWhy::NotListed);
    CHECK(why_read_only(policy_key, true, "admin", WritePolicy{true, {"other"}, true}, false) == ReadOnlyWhy::NotListed);
    CHECK(why_read_only(policy_key, true, "admin", open, true) == ReadOnlyWhy::Running);
    CHECK(why_read_only(policy_key, true, "admin", WritePolicy{true, {"slice_enabled"}, false}, true) == ReadOnlyWhy::None);  // the block is a switch
    CHECK(why_read_only(policy_key, true, "admin", open, false) == ReadOnlyWhy::None);
    // a secret is never written, a route the screen does not trust is never used, a type it does not know is never edited
    Setting secret = make("pw", SType::String);
    secret.secret = true;
    CHECK(why_read_only(secret, true, "admin", WritePolicy{true, {"pw"}, false}, false) == ReadOnlyWhy::Secret);
    Setting other_route = make("x");
    other_route.path = "/api/v1/machines/m1/stop";
    CHECK(why_read_only(other_route, true, "admin", WritePolicy{true, {"x"}, false}, false) == ReadOnlyWhy::Route);
    other_route.path = "/api/v1/auth/policy/";
    CHECK(why_read_only(other_route, true, "admin", WritePolicy{true, {"x"}, false}, false) == ReadOnlyWhy::Route);
    other_route.path = kPolicyPath;
    other_route.method = "POST";
    CHECK(why_read_only(other_route, true, "admin", WritePolicy{true, {"x"}, false}, false) == ReadOnlyWhy::Route);
    Setting odd = make("odd", SType::Unknown);
    CHECK(why_read_only(odd, true, "admin", WritePolicy{true, {"odd"}, false}, false) == ReadOnlyWhy::Generic);
    Setting no_role = make("free");
    no_role.role.clear();
    CHECK(why_read_only(no_role, true, "", WritePolicy{true, {"free"}, false}, false) == ReadOnlyWhy::None);  // the contract asks for no role
    // every reason has a sentence in every language
    for (ReadOnlyWhy w : {ReadOnlyWhy::File, ReadOnlyWhy::Secret, ReadOnlyWhy::Copy, ReadOnlyWhy::Route, ReadOnlyWhy::Generic, ReadOnlyWhy::Role, ReadOnlyWhy::Disabled, ReadOnlyWhy::NotListed, ReadOnlyWhy::Running})
        for (std::size_t l = 0; l < kBaseLangCount; ++l) CHECK(!settings_text(read_only_text(w), static_cast<Lang>(l)).empty());
    // with the whole copy: of the 106 settings the screen can change none by default, and with every switch open exactly the 19 keys of the security page
    int by_default = 0, all_open = 0;
    std::set<std::string> all_keys;
    for (const Setting& s2 : c.settings) all_keys.insert(s2.key);
    const WritePolicy everything{true, all_keys, false};
    for (const Setting& s2 : c.settings) {
        if (why_read_only(s2, true, "admin", none, false) == ReadOnlyWhy::None) ++by_default;
        if (why_read_only(s2, true, "admin", everything, false) == ReadOnlyWhy::None) ++all_open;
    }
    CHECK(by_default == 0 && all_open == 19);
}

// ---- a change, step by step ------------------------------------------------------------------------------------------------------------------

static void the_steps_of_a_change() {
    Setting low = make("low");
    Setting risky = make("risky");
    risky.risk = Risk::Security;
    Request out;
    SettingsEditor e;
    CHECK(e.step() == EditStep::Idle && !e.busy() && e.question(Lang::En).empty() && e.remaining_ms(0) == 0);
    CHECK(!e.confirm(0, out) && e.step() == EditStep::Idle);  // nothing to confirm
    // a setting of no risk: one question, then the request
    CHECK(e.begin(low, SValue::of_bool(true), ReadOnlyWhy::None, false, 1000, 8000) == BeginResult::Started);
    CHECK(e.step() == EditStep::Ask && e.busy() && e.key() == "low" && e.remaining_ms(1000) == 8000 && e.remaining_ms(5000) == 4000);
    CHECK(e.question(Lang::En) == "Change Label low en to On?" && e.question(Lang::Es) == "¿Cambiar Label low es a Activado?");
    CHECK(e.confirm(2000, out) && out.method == "PUT" && out.body == "{\"key\":\"low\",\"value\":true}" && e.step() == EditStep::Sent);
    CHECK(!e.confirm(2001, out));  // a second touch sends nothing more
    e.on_result(interpret_save(low, true, CallError::None, answer(200, "{}")), 2100);
    CHECK(e.step() == EditStep::Idle && e.key().empty());
    // a risky one asks twice, and shows the warning of the contract the second time
    CHECK(e.begin(risky, SValue::of_bool(false), ReadOnlyWhy::None, false, 0, 8000) == BeginResult::Started);
    CHECK(!e.confirm(100, out) && e.step() == EditStep::AskRisk);
    CHECK(has(e.question(Lang::En), "Warning risky en") && has(e.question(Lang::En), "Confirm again to apply it."));
    CHECK(has(e.question(Lang::Pt), "Warning risky pt") && has(e.question(Lang::Pt), "Confirme outra vez"));
    CHECK(e.remaining_ms(100) == 8000);  // each question has its own time
    CHECK(e.confirm(200, out) && e.step() == EditStep::Sent && out.body == "{\"key\":\"risky\",\"value\":false}");
    // the server calls it looser than recommended: a third question, then the same change with confirm_weak
    e.on_result(interpret_save(risky, true, CallError::None, answer(409, "{\"error\":\"weak_confirm_required\"}")), 300);
    CHECK(e.step() == EditStep::AskWeak && e.question(Lang::En) == settings_text(ST::AskWeak, Lang::En) && e.remaining_ms(300) == 8000);
    CHECK(e.confirm(400, out) && out.body == "{\"key\":\"risky\",\"value\":false,\"confirm_weak\":true}" && e.step() == EditStep::Sent);
    e.on_result(interpret_save(risky, true, CallError::None, answer(409, "{\"error\":\"weak_confirm_required\"}")), 500);
    CHECK(e.step() == EditStep::Idle);  // asked once; a server that asks again ends the change
    // cancel at every step
    CHECK(e.begin(risky, SValue::of_bool(true), ReadOnlyWhy::None, false, 0, 8000) == BeginResult::Started);
    e.cancel();
    CHECK(e.step() == EditStep::Idle && !e.confirm(1, out));
    CHECK(e.begin(risky, SValue::of_bool(true), ReadOnlyWhy::None, false, 0, 8000) == BeginResult::Started && !e.confirm(1, out));
    e.cancel();
    CHECK(e.step() == EditStep::Idle);
    // nothing under way is sent twice or mixed with another change
    CHECK(e.begin(low, SValue::of_bool(true), ReadOnlyWhy::None, false, 0, 8000) == BeginResult::Started);
    CHECK(e.begin(risky, SValue::of_bool(true), ReadOnlyWhy::None, false, 0, 8000) == BeginResult::Busy && e.key() == "low");
    CHECK(e.confirm(10, out) && e.begin(risky, SValue::of_bool(true), ReadOnlyWhy::None, false, 20, 8000) == BeginResult::Busy);
    e.on_result(interpret_save(low, false, CallError::NoAnswer, Response()), 30);
    CHECK(e.step() == EditStep::Idle);
    // a read-only setting and a spoken order start nothing
    CHECK(e.begin(low, SValue::of_bool(true), ReadOnlyWhy::Role, false, 0, 8000) == BeginResult::ReadOnly && e.step() == EditStep::Idle);
    CHECK(e.begin(low, SValue::of_bool(true), ReadOnlyWhy::None, true, 0, 8000) == BeginResult::ByVoice && e.step() == EditStep::Idle);
    // a touch that comes when the time of a question is over sends nothing, even before the next tick
    SettingsEditor late;
    CHECK(late.begin(low, SValue::of_bool(true), ReadOnlyWhy::None, false, 1000, 8000) == BeginResult::Started);
    CHECK(!late.confirm(9000, out) && late.step() == EditStep::Idle);
    CHECK(late.begin(risky, SValue::of_bool(true), ReadOnlyWhy::None, false, 0, 8000) == BeginResult::Started);
    CHECK(!late.confirm(100, out) && !late.confirm(8100, out) && late.step() == EditStep::Idle);
    // a question nobody answered is given up by itself
    CHECK(e.begin(risky, SValue::of_bool(true), ReadOnlyWhy::None, false, 1000, 8000) == BeginResult::Started);
    CHECK(!e.tick(8999) && e.step() == EditStep::Ask && e.remaining_ms(8999) == 1);
    CHECK(e.tick(9000) && e.step() == EditStep::Idle && e.remaining_ms(9000) == 0);
    CHECK(!e.tick(9001));
    // the second question has its own time, counted from the first touch
    CHECK(e.begin(risky, SValue::of_bool(true), ReadOnlyWhy::None, false, 0, 8000) == BeginResult::Started);
    CHECK(!e.confirm(7000, out) && !e.tick(14999) && e.tick(15000) && e.step() == EditStep::Idle);
    // a clock that wraps does not give up early or never
    const std::uint32_t near_wrap = 0xFFFFFFFFu - 3000u;
    CHECK(e.begin(low, SValue::of_bool(true), ReadOnlyWhy::None, false, near_wrap, 8000) == BeginResult::Started);
    CHECK(!e.tick(near_wrap + 7999u) && e.remaining_ms(0) == 4999u && e.tick(near_wrap + 8000u));
    // an answer that arrives when nothing was sent changes nothing
    e.on_result(interpret_save(low, true, CallError::None, answer(200, "{}")), 0);
    CHECK(e.step() == EditStep::Idle);
}

// ---- the page ------------------------------------------------------------------------------------------------------------------------------------

static const RowView* row_of(const PageView& p, const std::string& key) {
    for (const GroupView& g : p.groups)
        for (const RowView& r : g.rows)
            if (r.key == key) return &r;
    return nullptr;
}

static void the_page_of_the_copy() {
    Loaded copy;
    copy.contract = embedded_contract();
    copy.why = "forbidden";
    Settings cfg;
    const PageView p = build_page(copy, cfg, Lang::En, "operator", false);
    CHECK(!p.live && p.source == "Copy kept in the screen: read only, without values" && p.why == "The server gives its settings to an administrator only.");
    CHECK(p.groups.size() == 17 && p.row_count == 106 && p.editable_count == 0 && p.warnings.empty());
    for (const GroupView& g : p.groups) {
        CHECK(!g.label.empty());
        for (const RowView& r : g.rows) {
            CHECK(!r.label.empty() && r.value == "-" && !r.editable && !r.note.empty() && !r.not_default);
            CHECK(r.why == ReadOnlyWhy::File || r.why == ReadOnlyWhy::Copy);
        }
    }
    const RowView* listen = row_of(p, "listen");
    CHECK(listen != nullptr && listen->label == "Listen address" && listen->why == ReadOnlyWhy::File && has(listen->note, "server.cfg"));
    const RowView* slice = row_of(p, "slice_enabled");
    CHECK(slice != nullptr && slice->why == ReadOnlyWhy::Copy && slice->control == ControlKind::Switch && slice->high_risk && !slice->restart);
    CHECK(row_of(p, "no_such_key") == nullptr);
    // every language shows its own labels (or the English ones when the fonts cannot draw them)
    for (std::size_t l = 0; l < kLangCount; ++l) {
        const PageView q = build_page(copy, cfg, static_cast<Lang>(l), "", false);
        CHECK(q.row_count == 106);
        for (const GroupView& g : q.groups) {
            CHECK(!g.label.empty());
            for (const RowView& r : g.rows) CHECK(!r.label.empty() && text_in_font(r.label.c_str()) && text_in_font(r.note.c_str()));
        }
    }
    CHECK(row_of(build_page(copy, cfg, Lang::Es, "", false), "listen")->label == "Dirección de escucha");
    CHECK(row_of(build_page(copy, cfg, Lang::Pt, "", false), "listen")->label == "Endereço de escuta");
}

static void the_page_of_the_live_contract() {
    Setting flag = make("flag");
    flag.risk = Risk::Exposure;
    flag.has_default = true;
    flag.def = SValue::of_bool(false);
    Setting num = make("hours", SType::Integer, "security");
    num.has_min = num.has_max = true; num.min = 1; num.max = 720;
    num.unit = "hours";
    num.has_default = true;
    num.def = SValue::of_int(12);
    num.apply = Apply::Restart;
    num.via = "file"; num.method.clear(); num.path.clear(); num.file = "server.cfg"; num.role.clear(); num.scope.clear();
    Setting secret = make("pw", SType::String);
    secret.secret = true;
    Setting names = make("names", SType::List);
    names.via = "file"; names.method.clear(); names.path.clear();
    Setting plain = make("plain", SType::Integer);
    plain.has_min = plain.has_max = true; plain.min = 1; plain.max = 9;
    plain.unit = "lines";
    plain.has_default = true;
    plain.def = SValue::of_int(3);
    Contract c = small_contract({flag, num, secret, names, plain});
    Settings cfg;
    cfg.settings_write = true;
    cfg.settings_write_keys = "flag, plain";
    const Loaded l = live_loaded(c, {{"flag", SValue::of_bool(true)}, {"hours", SValue::of_int(24)}, {"pw", SValue::of_text("hunter2")}, {"names", SValue::of_list({"a", "b"})}, {"plain", SValue::of_int(3)}, {"future", SValue::of_bool(true)}});
    const PageView p = build_page(l, cfg, Lang::En, "admin", false);
    CHECK(p.live && p.source == "Values in force, read from the server (9.9.9)" && p.why.empty());
    CHECK(p.groups.size() == 3 && p.groups[0].id == "network" && p.groups[1].id == "security" && p.groups[2].id == "other");  // the key nobody described goes last
    CHECK(p.row_count == 6 && p.editable_count == 2);
    const RowView* f = row_of(p, "flag");
    CHECK(f != nullptr && f->value == "On" && f->editable && f->high_risk && f->not_default && f->control == ControlKind::Switch && f->why == ReadOnlyWhy::None);
    const RowView* h = row_of(p, "hours");
    CHECK(h != nullptr && h->value == "24 h" && !h->editable && h->not_default && h->restart && h->why == ReadOnlyWhy::File && h->control == ControlKind::Number);
    const RowView* pw = row_of(p, "pw");
    CHECK(pw != nullptr && pw->value == "Hidden" && pw->why == ReadOnlyWhy::Secret && !pw->editable);  // the value the server sent for a secret is never shown
    CHECK(!has(pw->value, "hunter2") && !has(pw->label, "hunter2") && !has(pw->note, "hunter2"));
    const RowView* n = row_of(p, "names");
    CHECK(n != nullptr && n->value == "a, b" && !n->editable);
    const RowView* pl = row_of(p, "plain");
    CHECK(pl != nullptr && pl->value == "3" && pl->editable && !pl->not_default && !pl->restart && pl->note.empty());  // a unit of a count is left out
    const RowView* fu = row_of(p, "future");
    CHECK(fu != nullptr && fu->value == "On" && fu->why == ReadOnlyWhy::Generic && !fu->editable && fu->label == "future");
    // the same page for an operator, with the writing off, or with a job running
    CHECK(build_page(l, cfg, Lang::En, "operator", false).editable_count == 0);
    CHECK(build_page(l, Settings(), Lang::En, "admin", false).editable_count == 0);
    CHECK(build_page(l, cfg, Lang::En, "admin", true).editable_count == 0 && row_of(build_page(l, cfg, Lang::En, "admin", true), "flag")->why == ReadOnlyWhy::Running);
    Settings free_running = cfg;
    free_running.settings_block_running = false;
    CHECK(build_page(l, free_running, Lang::En, "admin", true).editable_count == 2);
    // the words follow the language of the operator
    const PageView es = build_page(l, cfg, Lang::Es, "admin", false);
    CHECK(row_of(es, "flag")->value == "Activado" && row_of(es, "flag")->label == "Label flag es" && es.source.find("Valores en vigor") == 0);
    CHECK(row_of(build_page(l, cfg, Lang::De, "admin", false), "flag")->value == "Ein");
    // a value that is a switch the server reports as absent is a dash
    const Loaded sparse = live_loaded(c, {});
    CHECK(row_of(build_page(sparse, cfg, Lang::En, "admin", false), "flag")->value == "-" && !row_of(build_page(sparse, cfg, Lang::En, "admin", false), "flag")->not_default);
}

static void what_the_server_says_against_hmi_cfg() {
    Settings cfg;
    cfg.rack_places = "1-14";
    const Contract& copy = embedded_contract();
    Loaded l = live_loaded(copy, {{"rack_max_slots", SValue::of_int(14)}, {"max_body_mb", SValue::of_int(8)}});
    CHECK(build_page(l, cfg, Lang::En, "admin", false).warnings.empty());
    l.values["rack_max_slots"] = SValue::of_int(6);
    const PageView p = build_page(l, cfg, Lang::En, "admin", false);
    CHECK(p.warnings.size() == 1 && p.warnings[0] == "The tool-changer bar of the server has 6 places but hmi.cfg uses up to place 14 (rack_places).");
    cfg.rack_places = "1-6";
    CHECK(build_page(l, cfg, Lang::En, "admin", false).warnings.empty());
    cfg.rack_places = "2,9";
    CHECK(build_page(l, cfg, Lang::Pt, "admin", false).warnings[0] == "A barra do trocador do servidor tem 6 lugares mas hmi.cfg usa até ao lugar 9 (rack_places).");
    l.values["rack_max_slots"] = SValue::of_text("six");
    CHECK(build_page(l, cfg, Lang::En, "admin", false).warnings.empty());  // a value that is no number says nothing
    // a copy knows no value, so it warns of nothing
    Loaded notlive;
    notlive.contract = copy;
    CHECK(cross_check_lines(notlive, cfg, Lang::En).empty() && body_warning(1u << 30, notlive, Lang::En).empty());
    // the body of a request
    CHECK(body_warning(8u * 1024 * 1024, l, Lang::En).empty());
    CHECK(body_warning(8u * 1024 * 1024 + 1, l, Lang::En) == "The file is larger than the 8 MB the server takes in a request.");
    CHECK(body_warning(9000000, l, Lang::Fr) == "Le fichier dépasse les 8 Mo que le serveur accepte dans une requête.");
    CHECK(body_warning(0, l, Lang::En).empty());
    l.values.erase("max_body_mb");
    CHECK(body_warning(1ull << 40, l, Lang::En).empty());
    l.values["max_body_mb"] = SValue::of_int(0);
    CHECK(body_warning(1ull << 40, l, Lang::En).empty());
}

static void when_the_contract_is_read() {
    ContractPoller p;
    CHECK(!p.due(false, 0));  // switched off in hmi.cfg
    CHECK(p.due(true, 0) && p.due(true, 123456));  // never read: now
    p.done(true, 600, 1000);
    CHECK(!p.due(true, 1000) && !p.due(true, 600999) && p.due(true, 601000) && p.due(true, 700000));
    p.done(false, 600, 601000);  // failed: again after five minutes at most
    CHECK(!p.due(true, 900999) && p.due(true, 901000));
    p.done(false, 30, 0);  // a short poll time is kept
    CHECK(!p.due(true, 29999) && p.due(true, 30000));
    p.done(true, 600, 5000);
    p.again();  // the login changed: at the next round
    CHECK(p.due(true, 5001));
    p.done(true, 86400, 0xFFFFFF00u);  // the clock wraps
    CHECK(!p.due(true, 0xFFFFFF01u) && p.due(true, 0xFFFFFF00u + 86400000u));
}

static void the_login_knows_its_role() {
    SessionTracker t;
    CHECK(t.role().empty());
    LoginView v;
    CHECK(parse_login("{\"token\":\"abcdefgh12345678\",\"name\":\"screen\",\"role\":\"operator\",\"app\":\"hmi\",\"expires_s\":3600}", v) && v.role == "operator");
    t.on_login(v, 0);
    CHECK(t.role() == "operator" && t.logged_in());
    t.on_status(401);
    CHECK(t.role().empty() && !t.logged_in());  // a login that is gone takes its role with it
    t.on_login(v, 0);
    t.forget();
    CHECK(t.role().empty());
}

// ---- the texts ---------------------------------------------------------------------------------------------------------------------------------------

static std::set<std::string> holes_of(const std::string& text) {
    std::set<std::string> out;
    for (std::size_t at = text.find('{'); at != std::string::npos; at = text.find('{', at + 1)) {
        const std::size_t end = text.find('}', at);
        if (end == std::string::npos) break;
        out.insert(text.substr(at, end - at + 1));
    }
    return out;
}

static void the_texts() {
    int not_drawable = 0;
    for (unsigned t = 0; t < static_cast<unsigned>(ST::Count); ++t) {
        const Row& row = settings_text_row(static_cast<ST>(t));
        const std::set<std::string> english = holes_of(row[0]);
        for (std::size_t l = 0; l < kBaseLangCount; ++l) {
            CHECK(row[l] != nullptr && row[l][0] != '\0');
            CHECK(holes_of(row[l]) == english);  // the same holes in every language
            const std::string shown = settings_text(static_cast<ST>(t), static_cast<Lang>(l));
            CHECK(!shown.empty() && (shown == row[l] || shown == row[0]));
            if (shown != row[l]) ++not_drawable;
            // the words that never change
            for (const char* keep : {"hmi.cfg", "server.cfg", "settings_write", "settings_write_keys", "rack_places"}) CHECK(has(row[0], keep) == has(row[l], keep));
        }
        CHECK(text_in_font(row[0]));
    }
    CHECK(not_drawable == 0);  // every sentence of every language is drawn as it is written
    // the formal register of the three languages that have one
    for (unsigned t = 0; t < static_cast<unsigned>(ST::Count); ++t) {
        const Row& row = settings_text_row(static_cast<ST>(t));
        CHECK(!has(row[3], " du ") && !has(row[3], "Dir ") && !has(row[4], " tu ") && !has(row[4], " toi ") && !has(row[5], " tu ") && !has(row[5], " teu "));
    }
    CHECK(fill("Change {label} to {value}?", {{"label", "A"}, {"value", "B"}}) == "Change A to B?");
    CHECK(fill("{n} and {n}", {{"n", "{n}"}}) == "{n} and {n}");  // a value that holds a hole is not filled again
    CHECK(fill("no holes", {{"x", "y"}}) == "no holes" && fill("{x}", {}) == "{x}");
    CHECK(settings_text(ST::Title, Lang::Ja) == "サーバー設定" || settings_text(ST::Title, Lang::Ja) == "Server settings");
    CHECK(settings_text(ST::Title, Lang::En) == "Server settings" && settings_text(ST::Title, Lang::Fr) == "Réglages du serveur" && settings_text(ST::Title, Lang::Pt) == "Definições do servidor");
    CHECK(contract_text(Texts(), Lang::En, "key") == "key");
    Texts only_ja;
    only_ja[7] = "x";
    CHECK(contract_text(only_ja, Lang::En, "key") == "x" && contract_text(only_ja, Lang::Ja, "key") == "x");
    Texts en_only;
    en_only[0] = "English";
    CHECK(contract_text(en_only, Lang::Es, "k") == "English" && contract_text(en_only, Lang::Zh, "k") == "English");
}

// ---- a storm of failures against a simulated server -------------------------------------------------------------------------------------------

/// A server that holds the values of the keys of the policy, takes a change through PUT /auth/policy, can ask for `confirm_weak`, and can fail: no answer, a 503, a router page, a 403.
static const std::vector<std::string> kStormKeys = {"slice_enabled", "design_run", "login_ip_limit", "password_min_len", "compat_write", "listen", "max_body_mb"};

/// The contract the simulated server gives: the settings of the storm taken from the copy (the whole copy is read in other tests; here the work is in the steps of a change).
static Contract storm_contract() {
    Contract c;
    c.schema = kContractSchema;
    c.groups = embedded_contract().groups;
    for (const std::string& k : kStormKeys) c.settings.push_back(*embedded_contract().find(k));
    return c;
}

struct FakeServer {
    std::map<std::string, SValue> values;
    int puts = 0, weak_puts = 0, gets = 0;
    bool weak_needed = false;
    std::uint32_t mode = 0;  // 0 good, 1 no answer, 2 503, 3 router page, 4 forbidden
    bool call(const Request& r, Response& out, CallError& error) {
        error = CallError::None;
        if (mode == 1) { error = CallError::NoAnswer; return false; }
        if (mode == 2) { out = answer(503, "{\"error\":\"unavailable\"}"); return true; }
        if (mode == 3) { out = answer(200, "<html>router</html>"); return true; }
        if (mode == 4) { out = answer(403, "{\"error\":\"forbidden\"}"); return true; }
        if (r.method == "GET" && r.path == kContractPath) {
            ++gets;
            std::string v = "\"values\":{";
            bool first = true;
            for (const std::string& k : kStormKeys) { v += (first ? "" : ",") + json_quote(k) + ":" + value_json_of(values[k]); first = false; }
            out = answer(200, contract_json(storm_contract(), kContractSchema, "\"server_version\":\"sim\"," + v + "}"));
            return true;
        }
        if (r.method == "PUT" && r.path == kPolicyPath) {
            ++puts;
            Json j;
            if (!parse_json(r.body, j)) { out = answer(400, "{\"error\":\"bad_json\"}"); return true; }
            const Json* conf = j.get("confirm_weak");
            const bool weak_ok = conf != nullptr && conf->type == Json::Type::Bool && conf->b;
            if (weak_ok) ++weak_puts;
            if (weak_needed && !weak_ok) { out = answer(409, "{\"error\":\"weak_confirm_required\"}"); return true; }
            const Setting* s = embedded_contract().find(j.str("key"));
            if (s == nullptr || s->via != "policy") { out = answer(422, "{\"error\":\"bad_policy\",\"detail\":\"" + j.str("key") + "\"}"); return true; }
            const Json* val = j.get("value");
            if (val == nullptr) { out = answer(422, "{\"error\":\"bad_policy\"}"); return true; }
            values[s->key] = detail::value_of(*val);
            out = answer(200, std::string("{\"changed\":{\"key\":\"") + s->key + "\",\"restart_required\":" + (s->apply == Apply::Restart ? "true" : "false") + "}}");
            return true;
        }
        out = answer(404, "{\"error\":\"not_found\"}");
        return true;
    }
};

static std::uint64_t storm(std::uint32_t seed, int rounds, int& saved_count) {
    Lcg rng{seed};
    FakeServer server;
    for (const Setting& s : embedded_contract().settings) server.values[s.key] = s.has_default ? s.def : SValue();
    Settings cfg;
    cfg.settings_write = true;
    cfg.settings_write_keys = "slice_enabled, design_run, login_ip_limit, password_min_len";
    const WritePolicy policy = write_policy(cfg);
    const std::vector<std::string>& keys = kStormKeys;
    SettingsEditor editor;
    std::uint32_t now = 0;
    std::uint64_t trace = 1469598103934665603ull;
    auto mix = [&](std::uint64_t v) { trace = (trace ^ v) * 1099511628211ull; };
    for (int i = 0; i < rounds; ++i) {
        now += 100 + rng.below(4000);
        server.mode = rng.below(10) < 6 ? 0 : 1 + rng.below(4);
        server.weak_needed = rng.below(3) == 0;
        // read the contract
        Response r;
        CallError error;
        const bool reached = server.call(contract_request(), r, error);
        const Loaded got = load_contract(reached, error, r);
        mix(got.live ? 1 : 0);
        // the copy never lets a change start
        const std::string role = rng.below(5) == 0 ? "operator" : "admin";
        const Setting& s = *embedded_contract().find(keys[rng.below(static_cast<std::uint32_t>(keys.size()))]);
        const bool running = rng.below(6) == 0;
        const ReadOnlyWhy why = why_read_only(s, got.live, role, policy, running);
        const SValue before = server.values[s.key];
        const int puts_before = server.puts;
        SValue next = s.type == SType::Boolean ? SValue::of_bool(rng.below(2) == 1) : SValue::of_int(static_cast<long long>(s.has_min ? s.min : 0) + rng.below(5));
        const BeginResult b = editor.begin(s, next, why, rng.below(8) == 0, now, 8000);
        if (!got.live) CHECK(b != BeginResult::Started);
        if (role != "admin") CHECK(b != BeginResult::Started);
        if (s.via == "file") CHECK(b != BeginResult::Started);
        if (running) CHECK(b != BeginResult::Started);
        mix(static_cast<std::uint64_t>(b));
        if (b == BeginResult::Started) {
            // the operator answers each question, sometimes not at all
            Request req;
            int questions = 0;
            bool sent = false;
            int expected_puts = puts_before;
            while (editor.step() != EditStep::Idle && editor.step() != EditStep::Sent && questions < 4) {
                ++questions;
                CHECK(server.puts == expected_puts);  // nothing is sent while a question is open
                const std::uint32_t pick = rng.below(10);
                if (pick == 0) { editor.cancel(); break; }
                if (pick == 1) { now += 9000; CHECK(editor.tick(now)); break; }
                now += 50;
                if (editor.confirm(now, req)) {
                    sent = true;
                    Response resp;
                    CallError err;
                    const bool ok = server.call(req, resp, err);
                    expected_puts = server.puts;
                    const SaveResult result = interpret_save(s, ok, err, resp);
                    mix(static_cast<std::uint64_t>(result.kind));
                    editor.on_result(result, now);
                    if (result.kind == SaveKind::Saved) { ++saved_count; CHECK(server.values[s.key] == next); }
                    else if (result.kind != SaveKind::NeedsWeakConfirm) CHECK(server.values[s.key] == before);
                }
            }
            if (editor.step() == EditStep::Sent) editor.cancel();
            if (!sent) CHECK(server.puts == puts_before && server.values[s.key] == before);  // no touch, no change
        }
        CHECK(!editor.busy() || editor.step() == EditStep::Idle);
        editor.cancel();
        // a change without confirm_weak that the server wanted it for was never accepted
        CHECK(server.weak_puts <= server.puts);
    }
    for (const auto& kv : server.values) mix(std::hash<std::string>{}(kv.first + value_text(kv.second)));
    mix(static_cast<std::uint64_t>(server.puts));
    return trace;
}

static void a_seeded_storm() {
    int saved_a = 0, saved_b = 0, saved_c = 0;
    const std::uint64_t a = storm(20261120u, 400, saved_a);
    const std::uint64_t b = storm(20261120u, 400, saved_b);
    const std::uint64_t c = storm(7u, 400, saved_c);
    CHECK(a == b && saved_a == saved_b);  // the same seed is the same run
    CHECK(a != c);
    CHECK(saved_a > 0 && saved_c > 0);   // and the storm does reach the end of a change
}

// ---- a document that is cut or bent never takes the screen down -------------------------------------------------------------------------------

static void bent_documents() {
    const std::string whole = contract_json(small_contract({make("a"), make("b", SType::Integer)}), kContractSchema, "\"values\":{\"a\":true,\"b\":3}");
    Lcg rng{424242u};
    int accepted = 0;
    for (int i = 0; i < 300; ++i) {
        std::string t = whole;
        const std::uint32_t kind = rng.below(4);
        if (kind == 0) t.resize(rng.below(static_cast<std::uint32_t>(t.size())));                                              // cut short
        else if (kind == 1) t[rng.below(static_cast<std::uint32_t>(t.size()))] = static_cast<char>(rng.below(256));            // one byte changed
        else if (kind == 2) t.insert(rng.below(static_cast<std::uint32_t>(t.size())), 1, "{}[]\",:\\"[rng.below(8)]);           // one character added
        else t.erase(rng.below(static_cast<std::uint32_t>(t.size())), 1 + rng.below(5));                                       // a few removed
        const Loaded l = load_contract(true, CallError::None, answer(200, t));
        CHECK(!l.contract.settings.empty());  // there is always something to show: the live document or the copy
        if (l.live) ++accepted; else CHECK(!l.why.empty());
        for (const auto& s : l.contract.settings) CHECK(!s.key.empty());
    }
    CHECK(accepted > 0 && accepted < 300);
    // nesting deeper than the reader accepts, and a very long string
    std::string deep;
    for (int i = 0; i < 100; ++i) deep += "[";
    CHECK(!load_contract(true, CallError::None, answer(200, deep)).live);
    CHECK(!load_contract(true, CallError::None, answer(200, std::string(400000, 'x'))).live);
}

// ---- the light view of the values in force (GET /api/v1/settings/values) ---------------------------------------------------------------------------

static const char* kSampleHash = "0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef";

/// The answer of the light route as the server writes it for the examples of its documentation.
static std::string light_sample(const std::string& hash = kSampleHash, const std::string& extra = "") {
    return std::string("{\"schema\":\"eh3d.settings.values/1\",\"server_version\":\"0.2.0\",\"contract_hash\":\"") + hash + "\",\"count\":" + (extra.empty() ? "7" : "8") +
           ",\"settings\":{\"session_hours\":{\"v\":12,\"origin\":\"file\",\"editable\":false},"
           "\"slice_enabled\":{\"v\":true,\"origin\":\"override\",\"editable\":true},"
           "\"password_min_len\":{\"v\":12,\"origin\":\"override\",\"editable\":true},"
           "\"data_dir\":{\"hidden\":true,\"origin\":\"file\",\"editable\":false},"
           "\"listen\":{\"v\":\"127.0.0.1:18200\",\"origin\":\"file\",\"editable\":false},"
           "\"future_secret\":{\"v\":\"***\",\"secret\":true,\"editable\":false},"
           "\"allowed_hosts\":{\"v\":[\"a.example\",\"b.example\"],\"origin\":\"file\",\"editable\":false,\"truncated\":true}" + extra + "}}";
}

static void the_light_answer() {
    LightValues lv;
    CHECK(parse_light(light_sample(), lv) == LightParse::Ok);
    CHECK(lv.server_version == "0.2.0" && lv.contract_hash == kSampleHash && lv.entries.size() == 7);
    const LightEntry& hours = lv.entries.at("session_hours");
    CHECK(hours.has_value && hours.value == SValue::of_int(12) && !hours.overridden && !hours.editable && !hours.secret && !hours.hidden);
    const LightEntry& slice = lv.entries.at("slice_enabled");
    CHECK(slice.has_value && slice.value == SValue::of_bool(true) && slice.overridden && slice.editable);
    const LightEntry& dir = lv.entries.at("data_dir");
    CHECK(!dir.has_value && dir.hidden);
    const LightEntry& sec = lv.entries.at("future_secret");
    CHECK(!sec.has_value && sec.secret);
    const LightEntry& list = lv.entries.at("allowed_hosts");
    CHECK(list.has_value && list.value.kind == SValue::Kind::List && list.value.list.size() == 2 && list.truncated);
    CHECK(lv.entries.at("listen").value == SValue::of_text("127.0.0.1:18200"));
    const Loaded l = loaded_from_light(lv);
    CHECK(l.light && !l.live && l.has_values() && l.contract.settings.size() == 106 && l.values.size() == 5);
    CHECK(l.hidden.count("data_dir") == 1 && l.secret_keys.count("future_secret") == 1 && l.truncated.count("allowed_hosts") == 1 && l.overridden.count("slice_enabled") == 1);
    CHECK(l.values.count("data_dir") == 0 && l.values.count("future_secret") == 0);
    CHECK(l.hash == HashState::Differs && l.server_hash == kSampleHash && l.server_version == "0.2.0");
    // the answer of the server with the hash of the copy is the same document
    CHECK(compare_hash(kContractCopyHash) == HashState::Same && std::string(kContractCopyHash).size() == 64 && is_sha256_hex(kContractCopyHash));
    CHECK(compare_hash("") == HashState::Unknown && compare_hash("xyz") == HashState::Unknown && compare_hash(std::string(64, 'G')) == HashState::Unknown);
    CHECK(compare_hash(std::string(64, 'A')) == HashState::Unknown);  // the server writes lower case
    CHECK(compare_hash(kSampleHash) == HashState::Differs);
}

static void the_page_of_the_light_view() {
    LightValues lv;
    CHECK(parse_light(light_sample(kContractCopyHash), lv) == LightParse::Ok);
    const Loaded l = loaded_from_light(lv);
    Settings cfg;
    cfg.settings_write = true;
    cfg.settings_write_keys = "slice_enabled";
    const PageView p = build_page(l, cfg, Lang::En, "operator", false);
    CHECK(!p.live && p.source == "Values in force, read from the server (light view); texts from the copy kept in the screen (0.2.0)" && p.why.empty());
    CHECK(p.row_count == 107 && p.editable_count == 0);  // 106 of the copy and two keys it does not know; the light view never opens a write
    CHECK(p.warnings.size() == 1 && p.warnings[0] == "2 settings of the server are not in the copy of the screen; they are shown read only.");  // the same contract hash: no word about it
    const RowView* hours = row_of(p, "session_hours");
    CHECK(hours != nullptr && hours->value == "12 h" && !hours->editable && !hours->not_default);
    const RowView* slice = row_of(p, "slice_enabled");
    CHECK(slice != nullptr && slice->value == "On" && slice->not_default && slice->overridden && !slice->editable && slice->why == ReadOnlyWhy::Copy);
    CHECK(row_of(p, "data_dir")->value == "Not shown by the server" && row_of(p, "listen")->value == "127.0.0.1:18200");
    CHECK(row_of(p, "future_secret") != nullptr && row_of(p, "future_secret")->value == "Hidden" && row_of(p, "future_secret")->label == "future_secret");
    CHECK(row_of(p, "allowed_hosts")->value == "a.example, b.example..." && row_of(p, "allowed_hosts")->label == "allowed_hosts");
    CHECK(row_of(p, "rack_max_slots")->value == "-");  // the answer had no value for it
    // the same answer for an administrator whose full contract could not be read: still read only
    CHECK(build_page(l, cfg, Lang::En, "admin", false).editable_count == 0);
    // another contract hash: one line says the copy of the screen is out of date, in every language
    LightValues other;
    CHECK(parse_light(light_sample(), other) == LightParse::Ok);
    const Loaded od = loaded_from_light(other);
    for (std::size_t i = 0; i < kLangCount; ++i) {
        const PageView q = build_page(od, cfg, static_cast<Lang>(i), "", false);
        CHECK(!q.warnings.empty() && q.warnings[0] == settings_text(ST::HashDiffers, static_cast<Lang>(i)) && text_in_font(q.warnings[0].c_str()));
    }
    CHECK(build_page(od, cfg, Lang::En, "", false).warnings[0] == "The list of settings of the server is not the copy this screen carries: texts and keys may be out of date.");
    CHECK(build_page(od, cfg, Lang::Pt, "", false).warnings[0].find("definições") != std::string::npos);
    // the keys the copy does not know (a newer server) are counted and drawn read only, hidden and secret ones included
    LightValues newer;
    CHECK(parse_light(light_sample(kContractCopyHash, ",\"brand_new\":{\"v\":5,\"origin\":\"file\",\"editable\":false}"), newer) == LightParse::Ok);
    const PageView n = build_page(loaded_from_light(newer), cfg, Lang::En, "", false);
    CHECK(n.warnings.size() == 1 && n.warnings[0].find("3 settings of the server are not in the copy") == 0);  // future_secret, allowed_hosts and brand_new are unknown to the copy
    CHECK(row_of(n, "brand_new") != nullptr && row_of(n, "brand_new")->value == "5" && !row_of(n, "brand_new")->editable);
    // an unknown hash says nothing
    LightValues nohash;
    CHECK(parse_light(light_sample(""), nohash) == LightParse::Ok && nohash.contract_hash.empty() && loaded_from_light(nohash).hash == HashState::Unknown);
    // the full contract also tells its hash
    Contract c = small_contract({make("flag")});
    const Loaded same = load_contract(true, CallError::None, answer(200, contract_json(c, kContractSchema, std::string("\"server_version\":\"1\",\"contract_hash\":\"") + kContractCopyHash + "\",\"values\":{\"flag\":true}")));
    CHECK(same.live && same.hash == HashState::Same && same.server_hash == kContractCopyHash);
    const Loaded diff = load_contract(true, CallError::None, answer(200, contract_json(c, kContractSchema, std::string("\"contract_hash\":\"") + kSampleHash + "\",\"values\":{\"flag\":true}")));
    CHECK(diff.live && diff.hash == HashState::Differs && build_page(diff, cfg, Lang::En, "admin", false).warnings.size() >= 1);
    const Loaded bad = load_contract(true, CallError::None, answer(200, contract_json(c, kContractSchema, "\"contract_hash\":\"not-a-hash\",\"values\":{\"flag\":true}")));
    CHECK(bad.live && bad.hash == HashState::Unknown && bad.server_hash.empty());
    CHECK(load_contract(true, CallError::None, answer(200, contract_json(c, kContractSchema))).hash == HashState::Unknown);
}

/// A server that answers like EH3D-SERVER does for the contract and for the light view, and counts what it was asked.
struct LightServer {
    std::string role;                 // who is logged in (the contract is for an administrator)
    bool admin_scope = true;          // an administrator token with admin:policy
    bool down = false;                // no answer at all
    bool broke = false;               // an answer that does not fit
    bool give_etag = true;
    std::map<std::string, std::string> values;  // key -> JSON of its value
    int version = 1;                  // a change of any value makes a new tag
    int full_calls = 0, light_calls = 0, not_modified = 0, light_with_tag = 0;
    std::string last_tag;

    std::string body() const {
        std::string o = std::string("{\"schema\":\"eh3d.settings.values/1\",\"server_version\":\"0.2.0\",\"contract_hash\":\"") + kContractCopyHash + "\",\"count\":" + std::to_string(values.size()) + ",\"settings\":{";
        bool first = true;
        for (const auto& kv : values) {
            o += std::string(first ? "" : ",") + json_quote(kv.first) + ":" + kv.second;
            first = false;
        }
        return o + "}}";
    }
    std::string etag() const {
        std::uint32_t h = 2166136261u;
        for (char c : body()) h = (h ^ static_cast<unsigned char>(c)) * 16777619u;
        char buf[24];
        std::snprintf(buf, sizeof buf, "\"%08x\"", static_cast<unsigned>(h));
        return buf;
    }
    bool operator()(const Request& r, Response& out, CallError& error, Conditional& cond) {
        const std::string& inm = cond.if_none_match;
        error = CallError::None;
        out = Response();
        if (down) { error = CallError::NoAnswer; return false; }
        if (broke) { error = CallError::TooLong; return false; }
        if (r.path == kContractPath) {
            ++full_calls;
            if (role != "admin" || !admin_scope) { out.status = 403; out.body = "{\"error\":\"forbidden_scope\",\"detail\":\"needs admin:policy\"}"; return true; }
            out.status = 200;
            out.body = contract_json(embedded_contract(), kContractSchema, std::string("\"server_version\":\"0.2.0\",\"contract_hash\":\"") + kContractCopyHash + "\",\"values\":{\"max_body_mb\":8}");
            return true;
        }
        if (r.path == kLightPath) {
            ++light_calls;
            last_tag = inm;
            if (!inm.empty()) ++light_with_tag;
            if (role.empty()) { out.status = 401; out.body = "{\"error\":\"unauthorized\"}"; return true; }
            if (!inm.empty() && inm == etag()) { ++not_modified; out.status = 304; return true; }
            out.status = 200;
            out.body = body();
            if (give_etag) cond.etag = etag();
            return true;
        }
        out.status = 404;
        out.body = "{\"error\":\"not_found\"}";
        return true;
    }
};

static LightServer fresh_server(const std::string& role) {
    LightServer s;
    s.role = role;
    s.values = {{"session_hours", "{\"v\":12,\"origin\":\"file\",\"editable\":false}"},
                {"slice_enabled", "{\"v\":false,\"origin\":\"file\",\"editable\":true}"},
                {"data_dir", "{\"hidden\":true,\"origin\":\"file\",\"editable\":false}"},
                {"password_min_len", "{\"v\":12,\"origin\":\"file\",\"editable\":true}"}};
    return s;
}

static void etag_and_not_modified() {
    LightServer srv = fresh_server("operator");
    Fetch fetch = [&srv](const Request& r, Response& out, CallError& e, Conditional& c) { return srv(r, out, e, c); };
    LightCache cache;
    Settings cfg;
    ReadResult first = read_settings(fetch, "operator", cfg, cache);
    CHECK(first.changed && first.loaded.light && first.loaded.why.empty() && first.loaded.values.at("session_hours") == SValue::of_int(12));
    CHECK(srv.full_calls == 0 && srv.light_calls == 1 && srv.light_with_tag == 0 && cache.has() && cache.etag() == srv.etag());
    // nothing changed: the screen sends the tag and the server answers 304; the page keeps what it shows
    ReadResult again = read_settings(fetch, "operator", cfg, cache);
    CHECK(!again.changed && srv.not_modified == 1 && srv.last_tag == srv.etag() && again.loaded.light && again.loaded.values.at("session_hours") == SValue::of_int(12));
    ReadResult third = read_settings(fetch, "operator", cfg, cache);
    CHECK(!third.changed && srv.not_modified == 2 && srv.light_calls == 3 && srv.full_calls == 0);
    // a value changed: a new tag and a new answer
    srv.values["session_hours"] = "{\"v\":24,\"origin\":\"override\",\"editable\":false}";
    ReadResult changed = read_settings(fetch, "operator", cfg, cache);
    CHECK(changed.changed && changed.loaded.values.at("session_hours") == SValue::of_int(24) && changed.loaded.overridden.count("session_hours") == 1 && cache.etag() == srv.etag());
    // a server that gives no tag: every round reads the whole answer and nothing is asked by tag
    LightServer plain = fresh_server("viewer");
    plain.give_etag = false;
    Fetch plain_fetch = [&plain](const Request& r, Response& out, CallError& e, Conditional& c) { return plain(r, out, e, c); };
    LightCache plain_cache;
    CHECK(read_settings(plain_fetch, "viewer", cfg, plain_cache).changed && !plain_cache.has());
    CHECK(read_settings(plain_fetch, "viewer", cfg, plain_cache).changed && plain.light_with_tag == 0 && plain.not_modified == 0);
    // an unreachable server drops the held answer; the next request is whole again
    srv.down = true;
    ReadResult gone = read_settings(fetch, "operator", cfg, cache);
    CHECK(gone.changed && !gone.loaded.light && gone.loaded.why == "unreachable" && gone.loaded.contract.settings.size() == 106 && !cache.has());
    srv.down = false;
    const int before = srv.light_with_tag;
    CHECK(read_settings(fetch, "operator", cfg, cache).loaded.light && srv.light_with_tag == before);
    // the login changed: the held answer is forgotten
    cache.clear();
    CHECK(!cache.has() && cache.if_none_match().empty());
    // the request on the wire
    const Request lr = cache.request();
    CHECK(lr.method == "GET" && lr.path == "/api/v1/settings/values" && lr.body.empty());
    const std::string with_tag = wire(lr, "10.0.0.5", "tok", "eh3d-node-hmi-gui/0.1.1", "\"abc123\"");
    CHECK(has(with_tag, "GET /api/v1/settings/values HTTP/1.1\r\n") && has(with_tag, "If-None-Match: \"abc123\"\r\n") && has(with_tag, "Authorization: Bearer tok\r\n"));
    CHECK(!has(wire(lr, "h", "t"), "If-None-Match"));
    CHECK(!has(wire(lr, "h", "t", "", "abc"), "If-None-Match") && !has(wire(lr, "h", "t", "", "\"a\r\nX: y\""), "If-None-Match"));  // not a tag: never sent
    CHECK(!has(wire(Request{"PUT", kPolicyPath, "{}"}, "h", "t", "", "\"abc\""), "If-None-Match"));                                     // only a read is conditional
    CHECK(request_carries_token(lr));
    CHECK(answer_limit(lr) == 16384u && answer_limit({"GET", "/api/v1/settings/values?group=login", ""}) == 16384u && answer_limit(contract_request()) == 1024u * 1024u);
}

static void the_etag_header() {
    Response r;
    std::string tag;
    CHECK(parse_response("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nETag: \"abc123\"\r\nContent-Length: 2\r\n\r\n{}", r, &tag) && r.status == 200 && tag == "\"abc123\"");
    CHECK(parse_response("HTTP/1.1 200 OK\r\netag:   \"x9\"  \r\nContent-Length: 2\r\n\r\n{}", r, &tag) && tag == "\"x9\"");              // any case, spaces around
    CHECK(parse_response("HTTP/1.1 200 OK\r\nETag: W/\"weak1\"\r\nContent-Length: 2\r\n\r\n{}", r, &tag) && tag == "W/\"weak1\"");
    CHECK(parse_response("HTTP/1.1 304 Not Modified\r\nETag: \"abc123\"\r\nContent-Length: 0\r\n\r\n", r, &tag) && r.status == 304 && r.body.empty() && tag == "\"abc123\"");
    CHECK(parse_response("HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\n{}", r, &tag) && tag.empty());                                       // none: cleared from the last answer
    CHECK(parse_response("HTTP/1.1 200 OK\r\nETag: abc\r\nContent-Length: 2\r\n\r\n{}", r, &tag) && tag.empty());                         // not quoted
    CHECK(parse_response("HTTP/1.1 200 OK\r\nETag: \"a\"b\"\r\nContent-Length: 2\r\n\r\n{}", r, &tag) && tag.empty());                    // a quote inside
    CHECK(parse_response("HTTP/1.1 200 OK\r\nETag: \"\"\r\nContent-Length: 2\r\n\r\n{}", r, &tag) && tag.empty());                       // empty tag
    CHECK(parse_response("HTTP/1.1 200 OK\r\nETag: \"" + std::string(200, 'a') + "\"\r\nContent-Length: 2\r\n\r\n{}", r, &tag) && tag.empty());  // too long
    CHECK(parse_response("HTTP/1.1 200 OK\r\nETag: \"a b\"\r\nContent-Length: 2\r\n\r\n{}", r, &tag) && tag.empty());                    // a space inside
    CHECK(etag_ok("\"a\"") && !etag_ok("") && !etag_ok("\"") && !etag_ok("W/") && !etag_ok("W/\"") && etag_ok("W/\"a\""));
}

static void who_reads_what() {
    Settings cfg;
    // an administrator reads the full contract and never asks for the light view
    LightServer admin = fresh_server("admin");
    Fetch fa = [&admin](const Request& r, Response& out, CallError& e, Conditional& c) { return admin(r, out, e, c); };
    LightCache ca;
    ReadResult ra = read_settings(fa, "admin", cfg, ca);
    CHECK(ra.loaded.live && !ra.loaded.light && ra.loaded.hash == HashState::Same && admin.full_calls == 1 && admin.light_calls == 0 && ra.loaded.values.at("max_body_mb") == SValue::of_int(8));
    // an administrator token without the scope of the policy: the light view stands in, with no complaint
    LightServer scoped = fresh_server("admin");
    scoped.admin_scope = false;
    Fetch fs = [&scoped](const Request& r, Response& out, CallError& e, Conditional& c) { return scoped(r, out, e, c); };
    LightCache cs;
    ReadResult rs = read_settings(fs, "admin", cfg, cs);
    CHECK(!rs.loaded.live && rs.loaded.light && rs.loaded.why.empty() && scoped.full_calls == 1 && scoped.light_calls == 1);
    // an operator, a viewer and a login whose role is not known read the light view only
    for (const char* role : {"operator", "viewer", ""}) {
        LightServer s = fresh_server(*role ? role : "viewer");
        Fetch f = [&s](const Request& r, Response& out, CallError& e, Conditional& c) { return s(r, out, e, c); };
        LightCache c;
        const ReadResult rr = read_settings(f, role, cfg, c);
        CHECK(rr.loaded.light && s.full_calls == 0 && s.light_calls == 1 && rr.loaded.has_values());
    }
    // settings_values = off: the old arrangement, the full contract is tried and the copy is what remains
    Settings off;
    off.settings_values = false;
    LightServer o = fresh_server("operator");
    Fetch fo = [&o](const Request& r, Response& out, CallError& e, Conditional& c) { return o(r, out, e, c); };
    LightCache co;
    const ReadResult ro = read_settings(fo, "operator", off, co);
    CHECK(!ro.loaded.live && !ro.loaded.light && ro.loaded.why == "forbidden_scope" && o.full_calls == 1 && o.light_calls == 0 && !ro.loaded.has_values());
    CHECK(build_page(ro.loaded, off, Lang::En, "operator", false).why == "The server gives its settings to an administrator only.");
    // no login at all: the server refuses and the copy is shown with the reason
    LightServer none = fresh_server("");
    Fetch fn = [&none](const Request& r, Response& out, CallError& e, Conditional& c) { return none(r, out, e, c); };
    LightCache cn;
    const ReadResult rn = read_settings(fn, "", cfg, cn);
    CHECK(!rn.loaded.light && rn.loaded.why == "unauthorized" && rn.loaded.contract.settings.size() == 106 && !cn.has());
    // an older server without the route: the copy and the code it gave
    Fetch old = [](const Request&, Response& out, CallError& e, Conditional&) { e = CallError::None; out = Response(); out.status = 404; out.body = "{\"error\":\"not_found\"}"; return true; };
    LightCache cold;
    const ReadResult rold = read_settings(old, "operator", cfg, cold);
    CHECK(!rold.loaded.light && rold.loaded.why == "not_found" && !rold.loaded.has_values());
    // the poll time follows the source
    cfg.settings_poll_s = 600;
    cfg.settings_values_poll_s = 20;
    CHECK(read_interval_s(rs.loaded, cfg) == 20 && read_interval_s(ra.loaded, cfg) == 600 && read_interval_s(rold.loaded, cfg) == 600);
    // a poller that gives up quickly after a failure and waits the poll time of the light view after a good answer
    ContractPoller p;
    p.done(true, read_interval_s(rs.loaded, cfg), 0);
    CHECK(!p.due(true, 19999) && p.due(true, 20000));
}

static void the_light_view_that_goes_wrong() {
    Settings cfg;
    // the answer is longer than what the screen keeps, or the sockets say so
    LightServer s = fresh_server("viewer");
    s.broke = true;
    Fetch f = [&s](const Request& r, Response& out, CallError& e, Conditional& c) { return s(r, out, e, c); };
    LightCache c;
    CHECK(read_settings(f, "viewer", cfg, c).loaded.why == "toolong");
    Loaded l;
    Response big = answer(200, light_sample() + std::string(kLightMaxBytes, ' '));
    CHECK(apply_light(true, CallError::None, big, "\"t\"", c, l) == LightResult::Failed && l.why == "toolong" && !c.has());
    // a refusal with a code of the server, an error with no code, a 304 for a tag nobody sent
    CHECK(apply_light(true, CallError::None, answer(403, "{\"error\":\"forbidden\"}"), "", c, l) == LightResult::Failed && l.why == "forbidden");
    CHECK(apply_light(true, CallError::None, answer(500, "<html>oops</html>"), "", c, l) == LightResult::Failed && l.why == "http_500");
    CHECK(apply_light(true, CallError::None, answer(500, "{\"error\":\"Has Spaces\"}"), "", c, l) == LightResult::Failed && l.why == "http_500");
    CHECK(apply_light(true, CallError::None, answer(304, ""), "", c, l) == LightResult::Failed && l.why == "shape");
    // a newer or another schema, text that is not JSON, JSON that is not an object
    CHECK(apply_light(true, CallError::None, answer(200, "{\"schema\":\"eh3d.settings.values/2\",\"settings\":{}}"), "", c, l) == LightResult::Failed && l.why == "newer");
    CHECK(apply_light(true, CallError::None, answer(200, "{\"schema\":\"eh3d.settings.contract/1\",\"settings\":{}}"), "", c, l) == LightResult::Failed && l.why == "shape");
    CHECK(apply_light(true, CallError::None, answer(200, "not json"), "", c, l) == LightResult::Failed && l.why == "shape");
    CHECK(apply_light(true, CallError::None, answer(200, "[1,2]"), "", c, l) == LightResult::Failed && l.why == "shape");
    CHECK(apply_light(true, CallError::None, answer(200, ""), "", c, l) == LightResult::Failed && l.why == "shape");
    for (const char* why : {"toolong", "shape", "newer", "forbidden", "http_500", "unreachable"}) CHECK(!why_copy_text(why, Lang::En).empty());
    // hostile documents: each is refused whole and none is kept
    const std::string hash = std::string(",\"contract_hash\":\"") + kSampleHash + "\"";
    const std::string head = "{\"schema\":\"eh3d.settings.values/1\"" + hash;
    LightValues lv;
    CHECK(parse_light(head + ",\"settings\":[]}", lv) == LightParse::Malformed);                                       // settings is not an object
    CHECK(parse_light(head + "}", lv) == LightParse::Malformed);                                                         // no settings
    CHECK(parse_light(head + ",\"count\":2,\"settings\":{\"a\":{\"v\":1}}}", lv) == LightParse::Malformed);              // count is not the number of keys
    CHECK(parse_light(head + ",\"count\":1.5,\"settings\":{\"a\":{\"v\":1}}}", lv) == LightParse::Malformed);
    CHECK(parse_light(head + ",\"count\":\"1\",\"settings\":{\"a\":{\"v\":1}}}", lv) == LightParse::Malformed);
    CHECK(parse_light(head + ",\"settings\":{\"A\":{\"v\":1}}}", lv) == LightParse::Malformed);                          // a key that is not a name
    CHECK(parse_light(head + ",\"settings\":{\"a b\":{\"v\":1}}}", lv) == LightParse::Malformed);
    CHECK(parse_light(head + ",\"settings\":{\"\":{\"v\":1}}}", lv) == LightParse::Malformed);
    CHECK(parse_light(head + ",\"settings\":{\"" + std::string(49, 'a') + "\":{\"v\":1}}}", lv) == LightParse::Malformed);
    CHECK(parse_light(head + ",\"settings\":{\"a\":5}}", lv) == LightParse::Malformed);                                  // an entry that is not an object
    CHECK(parse_light(head + ",\"settings\":{\"a\":{\"v\":{\"x\":1}}}}", lv) == LightParse::Malformed);                  // a value that is an object
    CHECK(parse_light(head + ",\"settings\":{\"a\":{\"v\":[{\"x\":1}]}}}", lv) == LightParse::Malformed);
    CHECK(parse_light(head + ",\"settings\":{\"a\":{\"v\":\"" + std::string(kLightMaxText + 1, 'x') + "\"}}}", lv) == LightParse::Malformed);
    std::string many = "[";
    for (std::size_t i = 0; i <= kLightMaxItems; ++i) many += (i ? "," : "") + std::string("\"x\"");
    CHECK(parse_light(head + ",\"settings\":{\"a\":{\"v\":" + many + "]}}}", lv) == LightParse::Malformed);
    std::string keys = "";
    for (std::size_t i = 0; i <= kLightMaxKeys; ++i) keys += (i ? "," : "") + std::string("\"k") + std::to_string(i) + "\":{\"v\":1}";
    CHECK(parse_light(head + ",\"settings\":{" + keys + "}}", lv) == LightParse::Malformed);                             // more keys than the screen trusts
    std::string deep;
    for (int i = 0; i < 100; ++i) deep += "{\"a\":";
    CHECK(parse_light(head + ",\"settings\":" + deep + "1" + std::string(100, '}') + "}", lv) == LightParse::NotJson);
    // what is odd but harmless is kept: no value, a null, a hash that is not one, a long version
    CHECK(parse_light(head + ",\"settings\":{\"a\":{\"origin\":\"file\"},\"b\":{\"v\":null},\"c\":{\"v\":1.5}}}", lv) == LightParse::Ok && !lv.entries.at("a").has_value && !lv.entries.at("b").has_value && lv.entries.at("c").value.kind == SValue::Kind::Text);
    CHECK(parse_light("{\"schema\":\"eh3d.settings.values/1\",\"contract_hash\":\"nope\",\"server_version\":\"" + std::string(40, '9') + "\",\"settings\":{}}", lv) == LightParse::Ok && lv.contract_hash.empty() && lv.server_version.empty());
    // a secret never shows: whatever the server sent in the place of its value, and the mark with no flag
    CHECK(parse_light(head + ",\"settings\":{\"pw\":{\"v\":\"hunter2\",\"secret\":true},\"tok\":{\"v\":\"***\"},\"p\":{\"v\":\"/home/me\",\"hidden\":true}}}", lv) == LightParse::Ok);
    CHECK(!lv.entries.at("pw").has_value && lv.entries.at("pw").secret && !lv.entries.at("tok").has_value && lv.entries.at("tok").secret && !lv.entries.at("p").has_value && lv.entries.at("p").hidden);
    const Loaded sec = loaded_from_light(lv);
    CHECK(sec.values.empty() && sec.secret_keys.size() == 2 && sec.hidden.size() == 1);
    Settings plain;
    const PageView pv = build_page(sec, plain, Lang::En, "viewer", false);
    for (const GroupView& g : pv.groups)
        for (const RowView& r : g.rows) CHECK(!has(r.value, "hunter2") && !has(r.value, "/home/me") && !has(r.note, "hunter2") && !has(r.label, "hunter2"));
    CHECK(row_of(pv, "pw")->value == "Hidden" && row_of(pv, "tok")->value == "Hidden" && row_of(pv, "p")->value == "Not shown by the server");
    // a key the copy marks secret is hidden even when the server sends its value
    Setting known_secret = make("copysecret", SType::String);
    known_secret.secret = true;
    Loaded with_copy = loaded_from_light(lv);
    with_copy.contract = small_contract({known_secret});
    with_copy.values["copysecret"] = SValue::of_text("leaked");
    CHECK(row_of(build_page(with_copy, plain, Lang::En, "viewer", false), "copysecret")->value == "Hidden");
}

static void the_light_view_settings_of_the_file() {
    Settings d;
    CHECK(d.settings_values && d.settings_values_poll_s == 30);
    std::vector<Problem> problems;
    Settings s = parse_settings("settings_values = off\nsettings_values_poll_s = 5\n", problems);
    CHECK(problems.empty() && !s.settings_values && s.settings_values_poll_s == 5);
    s = parse_settings("settings_values = on\nsettings_values_poll_s = 3600\n", problems);
    CHECK(problems.empty() && s.settings_values && s.settings_values_poll_s == 3600);
    for (const char* bad : {"settings_values = maybe", "settings_values = 2", "settings_values_poll_s = 4", "settings_values_poll_s = 3601", "settings_values_poll_s = x", "settings_values_poll_s = -1"}) {
        problems.clear();
        const Settings b = parse_settings(std::string(bad) + "\n", problems);
        CHECK(problems.size() == 1 && b.settings_values && b.settings_values_poll_s == 30);  // a line that cannot be used changes nothing and is listed
    }
}

/// A seeded storm: every round the login, the server and the answer change; whatever happens the screen shows something, never a secret, and the same seed gives the same run.
static std::uint64_t light_storm(std::uint32_t seed, int rounds, int& fresh_count) {
    Lcg rng{seed};
    std::uint64_t trace = 1469598103934665603ull;
    auto mix = [&trace](std::uint64_t v) { trace = (trace ^ v) * 1099511628211ull; };
    LightServer srv = fresh_server("operator");
    srv.values["pw"] = "{\"v\":\"canary-secret-value\",\"secret\":true,\"editable\":false}";
    LightCache cache;
    Settings cfg;
    Fetch fetch = [&srv](const Request& r, Response& out, CallError& e, Conditional& c) { return srv(r, out, e, c); };
    const char* roles[] = {"admin", "operator", "viewer", ""};
    for (int i = 0; i < rounds; ++i) {
        const std::uint32_t what = rng.below(10);
        if (what == 0) srv.down = !srv.down;
        else if (what == 1) srv.broke = !srv.broke;
        else if (what == 2) srv.give_etag = !srv.give_etag;
        else if (what == 3) srv.admin_scope = !srv.admin_scope;
        else if (what == 4) srv.values["session_hours"] = "{\"v\":" + std::to_string(1 + rng.below(700)) + ",\"origin\":\"file\",\"editable\":false}";
        else if (what == 5) cache.clear();
        const std::string role = roles[rng.below(4)];
        srv.role = role;
        cfg.settings_values = rng.below(8) != 0;
        const ReadResult rr = read_settings(fetch, role, cfg, cache);
        CHECK(!rr.loaded.contract.settings.empty());
        CHECK(rr.loaded.live || rr.loaded.light || !rr.loaded.why.empty());
        const PageView pv = build_page(rr.loaded, cfg, Lang::En, role, false);
        CHECK(pv.row_count >= 106 && pv.editable_count == 0);
        for (const GroupView& g : pv.groups)
            for (const RowView& r : g.rows) {
                CHECK(!has(r.value, "canary-secret-value") && !has(r.note, "canary-secret-value"));
                mix(std::hash<std::string>{}(r.key + r.value));
            }
        CHECK(!(rr.loaded.light && rr.loaded.values.count("pw") != 0));  // the value of a secret is never kept
        if (rr.loaded.light && rr.changed) ++fresh_count;
        mix(static_cast<std::uint64_t>(rr.changed) + (rr.loaded.light ? 2u : 0u) + (rr.loaded.live ? 4u : 0u) + rr.loaded.why.size() * 8u);
    }
    mix(static_cast<std::uint64_t>(srv.not_modified));
    return trace;
}

static void a_seeded_light_storm() {
    int a_fresh = 0, b_fresh = 0, c_fresh = 0;
    const std::uint64_t a = light_storm(20261121u, 400, a_fresh);
    const std::uint64_t b = light_storm(20261121u, 400, b_fresh);
    const std::uint64_t c = light_storm(11u, 400, c_fresh);
    CHECK(a == b && a_fresh == b_fresh);
    CHECK(a != c);
    CHECK(a_fresh > 0 && c_fresh > 0);
}

static void bent_light_documents() {
    const std::string whole = light_sample(kContractCopyHash);
    Lcg rng{777u};
    int accepted = 0;
    for (int i = 0; i < 400; ++i) {
        std::string t = whole;
        const std::uint32_t kind = rng.below(4);
        if (kind == 0) t.resize(rng.below(static_cast<std::uint32_t>(t.size())));
        else if (kind == 1) t[rng.below(static_cast<std::uint32_t>(t.size()))] = static_cast<char>(rng.below(256));
        else if (kind == 2) t.insert(rng.below(static_cast<std::uint32_t>(t.size())), 1, "{}[]\",:\\"[rng.below(8)]);
        else t.erase(rng.below(static_cast<std::uint32_t>(t.size())), 1 + rng.below(5));
        LightCache cache;
        Loaded l;
        const LightResult r = apply_light(true, CallError::None, answer(200, t), "", cache, l);
        CHECK(l.contract.settings.size() == 106);  // there is always something to show: the answer or the copy
        if (r == LightResult::Fresh) { ++accepted; CHECK(l.light && l.values.count("future_secret") == 0); }
        else CHECK(!l.why.empty());
    }
    CHECK(accepted > 0 && accepted < 400);
}

int main() {
    the_copy();
    the_round_trip();
    documents_it_does_not_understand();
    loading_the_live_contract();
    groups_and_unlisted_keys();
    drift_between_the_copy_and_the_server();
    controls_by_type();
    typed_input();
    the_write_request();
    the_answer_of_a_write();
    the_write_policy();
    the_steps_of_a_change();
    the_page_of_the_copy();
    the_page_of_the_live_contract();
    what_the_server_says_against_hmi_cfg();
    when_the_contract_is_read();
    the_login_knows_its_role();
    the_texts();
    a_seeded_storm();
    bent_documents();
    the_light_answer();
    the_page_of_the_light_view();
    etag_and_not_modified();
    the_etag_header();
    who_reads_what();
    the_light_view_that_goes_wrong();
    the_light_view_settings_of_the_file();
    a_seeded_light_storm();
    bent_light_documents();
    std::printf("settings: %d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
