// EH3D-NODE-HMI-GUI - core/lang.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The eight languages of the screen, in the order every table of texts uses, and how a language is named in the settings.
#pragma once

#include <array>
#include <cstddef>
#include <string>

namespace hmi {

enum class Lang : unsigned { En = 0, Es, It, De, Fr, Pt, Zh, Ja };

constexpr std::size_t kLangCount = 8;

/// The code of a language as the settings file writes it (`pt` is the Portuguese of Portugal).
inline const char* lang_code(Lang l) {
    static const std::array<const char*, kLangCount> codes = {"en", "es", "it", "de", "fr", "pt", "zh", "ja"};
    return codes[static_cast<std::size_t>(l)];
}

/// The language of a code; false when the code is none of the eight.
inline bool lang_from_code(const std::string& code, Lang& out) {
    for (std::size_t i = 0; i < kLangCount; ++i) {
        if (code == lang_code(static_cast<Lang>(i))) {
            out = static_cast<Lang>(i);
            return true;
        }
    }
    return false;
}

/// The text of a row of eight for a language.
using Row = std::array<const char*, kLangCount>;

inline const char* pick(const Row& row, Lang l) { return row[static_cast<std::size_t>(l)]; }

}  // namespace hmi
