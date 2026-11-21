// EH3D-NODE-HMI-GUI - core/lang.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The languages of the screen: the eight of the project (en, es, it, de, fr, pt, zh, ja), which have their texts in the tables of ui_text.hpp and quality.hpp.
// `pt` is the Portuguese of Portugal. The codes `pt-br` and `fr-ca` of the former variants are still read, as aliases of `pt` and `fr`; the other languages
// that the screen once had are not read any more (a settings file that names one keeps the language it had).
#pragma once

#include <array>
#include <cstddef>
#include <string>

namespace hmi {

enum class Lang : unsigned {
    En = 0, Es, It, De, Fr, Pt, Zh, Ja  // the eight of the tables
};

constexpr std::size_t kBaseLangCount = 8;
constexpr std::size_t kLangCount = 8;

/// The code of a language as the settings file writes it (`pt` is the Portuguese of Portugal).
inline const char* lang_code(Lang l) {
    static const std::array<const char*, kLangCount> codes = {"en", "es", "it", "de", "fr", "pt", "zh", "ja"};
    return codes[static_cast<std::size_t>(l)];
}

/// The language of a code; false when the code is none of the eight. `pt-br` is read as `pt` and `fr-ca` as `fr`.
inline bool lang_from_code(const std::string& code, Lang& out) {
    for (std::size_t i = 0; i < kLangCount; ++i) {
        if (code == lang_code(static_cast<Lang>(i))) {
            out = static_cast<Lang>(i);
            return true;
        }
    }
    if (code == "pt-br") {
        out = Lang::Pt;
        return true;
    }
    if (code == "fr-ca") {
        out = Lang::Fr;
        return true;
    }
    return false;
}

/// The language whose table a language falls back to when it lacks a text: itself (every language has its own table).
inline Lang lang_fallback(Lang l) { return l; }

/// The text of a row of eight for a language.
using Row = std::array<const char*, kBaseLangCount>;

inline const char* pick(const Row& row, Lang l) { return row[static_cast<std::size_t>(lang_fallback(l))]; }

}  // namespace hmi
