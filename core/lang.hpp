// EH3D-NODE-HMI-GUI - core/lang.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The languages of the screen. The first eight (en, es, it, de, fr, pt, zh, ja) have their texts in the tables of ui_text.hpp and quality.hpp; the other
// twenty-nine have them in the files of the folder `locales` (compiled into locales_gen.hpp), and a text that one of them lacks is shown in English (in Portuguese of
// Portugal for `pt-br` and in French of France for `fr-ca`, the two variants).
#pragma once

#include <array>
#include <cstddef>
#include <string>

namespace hmi {

enum class Lang : unsigned {
    En = 0, Es, It, De, Fr, Pt, Zh, Ja,  // the eight of the tables
    Bg, Hr, Cs, Da, Nl, Et, Fi, El, Hu, Ga, Lv, Lt, Mt, Pl, Ro, Sk, Sl, Sv, No, Is, Uk, Ru, Sr, Sq, Ca, Gl, Eu,
    PtBr, FrCa  // variants
};

constexpr std::size_t kBaseLangCount = 8;
constexpr std::size_t kLangCount = 37;

/// The code of a language as the settings file writes it (`pt` is the Portuguese of Portugal, `pt-br` the one of Brazil).
inline const char* lang_code(Lang l) {
    static const std::array<const char*, kLangCount> codes = {"en", "es", "it", "de", "fr", "pt", "zh", "ja", "bg", "hr", "cs", "da", "nl", "et", "fi", "el", "hu", "ga", "lv", "lt", "mt", "pl", "ro",
                                                              "sk", "sl", "sv", "no", "is", "uk", "ru", "sr", "sq", "ca", "gl", "eu", "pt-br", "fr-ca"};
    return codes[static_cast<std::size_t>(l)];
}

/// The language of a code; false when the code is none of the thirty-seven.
inline bool lang_from_code(const std::string& code, Lang& out) {
    for (std::size_t i = 0; i < kLangCount; ++i) {
        if (code == lang_code(static_cast<Lang>(i))) {
            out = static_cast<Lang>(i);
            return true;
        }
    }
    return false;
}

/// The language whose table a language falls back to when it lacks a text: itself for the eight, Portuguese / French for the two variants, English for the rest.
inline Lang lang_fallback(Lang l) {
    if (static_cast<std::size_t>(l) < kBaseLangCount) return l;
    if (l == Lang::PtBr) return Lang::Pt;
    if (l == Lang::FrCa) return Lang::Fr;
    return Lang::En;
}

/// The text of a row of eight for a language (the fallback language for the other twenty-nine).
using Row = std::array<const char*, kBaseLangCount>;

inline const char* pick(const Row& row, Lang l) { return row[static_cast<std::size_t>(lang_fallback(l))]; }

}  // namespace hmi
