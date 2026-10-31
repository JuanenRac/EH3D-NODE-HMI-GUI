// EH3D-NODE-HMI-GUI - core/keyboard.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// What the screen types with. Every setting that can be edited on the screen is described once (what kind of value it is, its range, its length), and that tells
// which keyboard it needs: a numeric pad for a number (with the range and the decimals of that number), the full keyboard for a text (the keys of the language in use),
// a switch or a list for the rest, with no keyboard at all. The pad and the keyboard are state machines that the screen draws; the rules are here so that they are
// tested on a computer.
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "lang.hpp"

namespace hmi {

enum class FieldKind { Number, Text, Secret, Switch, List };

struct FieldSpec {
    const char* key;
    FieldKind kind;
    double min = 0;                 // Number: the lowest value
    double max = 0;                 // Number: the highest value
    int decimals = 0;               // Number: how many digits after the point
    std::size_t min_len = 0;        // Text and Secret: the shortest text that is accepted (0: it may be empty)
    std::size_t max_len = 0;        // Text and Secret: the longest text
};

/// Every setting the screen edits, in the order of the settings page.
inline const std::vector<FieldSpec>& field_specs() {
    static const std::vector<FieldSpec> specs = {
        {"language", FieldKind::List},
        {"theme", FieldKind::List},
        {"orientation", FieldKind::List},
        {"brightness_percent", FieldKind::Number, 10, 100, 0},
        {"idle_dim_s", FieldKind::Number, 0, 3600, 0},
        {"server_host", FieldKind::Text, 0, 0, 0, 0, 63},
        {"server_port", FieldKind::Number, 1, 65535, 0},
        {"server_machine", FieldKind::Text, 0, 0, 0, 0, 31},
        {"token", FieldKind::Secret, 0, 0, 0, 8, 64},
        {"poll_s", FieldKind::Number, 1, 60, 0},
        {"confirm_moves", FieldKind::Switch},
        {"confirm_timeout_ms", FieldKind::Number, 1000, 60000, 0},
        {"jog_max_mm", FieldKind::Number, 1, 200, 0},
        {"spindle_max_rpm", FieldKind::Number, 0, 65535, 0},
        {"hw_stop_required", FieldKind::Switch},
        {"link_idle_ms", FieldKind::Number, 200, 60000, 0},
        {"voice_alerts", FieldKind::Switch},
        {"voice_orders", FieldKind::Switch},
        {"voice_confirm", FieldKind::Switch},
        {"volume_percent", FieldKind::Number, 0, 100, 0},
        {"mic_gain_percent", FieldKind::Number, 0, 100, 0},
        {"sound_finish", FieldKind::List},
        {"sound_tool_change", FieldKind::List},
    };
    return specs;
}

inline const FieldSpec* field_spec(const std::string& key) {
    for (const auto& s : field_specs()) {
        if (key == s.key) return &s;
    }
    return nullptr;
}

enum class KeyboardKind { None, Numeric, Full };

inline KeyboardKind keyboard_for(const FieldSpec& s) {
    switch (s.kind) {
        case FieldKind::Number: return KeyboardKind::Numeric;
        case FieldKind::Text:
        case FieldKind::Secret: return KeyboardKind::Full;
        default: return KeyboardKind::None;
    }
}

// ---------------------------------------------------------------------------------------------------------------- the numeric pad

enum class PadKey { D0, D1, D2, D3, D4, D5, D6, D7, D8, D9, Point, Minus, Back, Clear, Up, Down };

/// The numeric pad: it keeps the text that is being typed and says whether it is a number inside the range.
class NumberPad {
public:
    explicit NumberPad(const FieldSpec& spec, const std::string& current = "") : spec_(spec), text_(current) {}

    const std::string& text() const { return text_; }

    void press(PadKey k) {
        switch (k) {
            case PadKey::Back:
                if (!text_.empty()) text_.pop_back();
                return;
            case PadKey::Clear: text_.clear(); return;
            case PadKey::Up: step(+1); return;
            case PadKey::Down: step(-1); return;
            case PadKey::Minus:
                // a minus only makes sense at the start and when the range goes below zero
                if (spec_.min < 0 && text_.empty()) text_ = "-";
                return;
            case PadKey::Point:
                if (spec_.decimals > 0 && text_.find('.') == std::string::npos) text_ += text_.empty() || text_ == "-" ? "0." : ".";
                return;
            default: break;
        }
        char digit = static_cast<char>('0' + static_cast<int>(k));
        std::size_t point = text_.find('.');
        if (point != std::string::npos && text_.size() - point - 1 >= static_cast<std::size_t>(spec_.decimals)) return;  // no more decimals than the setting has
        if (text_ == "0") text_.clear();                                                                               // no leading zero
        if (text_ == "-0") text_ = "-";
        if (text_.size() >= 12) return;
        text_ += digit;
    }

    /// The value, when the text is a number inside the range.
    bool valid(double* value = nullptr) const {
        if (text_.empty() || text_ == "-" || text_.back() == '.') return false;
        double v = parse(text_);
        if (v < spec_.min || v > spec_.max) return false;
        if (value) *value = v;
        return true;
    }

private:
    static double parse(const std::string& t) {
        bool neg = !t.empty() && t[0] == '-';
        double n = 0, scale = 1;
        bool dot = false;
        for (std::size_t i = neg ? 1 : 0; i < t.size(); ++i) {
            if (t[i] == '.') { dot = true; continue; }
            if (dot) { scale /= 10; n += (t[i] - '0') * scale; } else n = n * 10 + (t[i] - '0');
        }
        return neg ? -n : n;
    }

    /// The arrows move the value by one unit of the last digit that setting has (1 for a whole number), kept inside the range.
    void step(int dir) {
        double v = text_.empty() ? spec_.min : parse(text_);
        double unit = 1;
        for (int i = 0; i < spec_.decimals; ++i) unit /= 10;
        v += dir * unit;
        if (v < spec_.min) v = spec_.min;
        if (v > spec_.max) v = spec_.max;
        char buf[32];
        std::snprintf(buf, sizeof buf, "%.*f", spec_.decimals, v);
        text_ = buf;
    }

    FieldSpec spec_;
    std::string text_;
};

// ---------------------------------------------------------------------------------------------------------------- the full keyboard

enum class Layer { Lower, Upper, Symbols };

/// A key as the screen draws it: what it shows and what it does.
struct Key {
    enum class Do { Char, Shift, Symbols, Letters, Space, Back, Done } what = Do::Char;
    std::string label;  // the letter (UTF-8) for a character; empty for the others, the screen names them
};

namespace detail {

/// The characters (UTF-8) of a string, one by one.
inline std::vector<std::string> chars_of(const std::string& s) {
    std::vector<std::string> out;
    for (std::size_t i = 0; i < s.size();) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        std::size_t n = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;
        out.push_back(s.substr(i, n));
        i += n;
    }
    return out;
}

/// The characters of an upper-case row (the letters the languages have besides the basic ones come as pairs).
inline std::string upper_of(const std::string& lower) {
    static const char* pairs[][2] = {{"ä", "Ä"}, {"ö", "Ö"}, {"ü", "Ü"}, {"é", "É"}, {"è", "È"}, {"à", "À"}, {"ù", "Ù"}, {"ç", "Ç"}, {"ñ", "Ñ"}, {"ã", "Ã"}, {"õ", "Õ"},
                                      {"ì", "Ì"}, {"ò", "Ò"}, {"á", "Á"}, {"í", "Í"}, {"ó", "Ó"}, {"ú", "Ú"}, {"ê", "Ê"}, {"ô", "Ô"}, {"â", "Â"}, {"ß", "ß"}};
    for (const auto& p : pairs) {
        if (lower == p[0]) return p[1];
    }
    if (lower.size() == 1 && lower[0] >= 'a' && lower[0] <= 'z') return std::string(1, static_cast<char>(lower[0] - 'a' + 'A'));
    return lower;
}

}  // namespace detail

/// The letter rows of a language (lower case); the screen adds the row of digits, the shift, the symbols, the space and the other keys.
inline std::vector<std::string> letter_rows(Lang l) {
    switch (l) {
        case Lang::De: return {"qwertzuiopü", "asdfghjklöä", "yxcvbnmß"};
        case Lang::Fr: return {"azertyuiop", "qsdfghjklmé", "wxcvbnèàç"};
        case Lang::Es: return {"qwertyuiop", "asdfghjklñ", "zxcvbnmáéíóú"};
        case Lang::Pt: return {"qwertyuiop", "asdfghjklç", "zxcvbnmãõáéê"};
        case Lang::It: return {"qwertyuiop", "asdfghjklàè", "zxcvbnmìòù"};
        default: return {"qwertyuiop", "asdfghjkl", "zxcvbnm"};  // English, and Chinese and Japanese typed in letters
    }
}

/// The symbols that settings need (host names, addresses, a token): no more than the keyboard of a terminal.
inline std::vector<std::string> symbol_rows() { return {"1234567890", "-_.:/@#=+*", "!?&%()[]<>"}; }

/// The keys of one layer, row by row. The digits are always the first row.
inline std::vector<std::vector<Key>> layout(Lang lang, Layer layer) {
    std::vector<std::vector<Key>> rows;
    auto chars = [](const std::string& s, bool upper) {
        std::vector<Key> r;
        for (const auto& c : detail::chars_of(s)) r.push_back({Key::Do::Char, upper ? detail::upper_of(c) : c});
        return r;
    };
    if (layer == Layer::Symbols) {
        for (const auto& r : symbol_rows()) rows.push_back(chars(r, false));
        rows.push_back({{Key::Do::Letters, ""}, {Key::Do::Space, ""}, {Key::Do::Back, ""}, {Key::Do::Done, ""}});
        return rows;
    }
    rows.push_back(chars("1234567890", false));
    auto letters = letter_rows(lang);
    for (std::size_t i = 0; i < letters.size(); ++i) {
        auto r = chars(letters[i], layer == Layer::Upper);
        if (i + 1 == letters.size()) {
            r.insert(r.begin(), {Key::Do::Shift, ""});
            r.push_back({Key::Do::Back, ""});
        }
        rows.push_back(r);
    }
    rows.push_back({{Key::Do::Symbols, ""}, {Key::Do::Space, ""}, {Key::Do::Done, ""}});
    return rows;
}

/// The full keyboard that types into a text: the shift is for one letter, as on a phone, and the text stays inside the length of the setting.
class TextEntry {
public:
    TextEntry(const FieldSpec& spec, Lang lang, const std::string& current = "") : spec_(spec), lang_(lang), text_(current) {}

    const std::string& text() const { return text_; }
    Layer layer() const { return layer_; }

    void press(const Key& k) {
        switch (k.what) {
            case Key::Do::Char: {
                if (detail::chars_of(text_).size() >= spec_.max_len) return;
                text_ += k.label;
                if (layer_ == Layer::Upper) layer_ = Layer::Lower;
                return;
            }
            case Key::Do::Space:
                if (detail::chars_of(text_).size() < spec_.max_len) text_ += ' ';
                return;
            case Key::Do::Back: {
                auto cs = detail::chars_of(text_);
                if (cs.empty()) return;
                text_.erase(text_.size() - cs.back().size());
                return;
            }
            case Key::Do::Shift: layer_ = layer_ == Layer::Upper ? Layer::Lower : Layer::Upper; return;
            case Key::Do::Symbols: layer_ = Layer::Symbols; return;
            case Key::Do::Letters: layer_ = Layer::Lower; return;
            case Key::Do::Done: return;
        }
    }

    /// The text is accepted: empty (the setting is cleared) or not shorter than the setting wants.
    bool valid() const {
        std::size_t n = detail::chars_of(text_).size();
        return n == 0 || n >= spec_.min_len;
    }

    std::vector<std::vector<Key>> keys() const { return layout(lang_, layer_); }

private:
    FieldSpec spec_;
    Lang lang_;
    std::string text_;
    Layer layer_ = Layer::Lower;
};

}  // namespace hmi
