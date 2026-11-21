// EH3D-NODE-HMI-GUI - core/settings_values.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The light view of the settings of the server for a login that is not an administrator (`GET /api/v1/settings/values`, docs/CLIENT_SYNC.md of EH3D-SERVER): read only, a few kilobytes and
// never more than 16 KiB, for any login (viewer, operator, administrator; the screen of a machine logs in as the application `hmi`). For each key it gives the value in force (`v`), where it
// comes from (`origin`: `file` or `override`) and whether the security page changes it (`editable`); a key marked `secret` has `v` = `***` and no value is ever kept, a key whose format is a
// path of the computer has `hidden` and no value, and a long text or list is cut (`truncated`). It has no text: the screen keeps the labels of its own copy of the contract and compares the
// `contract_hash` of the answer with the hash of that copy (`kContractCopyHash`), so it can say when the copy is out of date. The answer carries an `ETag`; the screen sends it back in
// `If-None-Match` and a `304` costs a few bytes. Nothing here writes: the light view opens no write. The sockets are the firmware's (main/server_link.cpp); everything else is tested on a computer.
#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <string>

#include "hmi_config.hpp"
#include "server_errors.hpp"
#include "server_link.hpp"
#include "settings_contract.hpp"

namespace hmi {

/// The version of the light answer that the screen understands (`eh3d.settings.values/2` would be a newer one: the screen says so and shows its copy).
inline constexpr const char* kLightSchema = "eh3d.settings.values/1";
inline constexpr const char* kLightSchemaFamily = "eh3d.settings.values/";
/// What stands in the place of the value of a secret key.
inline constexpr const char* kSecretMark = "***";
/// The most keys one answer may have (the contract has 89), the longest text of a value and the most items of a list that the screen accepts: more is a document it does not trust.
inline constexpr std::size_t kLightMaxKeys = 256;
inline constexpr std::size_t kLightMaxText = 1024;
inline constexpr std::size_t kLightMaxItems = 64;

/// One key of the light answer.
struct LightEntry {
    bool has_value = false;
    SValue value;
    bool secret = false;
    bool hidden = false;
    bool truncated = false;
    bool overridden = false;  // `origin` = `override`
    bool editable = false;    // the security page changes it (only an administrator may)
};

struct LightValues {
    std::string server_version;
    std::string contract_hash;  // empty when the server gave none or one that is not a SHA-256
    std::map<std::string, LightEntry> entries;
};

enum class LightParse { Ok, NotJson, OtherSchema, NewerSchema, Malformed };

namespace detail {
inline bool light_key_ok(const std::string& k) {
    if (k.empty() || k.size() > 48) return false;
    for (char c : k)
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) return false;
    return true;
}
inline bool flag_of(const Json& o, const char* name) {
    const Json* f = o.get(name);
    return f != nullptr && f->type == Json::Type::Bool && f->b;
}
}  // namespace detail

/// Reads the answer of `GET /api/v1/settings/values`. Nothing half-understood is kept: a key that is not a name of lower-case letters, digits and underscores, an entry that is not an object, a
/// value that is an object, a text or list longer than the screen accepts, more than `kLightMaxKeys` keys or a `count` that is not the number of keys make the whole answer malformed.
inline LightParse parse_light(const std::string& text, LightValues& out) {
    Json root;
    if (!parse_json(text, root) || root.type != Json::Type::Object) return LightParse::NotJson;
    const std::string schema = root.str("schema");
    if (schema != kLightSchema) return schema.compare(0, std::char_traits<char>::length(kLightSchemaFamily), kLightSchemaFamily) == 0 ? LightParse::NewerSchema : LightParse::OtherSchema;
    const Json* settings = root.get("settings");
    if (settings == nullptr || settings->type != Json::Type::Object || settings->o.size() > kLightMaxKeys) return LightParse::Malformed;
    const Json* count = root.get("count");
    if (count != nullptr) {
        long long n = 0;
        if (!detail::whole_of(count, n) || n != static_cast<long long>(settings->o.size())) return LightParse::Malformed;
    }
    LightValues lv;
    const std::string version = root.str("server_version");
    if (version.size() <= 32) lv.server_version = version;
    const std::string hash = root.str("contract_hash");
    if (is_sha256_hex(hash)) lv.contract_hash = hash;
    for (const auto& kv : settings->o) {
        const Json& o = kv.second;
        if (!detail::light_key_ok(kv.first) || o.type != Json::Type::Object) return LightParse::Malformed;
        LightEntry e;
        e.secret = detail::flag_of(o, "secret");
        e.hidden = detail::flag_of(o, "hidden");
        e.truncated = detail::flag_of(o, "truncated");
        e.editable = detail::flag_of(o, "editable");
        e.overridden = o.str("origin") == "override";
        const Json* v = o.get("v");
        if (v != nullptr && v->type == Json::Type::String && v->s == kSecretMark) e.secret = true;  // the mark stands for a value nobody may see
        if (v != nullptr && !e.secret && !e.hidden) {
            switch (v->type) {
                case Json::Type::Null: break;
                case Json::Type::Object: return LightParse::Malformed;
                case Json::Type::String:
                    if (v->s.size() > kLightMaxText) return LightParse::Malformed;
                    e.has_value = true;
                    e.value = SValue::of_text(v->s);
                    break;
                case Json::Type::Array:
                    if (v->a.size() > kLightMaxItems) return LightParse::Malformed;
                    for (const Json& item : v->a)
                        if (item.type == Json::Type::Object || item.type == Json::Type::Array || (item.type == Json::Type::String && item.s.size() > kLightMaxText)) return LightParse::Malformed;
                    e.has_value = true;
                    e.value = detail::value_of(*v);
                    break;
                default:
                    e.has_value = true;
                    e.value = detail::value_of(*v);
            }
        }
        lv.entries[kv.first] = std::move(e);
    }
    out = std::move(lv);
    return LightParse::Ok;
}

/// The page's view of a light answer: the copy of the screen for the texts and the values of the answer.
inline Loaded loaded_from_light(const LightValues& lv) {
    Loaded out;
    out.contract = embedded_contract();
    out.light = true;
    out.server_version = lv.server_version;
    out.server_hash = lv.contract_hash;
    out.hash = compare_hash(lv.contract_hash);
    for (const auto& kv : lv.entries) {
        const LightEntry& e = kv.second;
        if (e.secret) out.secret_keys.insert(kv.first);
        else if (e.hidden) out.hidden.insert(kv.first);
        else if (e.has_value) out.values[kv.first] = e.value;
        if (e.truncated && !e.secret && !e.hidden) out.truncated.insert(kv.first);
        if (e.overridden && !e.secret) out.overridden.insert(kv.first);
    }
    return out;
}

/// What a try to read the light view came to.
enum class LightResult { Fresh, Unchanged, Failed };

/// The last light answer and its ETag: the next request carries the tag in `If-None-Match`, and a `304` gives the answer back with nothing to read.
class LightCache {
public:
    /// The request for the light view (`GET /api/v1/settings/values`) and the tag of the answer held (`If-None-Match`; empty when there is none).
    Request request() const { return {"GET", kLightPath, ""}; }
    const std::string& if_none_match() const { return etag_; }
    bool has() const { return have_; }
    const std::string& etag() const { return etag_; }
    const Loaded& last() const { return last_; }
    /// Forgets the answer: the login changed or something failed, so the next request asks for the whole of it.
    void clear() {
        have_ = false;
        etag_.clear();
        last_ = Loaded();
    }
    /// Keeps an answer under its tag; an answer with no valid tag cannot be asked for again by tag, so nothing is kept.
    void keep(const std::string& etag, const Loaded& loaded) {
        if (!etag_ok(etag)) { clear(); return; }
        have_ = true;
        etag_ = etag;
        last_ = loaded;
    }

private:
    bool have_ = false;
    std::string etag_;
    Loaded last_;
};

/// Reads the answer of one request for the light view (`etag` is the ETag header of the answer, empty when there is none): `Fresh` (a new answer, in `out`), `Unchanged` (a `304`; `out` is the answer held) or `Failed` (`out` is the copy of the screen and
/// `out.why` says why: `unreachable`, `toolong`, `shape`, `newer`, a code of the server or `http_<status>`). A failure forgets the answer held.
inline LightResult apply_light(bool reached, CallError error, const Response& r, const std::string& etag, LightCache& cache, Loaded& out) {
    out = Loaded();
    out.contract = embedded_contract();
    if (!reached) {
        out.why = error == CallError::TooLong ? "toolong" : "unreachable";
        cache.clear();
        return LightResult::Failed;
    }
    if (r.status == 304) {
        if (cache.has()) {
            out = cache.last();
            return LightResult::Unchanged;
        }
        out.why = "shape";  // a 304 for a tag nobody sent
        return LightResult::Failed;
    }
    if (r.status != 200) {
        Json j;
        std::string code;
        if (parse_json(r.body, j) && j.type == Json::Type::Object) code = j.str("error");
        out.why = detail::looks_like_code(code) ? code : "http_" + std::to_string(r.status);
        cache.clear();
        return LightResult::Failed;
    }
    if (r.body.size() > kLightMaxBytes) {
        out.why = "toolong";
        cache.clear();
        return LightResult::Failed;
    }
    LightValues lv;
    switch (parse_light(r.body, lv)) {
        case LightParse::Ok: break;
        case LightParse::NewerSchema: out.why = "newer"; cache.clear(); return LightResult::Failed;
        default: out.why = "shape"; cache.clear(); return LightResult::Failed;
    }
    out = loaded_from_light(lv);
    cache.keep(etag, out);
    return LightResult::Fresh;
}

/// The conditional part of a read: the ETag to send as `If-None-Match` (empty: none) and the ETag the answer came with (empty: none or not a tag).
struct Conditional {
    std::string if_none_match;
    std::string etag;
};

/// How the link asks the server for a request: true with the answer, false with `error` when no answer came (the firmware's `srvlink::call`, or a simulated server in the tests).
using Fetch = std::function<bool(const Request&, Response&, CallError&, Conditional&)>;

/// What a round of reading the settings gave.
struct ReadResult {
    Loaded loaded;
    bool changed = true;  // false: the server said `304`, what the page shows is still right
};

/// One round of reading the settings of the server for a login of `role` (empty when the role is not known). An administrator asks for the full contract first (it holds the texts and the
/// values); anybody else, and an administrator whose full contract could not be read, asks for the light view when `settings_values` is on, with the tag of the last answer. With
/// `settings_values` off a login that is not an administrator still tries the full contract (an older arrangement of the server may give it). The copy of the screen is what remains.
inline ReadResult read_settings(const Fetch& fetch, const std::string& role, const Settings& cfg, LightCache& cache) {
    ReadResult out;
    if (role == "admin" || !cfg.settings_values) {
        Response r;
        CallError error = CallError::None;
        Conditional none;
        const bool got = fetch(contract_request(), r, error, none);
        Loaded full = load_contract(got, error, r);
        if (full.live || !cfg.settings_values) {
            cache.clear();
            out.loaded = std::move(full);
            return out;
        }
    }
    Response r;
    CallError error = CallError::None;
    Conditional cond;
    cond.if_none_match = cache.if_none_match();
    const bool got = fetch(cache.request(), r, error, cond);
    const LightResult result = apply_light(got, error, r, cond.etag, cache, out.loaded);
    out.changed = result != LightResult::Unchanged;
    return out;
}

/// The seconds until the next round: the poll time of the light view when that is what the page shows, the poll time of the contract otherwise.
inline std::uint32_t read_interval_s(const Loaded& shown, const Settings& cfg) { return shown.light ? cfg.settings_values_poll_s : cfg.settings_poll_s; }

}  // namespace hmi
