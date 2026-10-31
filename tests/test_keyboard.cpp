// EH3D-NODE-HMI-GUI host tests: which keyboard each setting needs, the numeric pad with the range of its setting, and the full keyboard of each language.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "../core/hmi_config.hpp"
#include "../core/keyboard.hpp"

using namespace hmi;

static int failures = 0;
static int checks = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        ++checks;                                                       \
        if (!(cond)) {                                                  \
            ++failures;                                                 \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
        }                                                               \
    } while (0)

static void type(NumberPad& p, const std::string& digits) {
    for (char c : digits) {
        if (c == '.') p.press(PadKey::Point);
        else if (c == '-') p.press(PadKey::Minus);
        else p.press(static_cast<PadKey>(c - '0'));
    }
}

static const Key* find(const std::vector<std::vector<Key>>& rows, const std::string& label) {
    for (const auto& r : rows)
        for (const auto& k : r)
            if (k.what == Key::Do::Char && k.label == label) return &k;
    return nullptr;
}

static const Key* find_do(const std::vector<std::vector<Key>>& rows, Key::Do what) {
    for (const auto& r : rows)
        for (const auto& k : r)
            if (k.what == what) return &k;
    return nullptr;
}

static void every_setting_has_its_keyboard() {
    // every key the settings file reads has a description, and the other way round
    const char* keys[] = {"language", "theme", "orientation", "brightness_percent", "idle_dim_s", "server_host", "server_port", "server_machine", "token", "poll_s",
                          "confirm_moves", "confirm_timeout_ms", "jog_max_mm", "spindle_max_rpm", "hw_stop_required", "link_idle_ms", "voice_alerts", "voice_orders",
                          "voice_confirm", "volume_percent", "mic_gain_percent", "sound_finish", "sound_tool_change"};
    for (const char* k : keys) CHECK(field_spec(k) != nullptr);
    CHECK(field_specs().size() == sizeof(keys) / sizeof(keys[0]));
    CHECK(field_spec("colour") == nullptr);
    // the kind of keyboard follows the kind of value
    CHECK(keyboard_for(*field_spec("poll_s")) == KeyboardKind::Numeric);
    CHECK(keyboard_for(*field_spec("server_port")) == KeyboardKind::Numeric);
    CHECK(keyboard_for(*field_spec("server_host")) == KeyboardKind::Full);
    CHECK(keyboard_for(*field_spec("token")) == KeyboardKind::Full);
    CHECK(keyboard_for(*field_spec("confirm_moves")) == KeyboardKind::None);
    CHECK(keyboard_for(*field_spec("language")) == KeyboardKind::None);
    // the ranges of the pad are the ones the settings file accepts: a value the pad takes is a value the file takes
    for (const auto& s : field_specs()) {
        if (s.kind != FieldKind::Number) continue;
        for (double v : {s.min, s.max}) {
            std::vector<Problem> problems;
            char line[96];
            std::snprintf(line, sizeof line, "%s = %.0f\n", s.key, v);
            parse_settings(line, problems);
            CHECK(problems.empty());
        }
        for (double v : {s.min - 1, s.max + 1}) {
            if (v < 0) continue;
            std::vector<Problem> problems;
            char line[96];
            std::snprintf(line, sizeof line, "%s = %.0f\n", s.key, v);
            parse_settings(line, problems);
            CHECK(problems.size() == 1);
        }
    }
}

static void numeric_pad() {
    const FieldSpec& poll = *field_spec("poll_s");  // 1 - 60
    NumberPad p(poll);
    CHECK(!p.valid());
    type(p, "45");
    double v = 0;
    CHECK(p.text() == "45" && p.valid(&v) && v == 45.0);
    type(p, "0");  // 450: out of the range
    CHECK(!p.valid());
    p.press(PadKey::Back);
    CHECK(p.valid(&v) && v == 45.0);
    p.press(PadKey::Clear);
    CHECK(p.text().empty() && !p.valid());
    // no leading zero, no point in a whole number, no minus where nothing is negative
    type(p, "007");
    CHECK(p.text() == "7");
    p.press(PadKey::Point);
    p.press(PadKey::Minus);
    CHECK(p.text() == "7");
    // the lowest value is not valid when it is below the range
    NumberPad zero(poll);
    type(zero, "0");
    CHECK(zero.text() == "0" && !zero.valid());
    // the arrows move by one and stay in the range
    NumberPad arrows(poll, "59");
    arrows.press(PadKey::Up);
    arrows.press(PadKey::Up);
    CHECK(arrows.text() == "60");
    NumberPad low(poll, "2");
    low.press(PadKey::Down);
    low.press(PadKey::Down);
    CHECK(low.text() == "1");
    NumberPad empty(poll);
    empty.press(PadKey::Up);
    CHECK(empty.text() == "2");  // from the lowest value
    // the port: up to 65535, nothing more
    NumberPad port(*field_spec("server_port"));
    type(port, "65535");
    CHECK(port.valid(&v) && v == 65535.0);
    port.press(PadKey::D1);
    CHECK(!port.valid());

    // a setting with decimals and a range below zero (a made-up one, to try the rules of the pad)
    FieldSpec offset{"offset", FieldKind::Number, -50, 50, 2};
    NumberPad o(offset);
    type(o, "-1.256");
    CHECK(o.text() == "-1.25" && o.valid(&v) && std::fabs(v + 1.25) < 1e-9);
    o.press(PadKey::Point);
    CHECK(o.text() == "-1.25");  // one point only
    NumberPad dot(offset);
    dot.press(PadKey::Point);
    CHECK(dot.text() == "0.");
    CHECK(!dot.valid());  // it does not end in a point
    type(dot, "5");
    CHECK(dot.valid(&v) && std::fabs(v - 0.5) < 1e-9);
    NumberPad minus(offset);
    minus.press(PadKey::Minus);
    CHECK(minus.text() == "-" && !minus.valid());
    minus.press(PadKey::Point);
    CHECK(minus.text() == "-0.");
    NumberPad big(offset, "49.99");
    big.press(PadKey::Up);
    CHECK(big.text() == "50.00");
    big.press(PadKey::Up);
    CHECK(big.text() == "50.00");
}

static void full_keyboard() {
    const FieldSpec& host = *field_spec("server_host");
    TextEntry t(host, Lang::En);
    CHECK(t.layer() == Layer::Lower);
    t.press(*find(t.keys(), "h"));
    t.press(*find(t.keys(), "e"));
    // the shift is for one letter
    t.press(*find_do(t.keys(), Key::Do::Shift));
    CHECK(t.layer() == Layer::Upper);
    t.press(*find(t.keys(), "L"));
    CHECK(t.layer() == Layer::Lower && t.text() == "heL");
    // the symbols and back to the letters
    t.press(*find_do(t.keys(), Key::Do::Symbols));
    CHECK(t.layer() == Layer::Symbols);
    t.press(*find(t.keys(), "."));
    t.press(*find(t.keys(), "5"));
    t.press(*find_do(t.keys(), Key::Do::Letters));
    CHECK(t.layer() == Layer::Lower && t.text() == "heL.5");
    t.press(*find_do(t.keys(), Key::Do::Space));
    CHECK(t.text() == "heL.5 ");
    t.press(*find_do(t.keys(), Key::Do::Back));
    t.press(*find_do(t.keys(), Key::Do::Back));
    CHECK(t.text() == "heL.");

    // a text stays inside the length of its setting
    const FieldSpec& machine = *field_spec("server_machine");  // 31
    TextEntry m(machine, Lang::En);
    for (int i = 0; i < 40; ++i) m.press(*find(m.keys(), "a"));
    CHECK(m.text().size() == 31);

    // a token: empty (cleared) or 8 to 64 characters, and the keyboard shows it is a secret by its kind
    const FieldSpec& token = *field_spec("token");
    TextEntry k(token, Lang::En);
    CHECK(k.valid());
    for (int i = 0; i < 5; ++i) k.press(*find(k.keys(), "x"));
    CHECK(!k.valid());
    for (int i = 0; i < 3; ++i) k.press(*find(k.keys(), "x"));
    CHECK(k.valid() && k.text() == "xxxxxxxx");
    CHECK(token.kind == FieldKind::Secret);

    // the keys of a language: German swaps y and z and has the umlauts, French is AZERTY, Spanish has the n with tilde
    CHECK(letter_rows(Lang::De)[0].substr(0, 6) == "qwertz");
    CHECK(letter_rows(Lang::Fr)[0].substr(0, 6) == "azerty");
    TextEntry de(machine, Lang::De);
    CHECK(find(de.keys(), "ü") && find(de.keys(), "ß") && !find(de.keys(), "ñ"));
    de.press(*find_do(de.keys(), Key::Do::Shift));
    CHECK(find(de.keys(), "Ü") != nullptr);
    de.press(*find(de.keys(), "Ü"));
    CHECK(de.text() == "Ü");
    // a letter with an accent is one character for the length and for the back key
    de.press(*find(de.keys(), "ö"));
    CHECK(de.text() == "Üö");
    de.press(*find_do(de.keys(), Key::Do::Back));
    CHECK(de.text() == "Ü");
    CHECK(find(TextEntry(machine, Lang::Es).keys(), "ñ") != nullptr);
    CHECK(find(TextEntry(machine, Lang::Fr).keys(), "é") != nullptr && find(TextEntry(machine, Lang::Fr).keys(), "ç") != nullptr);
    CHECK(find(TextEntry(machine, Lang::Pt).keys(), "ã") != nullptr && find(TextEntry(machine, Lang::Pt).keys(), "ç") != nullptr);
    CHECK(find(TextEntry(machine, Lang::It).keys(), "ì") != nullptr);
    // Chinese and Japanese type in letters
    CHECK(find(TextEntry(machine, Lang::Zh).keys(), "q") != nullptr && find(TextEntry(machine, Lang::Ja).keys(), "m") != nullptr);

    // every language has the same row of digits, a shift, a back key, a space and a done key
    for (std::size_t i = 0; i < kLangCount; ++i) {
        TextEntry e(machine, static_cast<Lang>(i));
        auto rows = e.keys();
        CHECK(rows[0].size() == 10 && rows[0][0].label == "1");
        CHECK(find_do(rows, Key::Do::Shift) && find_do(rows, Key::Do::Back) && find_do(rows, Key::Do::Space) && find_do(rows, Key::Do::Done) && find_do(rows, Key::Do::Symbols));
        // the symbols have what an address needs
        e.press(*find_do(rows, Key::Do::Symbols));
        auto sym = e.keys();
        CHECK(find(sym, ":") && find(sym, "/") && find(sym, ".") && find(sym, "-") && find(sym, "@") && find_do(sym, Key::Do::Letters));
    }
}

int main() {
    every_setting_has_its_keyboard();
    numeric_pad();
    full_keyboard();
    std::printf("%d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
