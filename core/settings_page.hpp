// EH3D-NODE-HMI-GUI - core/settings_page.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The page of the settings of the server for a touch screen next to a machine: what it shows, which settings it may change and how a change is made.
//
// The screen READS every setting of the contract (a value in force when the server gives the live contract, the name and the texts always). It CHANGES a setting only when all of these hold:
// the contract is the live one, the key is written through `PUT /auth/policy` (the 19 keys of the security page; the other 70 are set in server.cfg), the login of the screen is an
// administrator, `settings_write` is on in hmi.cfg (off by default), the key is in `settings_write_keys` of hmi.cfg (empty by default) and no job runs. A change is asked for with the words
// of the setting, a risky one asks twice, a value the server calls looser than recommended is asked about a third time, and a spoken order never starts one. The screen never shows, asks
// for or writes a secret. A contract it does not understand, a key it does not know or a route it does not trust is shown read only. The time is a number (milliseconds of a clock that
// only goes forward and may wrap), so every run of the tests is the same.
#pragma once

#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "hmi_config.hpp"
#include "rack.hpp"
#include "server_errors.hpp"
#include "settings_contract.hpp"
#include "settings_text.hpp"
#include "settings_values.hpp"

namespace hmi {

// ---- which settings the screen may change -----------------------------------------------------------------------------------------------------

/// Why a setting is shown but cannot be changed on this screen (`None`: it can).
enum class ReadOnlyWhy { None, File, Secret, Copy, Route, Generic, Role, Disabled, NotListed, Running };

/// The three switches of hmi.cfg that open the writing of the settings of the server, as the screen holds them.
struct WritePolicy {
    bool enabled = false;               // settings_write
    std::set<std::string> keys;         // settings_write_keys
    bool block_running = true;          // settings_block_running
};

/// A list of keys (`slice_enabled, design_run`): up to 32 names of lower-case letters, digits and underscores, once each; nothing is an empty list. False for anything else.
inline bool parse_key_list(const std::string& text, std::vector<std::string>& out) { return detail::to_key_list(text, out); }

inline WritePolicy write_policy(const Settings& s) {
    WritePolicy p;
    p.enabled = s.settings_write;
    p.block_running = s.settings_block_running;
    std::vector<std::string> keys;
    if (parse_key_list(s.settings_write_keys, keys)) p.keys.insert(keys.begin(), keys.end());
    return p;
}

/// The first reason a setting cannot be changed on this screen, in the order an operator would want to be told: a key set in the file is said to be so whatever else holds.
inline ReadOnlyWhy why_read_only(const Setting& s, bool live, const std::string& role, const WritePolicy& p, bool job_running) {
    if (s.via == "file") return ReadOnlyWhy::File;
    if (s.via == "none") return ReadOnlyWhy::Generic;
    if (s.secret) return ReadOnlyWhy::Secret;
    if (!live) return ReadOnlyWhy::Copy;
    if (!has_trusted_route(s)) return ReadOnlyWhy::Route;
    if (control_kind(s) == ControlKind::Generic) return ReadOnlyWhy::Generic;
    if (!s.role.empty() && !role_at_least(role, s.role)) return ReadOnlyWhy::Role;
    if (!p.enabled) return ReadOnlyWhy::Disabled;
    if (p.keys.count(s.key) == 0) return ReadOnlyWhy::NotListed;
    if (p.block_running && job_running) return ReadOnlyWhy::Running;
    return ReadOnlyWhy::None;
}

inline ST read_only_text(ReadOnlyWhy w) {
    switch (w) {
        case ReadOnlyWhy::File: return ST::RoFile;
        case ReadOnlyWhy::Secret: return ST::RoSecret;
        case ReadOnlyWhy::Copy: return ST::RoCopy;
        case ReadOnlyWhy::Route: return ST::RoRoute;
        case ReadOnlyWhy::Role: return ST::RoRole;
        case ReadOnlyWhy::Disabled: return ST::RoDisabled;
        case ReadOnlyWhy::NotListed: return ST::RoNotListed;
        case ReadOnlyWhy::Running: return ST::RoRunning;
        default: return ST::RoUnknown;
    }
}

// ---- what the page shows ------------------------------------------------------------------------------------------------------------------------

struct RowView {
    std::string key, label, value, note;
    ControlKind control = ControlKind::Generic;
    ReadOnlyWhy why = ReadOnlyWhy::Generic;
    bool editable = false;
    bool restart = false;     // a change is read when the server starts
    bool high_risk = false;   // a change asks twice
    bool not_default = false; // the value in force is not the default of the contract
    bool overridden = false;  // the light view says it was changed through the security page and differs from server.cfg
};

struct GroupView {
    std::string id, label;
    std::vector<RowView> rows;
};

struct PageView {
    bool live = false;
    std::string source;                  // one line: where the values come from
    std::string why;                     // one line: why the copy is shown (empty for a live contract)
    std::vector<GroupView> groups;
    std::vector<std::string> warnings;   // the lines the page adds under the title: new keys of the server, a place of the bar that the server does not have
    std::size_t row_count = 0;
    std::size_t editable_count = 0;
};

/// The unit of a setting in a few characters that need no translation (`12 h`, `8 MB`); a unit the screen does not know is shown as the contract writes it, and the units of a count
/// (`lines`, `places`) are left out because the label already says what is counted.
inline std::string unit_short(const std::string& unit) {
    if (unit == "seconds") return "s";
    if (unit == "hours") return "h";
    if (unit == "days") return "d";
    if (unit == "megabytes") return "MB";
    if (unit == "kilobytes") return "KB";
    if (unit == "gigabytes") return "GB";
    if (unit == "millimetres") return "mm";
    if (unit == "micrometres") return "\xC2\xB5m";
    if (unit == "lines" || unit == "places") return "";
    return unit;
}

/// The value in force as the operator reads it: On or Off for a switch, the number with its unit, the list, the text; `-` when the server did not give one; Hidden for a secret.
inline std::string display_value(const Setting& s, const Loaded& loaded, Lang l) {
    if (s.secret || loaded.secret_keys.count(s.key) != 0) return settings_text(ST::Hidden, l);       // never the value, whatever the server sent
    if (loaded.hidden.count(s.key) != 0) return settings_text(ST::NotAvailable, l);                  // the server does not give it (a path of the computer)
    const auto it = loaded.values.find(s.key);
    if (!loaded.has_values() || it == loaded.values.end() || it->second.kind == SValue::Kind::None) return "-";
    const SValue& v = it->second;
    std::string shown;
    switch (v.kind) {
        case SValue::Kind::Bool: shown = settings_text(v.b ? ST::On : ST::Off, l); break;
        case SValue::Kind::Int: {
            const std::string unit = unit_short(s.unit);
            shown = std::to_string(v.n) + (unit.empty() ? "" : " " + unit);
            break;
        }
        default: {
            shown = value_text(v);
            if (shown.empty()) return "-";
        }
    }
    return loaded.truncated.count(s.key) != 0 ? shown + "..." : shown;  // the server cut a long text or list
}

/// The lines the page adds about what the server says and what hmi.cfg says do not fit.
inline std::vector<std::string> cross_check_lines(const Loaded& loaded, const Settings& cfg, Lang l) {
    std::vector<std::string> out;
    if (!loaded.has_values()) return out;
    if (loaded.hash == HashState::Differs) out.push_back(settings_text(ST::HashDiffers, l));
    const Drift drift = contract_drift(loaded);
    if (!drift.newer_in_server.empty()) out.push_back(fill(settings_text(ST::NewKeys, l), {{"n", std::to_string(drift.newer_in_server.size())}}));
    const auto slots = loaded.values.find("rack_max_slots");
    std::vector<int> places;
    if (slots != loaded.values.end() && slots->second.kind == SValue::Kind::Int && parse_places(cfg.rack_places, places)) {
        int highest = 0;
        for (int p : places) highest = p > highest ? p : highest;
        if (highest > slots->second.n) out.push_back(fill(settings_text(ST::RackWarn, l), {{"server", std::to_string(slots->second.n)}, {"screen", std::to_string(highest)}}));
    }
    return out;
}

/// The line to show when a file of `bytes` is larger than what the server takes in one request (`max_body_mb`); empty when it fits or the server did not say.
inline std::string body_warning(std::uint64_t bytes, const Loaded& loaded, Lang l) {
    const auto it = loaded.values.find("max_body_mb");
    if (!loaded.has_values() || it == loaded.values.end() || it->second.kind != SValue::Kind::Int || it->second.n <= 0) return "";
    const std::uint64_t limit = static_cast<std::uint64_t>(it->second.n) * 1024u * 1024u;
    return bytes > limit ? fill(settings_text(ST::BodyWarn, l), {{"mb", std::to_string(it->second.n)}}) : "";
}

/// Why the copy is shown instead of the live contract, as a sentence in the language of the operator.
inline std::string why_copy_text(const std::string& why, Lang l) {
    if (why.empty()) return "";
    if (why == "unreachable") return settings_text(ST::WhyUnreachable, l);
    if (why == "toolong") return settings_text(ST::WhyTooLong, l);
    if (why == "shape" || why == "newer") return settings_text(ST::WhyShape, l);
    if (why == "forbidden" || why == "forbidden_scope" || why == "unauthorized") return settings_text(ST::WhyForbidden, l);
    const std::string known = server_error_text(why, l);
    return known.empty() ? why : known;
}

inline PageView build_page(const Loaded& loaded, const Settings& cfg, Lang l, const std::string& role, bool job_running) {
    PageView v;
    v.live = loaded.live;
    v.source = settings_text(loaded.live ? ST::SourceLive : loaded.light ? ST::SourceLight : ST::SourceCopy, l);
    if (loaded.has_values() && !loaded.server_version.empty()) v.source += " (" + loaded.server_version + ")";
    v.why = why_copy_text(loaded.why, l);
    const WritePolicy policy = write_policy(cfg);
    Contract view = loaded.contract;
    if (loaded.has_values()) {
        for (const Setting& extra : unlisted_settings(loaded.contract, loaded.values)) view.settings.push_back(extra);
        // keys of the light view that the contract does not list and whose value the server keeps back: drawn read only, with no value
        for (const std::set<std::string>* kept : {&loaded.secret_keys, &loaded.hidden})
            for (const std::string& key : *kept)
                if (loaded.contract.find(key) == nullptr) view.settings.push_back(generic_setting(key, SValue()));
    }
    for (const SettingsGroup& g : group_settings(view)) {
        GroupView gv;
        gv.id = g.id;
        gv.label = contract_text(g.label, l, g.id);
        for (const Setting& s : g.settings) {
            RowView r;
            r.key = s.key;
            r.label = contract_text(s.label, l, s.key);
            r.value = display_value(s, loaded, l);
            r.control = control_kind(s);
            r.why = why_read_only(s, loaded.live, role, policy, job_running);
            r.editable = r.why == ReadOnlyWhy::None;
            r.restart = s.apply == Apply::Restart;
            r.high_risk = is_high_risk(s);
            const auto val = loaded.values.find(s.key);
            r.not_default = loaded.has_values() && !s.secret && loaded.truncated.count(s.key) == 0 && s.has_default && val != loaded.values.end() && val->second.kind != SValue::Kind::None && val->second != s.def;
            r.overridden = loaded.has_values() && loaded.overridden.count(s.key) != 0;
            if (!r.editable) r.note = settings_text(read_only_text(r.why), l);
            else if (r.restart) r.note = settings_text(ST::RestartTag, l);
            gv.rows.push_back(std::move(r));
            ++v.row_count;
            if (gv.rows.back().editable) ++v.editable_count;
        }
        v.groups.push_back(std::move(gv));
    }
    v.warnings = cross_check_lines(loaded, cfg, l);
    return v;
}

/// The words for a problem with what was typed, in the language of the operator.
inline std::string problem_text(InputProblem p, const Setting& s, Lang l) {
    switch (p) {
        case InputProblem::Number: return fill(settings_text(ST::BadNumber, l), {{"min", std::to_string(s.min)}, {"max", std::to_string(s.max)}});
        case InputProblem::Whole: return settings_text(ST::BadWhole, l);
        case InputProblem::Text: return fill(settings_text(ST::BadText, l), {{"max", std::to_string(s.max_length)}});
        case InputProblem::Choice: return settings_text(ST::BadChoice, l);
        default: return "";
    }
}

/// The line the page shows after a change was sent.
inline std::string save_text(const SaveResult& r, const Setting& s, Lang l, const Settings& cfg) {
    const std::string label = contract_text(s.label, l, s.key);
    switch (r.kind) {
        case SaveKind::Saved: return fill(settings_text(r.restart_required ? ST::SavedRestart : ST::Saved, l), {{"label", label}});
        case SaveKind::NeedsWeakConfirm: return settings_text(ST::AskWeak, l);
        case SaveKind::Refused: {
            const std::string known = server_error_text(r.code, l);
            return fill(settings_text(ST::Refused, l), {{"why", known.empty() ? (r.detail.empty() ? r.code : r.code + ": " + r.detail) : known}});
        }
        default: return fill(settings_text(ST::NotSent, l), {{"why", call_error_text(r.error, cfg, l)}});
    }
}

// ---- a change, step by step ---------------------------------------------------------------------------------------------------------------------

/// Where a change stands: nothing under way, the first question, the second question of a risky setting, the question about a value the server calls looser than recommended, or sent.
enum class EditStep { Idle, Ask, AskRisk, AskWeak, Sent };

/// Why a change could not be started.
enum class BeginResult { Started, ReadOnly, ByVoice, Busy };

/// One change at a time. The screen asks the operator (the question is `question()`), counts `confirm_timeout_ms` for each answer and gives up by itself when it runs out; nothing is
/// sent without the touch that answers the last question. The caller sends the request that `confirm` gives and tells the result with `on_result`.
class SettingsEditor {
public:
    EditStep step() const { return step_; }
    bool busy() const { return step_ != EditStep::Idle; }
    const std::string& key() const { return key_; }
    const SValue& value() const { return value_; }

    /// Starts a change of `s` to `value`. Refused when the setting is read only on this screen, when the order came by voice (a spoken order never changes a setting) and when
    /// another change is under way.
    BeginResult begin(const Setting& s, const SValue& value, ReadOnlyWhy why, bool by_voice, std::uint32_t now_ms, std::uint32_t timeout_ms) {
        if (busy()) return BeginResult::Busy;
        if (by_voice) return BeginResult::ByVoice;
        if (why != ReadOnlyWhy::None) return BeginResult::ReadOnly;
        setting_ = s;
        key_ = s.key;
        value_ = value;
        step_ = EditStep::Ask;
        timeout_ms_ = timeout_ms;
        deadline_ms_ = now_ms + timeout_ms;
        return BeginResult::Started;
    }

    /// The question the page shows now, in the language of the operator.
    std::string question(Lang l) const {
        const std::string label = contract_text(setting_.label, l, setting_.key);
        switch (step_) {
            case EditStep::Ask: return fill(settings_text(ST::AskChange, l), {{"label", label}, {"value", shown_value(l)}});
            case EditStep::AskRisk: {
                const std::string warning = contract_text(setting_.warning, l);
                return (warning.empty() ? "" : warning + " ") + settings_text(ST::AskAgain, l);
            }
            case EditStep::AskWeak: return settings_text(ST::AskWeak, l);
            default: return "";
        }
    }

    /// The milliseconds left to answer the question; 0 when none is waiting.
    std::uint32_t remaining_ms(std::uint32_t now_ms) const {
        if (step_ != EditStep::Ask && step_ != EditStep::AskRisk && step_ != EditStep::AskWeak) return 0;
        const std::int32_t left = static_cast<std::int32_t>(deadline_ms_ - now_ms);
        return left > 0 ? static_cast<std::uint32_t>(left) : 0;
    }

    /// Gives up a question nobody answered in time. True when it did.
    bool tick(std::uint32_t now_ms) {
        if (step_ != EditStep::Idle && step_ != EditStep::Sent && static_cast<std::int32_t>(now_ms - deadline_ms_) >= 0) { reset(); return true; }
        return false;
    }

    /// The operator touched Confirm. A risky setting asks a second time first; the answer of the last question gives the request to send (and true). A touch that comes when the
    /// time of the question is over gives it up and sends nothing, also when `tick` has not run yet.
    bool confirm(std::uint32_t now_ms, Request& out) {
        if (tick(now_ms)) return false;
        if (step_ == EditStep::Ask && is_high_risk(setting_)) {
            step_ = EditStep::AskRisk;
            deadline_ms_ = now_ms + timeout_ms_;
            return false;
        }
        if (step_ == EditStep::Ask || step_ == EditStep::AskRisk || step_ == EditStep::AskWeak) {
            out = write_request(setting_, value_, step_ == EditStep::AskWeak);
            step_ = EditStep::Sent;
            return true;
        }
        return false;
    }

    /// The operator touched Cancel (or the stop, or left the page).
    void cancel() { reset(); }

    /// The answer of the write. A `NeedsWeakConfirm` asks once more (the same change is then sent with `confirm_weak`); every other result ends the change.
    void on_result(const SaveResult& r, std::uint32_t now_ms) {
        if (step_ != EditStep::Sent) return;
        if (r.kind == SaveKind::NeedsWeakConfirm && !weak_asked_) {
            weak_asked_ = true;
            step_ = EditStep::AskWeak;
            deadline_ms_ = now_ms + timeout_ms_;
            return;
        }
        reset();
    }

    const Setting& setting() const { return setting_; }

private:
    std::string shown_value(Lang l) const {
        if (value_.kind == SValue::Kind::Bool) return settings_text(value_.b ? ST::On : ST::Off, l);
        const std::string t = value_text(value_);
        return t.empty() ? "-" : t;
    }
    void reset() {
        step_ = EditStep::Idle;
        key_.clear();
        value_ = SValue();
        weak_asked_ = false;
    }

    EditStep step_ = EditStep::Idle;
    Setting setting_;
    std::string key_;
    SValue value_;
    std::uint32_t timeout_ms_ = 8000;
    std::uint32_t deadline_ms_ = 0;
    bool weak_asked_ = false;
};

// ---- when the contract is read -------------------------------------------------------------------------------------------------------------------

/// When the link reads the contract: a first time soon after the server answers, then every `settings_poll_s`; after a failure it tries again after the poll time but never later than
/// five minutes, so a contract that appears (the login became an administrator, the server was updated) is picked up without a restart. Times are milliseconds of a clock that only goes forward.
class ContractPoller {
public:
    bool due(bool enabled, std::uint32_t now_ms) const { return enabled && (!started_ || static_cast<std::int32_t>(now_ms - next_ms_) >= 0); }
    void done(bool ok, std::uint32_t poll_s, std::uint32_t now_ms) {
        started_ = true;
        const std::uint32_t wait_s = ok ? poll_s : (poll_s < 300 ? poll_s : 300);
        next_ms_ = now_ms + wait_s * 1000u;
    }
    /// Read again at the next round (the login changed, or the operator asked).
    void again() { started_ = false; }

private:
    bool started_ = false;
    std::uint32_t next_ms_ = 0;
};

}  // namespace hmi
