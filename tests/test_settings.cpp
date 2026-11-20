// EH3D-NODE-HMI-GUI - host tests of the settings of the server drawn from its contract: the copy the screen carries (89 settings in 15 groups), the live document and what the screen does
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
    CHECK(c.settings.size() == 89 && kContractCopySettingCount == 89);
    CHECK(c.groups.size() == 15 && kContractCopyGroupCount == 15);
    std::set<std::string> keys, groups;
    for (const ContractGroup& g : c.groups) {
        groups.insert(g.id);
        for (const std::string& t : g.label) CHECK(!t.empty());
    }
    CHECK(groups.size() == 15);
    int policy = 0, file = 0;
    for (const Setting& s : c.settings) {
        CHECK(keys.insert(s.key).second);  // each key once
        CHECK(groups.count(s.group) == 1);
        CHECK(s.type != SType::Unknown);
        CHECK(control_kind(s) != ControlKind::Generic);  // every setting of the copy has a control the screen knows
        CHECK(!s.secret);
        for (std::size_t i = 0; i < kBaseLangCount; ++i) CHECK(!s.label[i].empty() && !s.help[i].empty() && !s.warning[i].empty());
        CHECK(text_in_font(s.label[0].c_str()));  // the English text can always be drawn
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
    CHECK(policy == 19 && file == 70);
    // the keys the screen reasons about are there with the type it expects
    const Setting* body = c.find("max_body_mb");
    CHECK(body != nullptr && body->type == SType::Integer && body->min == 1 && body->max == 256 && body->unit == "megabytes");
    const Setting* slots = c.find("rack_max_slots");
    CHECK(slots != nullptr && slots->max == 14 && slots->def.n == 14);
    const Setting* looser = c.find("network_open");
    CHECK(looser != nullptr && looser->apply == Apply::Restart && looser->risk == Risk::Exposure && !looser->def.b);
    CHECK(c.find("no_such_key") == nullptr);
    // the groups come in the order of the contract and hold every setting
    const std::vector<SettingsGroup> grouped = group_settings(c);
    CHECK(grouped.size() == 15);
    std::size_t total = 0;
    for (const SettingsGroup& g : grouped) total += g.settings.size();
    CHECK(total == 89);
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
    CHECK(l.contract.settings.size() == 89);
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
    CHECK(!l.live && l.why == "unreachable" && l.contract.settings.size() == 89 && l.values.empty());
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
    CHECK(!l.live && l.why == "newer" && l.contract.settings.size() == 89);
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
    // with the whole copy: of the 89 settings the screen can change none by default, and with every switch open exactly the 19 keys of the security page
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
    CHECK(p.groups.size() == 15 && p.row_count == 89 && p.editable_count == 0 && p.warnings.empty());
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
        CHECK(q.row_count == 89);
        for (const GroupView& g : q.groups) {
            CHECK(!g.label.empty());
            for (const RowView& r : g.rows) CHECK(!r.label.empty() && text_in_font(r.label.c_str()) && text_in_font(r.note.c_str()));
        }
    }
    CHECK(row_of(build_page(copy, cfg, Lang::Es, "", false), "listen")->label == "Dirección de escucha");
    CHECK(row_of(build_page(copy, cfg, Lang::PtBr, "", false), "listen")->label == "Endereço de escuta");  // a variant reads its main language
    CHECK(row_of(build_page(copy, cfg, Lang::Pl, "", false), "listen")->label == "Listen address");         // the other twenty-nine read English
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
    CHECK(not_drawable >= 0);
    // the formal register of the three languages that have one
    for (unsigned t = 0; t < static_cast<unsigned>(ST::Count); ++t) {
        const Row& row = settings_text_row(static_cast<ST>(t));
        CHECK(!has(row[3], " du ") && !has(row[3], "Dir ") && !has(row[4], " tu ") && !has(row[4], " toi ") && !has(row[5], " tu ") && !has(row[5], " teu "));
    }
    CHECK(fill("Change {label} to {value}?", {{"label", "A"}, {"value", "B"}}) == "Change A to B?");
    CHECK(fill("{n} and {n}", {{"n", "{n}"}}) == "{n} and {n}");  // a value that holds a hole is not filled again
    CHECK(fill("no holes", {{"x", "y"}}) == "no holes" && fill("{x}", {}) == "{x}");
    CHECK(settings_text(ST::Title, Lang::Ja) == "サーバー設定" || settings_text(ST::Title, Lang::Ja) == "Server settings");
    CHECK(settings_text(ST::Title, Lang::Pl) == "Server settings" && settings_text(ST::Title, Lang::FrCa) == "Réglages du serveur" && settings_text(ST::Title, Lang::PtBr) == "Definições do servidor");
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
    std::printf("settings: %d checks, %d failed\n", g_checks, g_failed);
    return g_failed == 0 ? 0 : 1;
}
