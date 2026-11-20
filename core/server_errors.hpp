// EH3D-NODE-HMI-GUI - core/server_errors.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// What the screen says when the server refuses: the server answers `{"error": "<code>", "detail": "..."}` and the screen shows the text of the code in the language of the operator
// (the table is core/server_errors_gen.hpp, in the eight languages of the tables; the other twenty-nine show the English one). A code the table does not have (a newer server) is shown
// as the code itself and its detail, so nothing is hidden. A text with a character that the fonts of the screen do not hold is not drawn (it would be boxes): the English text is
// shown instead until the fonts are built with the characters of the table.
#pragma once

#include <cstddef>
#include <cstring>
#include <string>

#include "font_ranges.hpp"
#include "lang.hpp"
#include "server_errors_gen.hpp"
#include "server_link.hpp"
#include "status.hpp"

namespace hmi {

/// The row of a code of the server, or null when the table does not have it. The table is sorted by code, so it is searched by halves.
inline const ServerErrorRow* server_error_row(const std::string& code) {
    std::size_t lo = 0, hi = kServerErrorCount;
    while (lo < hi) {
        const std::size_t mid = lo + (hi - lo) / 2;
        const int c = std::strcmp(kServerErrors[mid].code, code.c_str());
        if (c == 0) return &kServerErrors[mid];
        if (c < 0) lo = mid + 1; else hi = mid;
    }
    return nullptr;
}

/// Whether every character of a UTF-8 text is in the fonts of the screen (`kFontRanges`); a malformed sequence is not.
inline bool text_in_font(const char* utf8) {
    const unsigned char* p = reinterpret_cast<const unsigned char*>(utf8);
    while (*p != 0) {
        unsigned cp = 0;
        int extra = 0;
        if (*p < 0x80) { cp = *p; extra = 0; }
        else if ((*p & 0xE0) == 0xC0) { cp = *p & 0x1Fu; extra = 1; }
        else if ((*p & 0xF0) == 0xE0) { cp = *p & 0x0Fu; extra = 2; }
        else if ((*p & 0xF8) == 0xF0) { cp = *p & 0x07u; extra = 3; }
        else return false;
        ++p;
        for (int i = 0; i < extra; ++i, ++p) {
            if ((*p & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (*p & 0x3Fu);
        }
        if (cp < 0x20) continue;
        bool found = false;
        for (std::size_t i = 0; i < kFontRangeCount && !found; ++i) found = kFontRanges[i].lo <= cp && cp <= kFontRanges[i].hi;
        if (!found) return false;
    }
    return true;
}

/// The text of a code in the language of the operator; empty when the table does not have the code. When the text of the language cannot be drawn with the fonts of the screen the
/// English one is given.
inline std::string server_error_text(const std::string& code, Lang l) {
    const ServerErrorRow* row = server_error_row(code);
    if (row == nullptr) return "";
    const char* wanted = pick(row->texts, l);
    return text_in_font(wanted) ? wanted : row->texts[0];
}

namespace detail {
/// Whether a text is shaped like a code of the server (lower case letters, digits and underscores).
inline bool looks_like_code(const std::string& s) {
    if (s.empty() || s.size() > 48) return false;
    for (char c : s)
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) return false;
    return true;
}
}  // namespace detail

/// The refusal of the server as one line for the operator: the text of its code in the language of the operator; a code the table does not have is `code: detail` (or the code
/// alone); an answer that is not a refusal of the server is `HTTP <status>`.
inline std::string error_note(const Response& r, Lang l) {
    Json j;
    if (parse_json(r.body, j) && j.type == Json::Type::Object) {
        const std::string code = j.str("error");
        if (!code.empty()) {
            const std::string known = server_error_text(code, l);
            if (!known.empty()) return known;
        }
    }
    return error_line(r);
}

/// A reason the screen shows (see `reason_text`) that may also be `code` or `code: detail` of the server, as `error_line` writes it: the text of the code in the language of the
/// operator when the table has it, else what `reason_text` gives.
inline std::string reason_note(const std::string& why, Lang l) {
    const std::string plain = reason_text(why, l);
    if (plain != why) return plain;
    const std::size_t cut = why.find(": ");
    const std::string code = cut == std::string::npos ? why : why.substr(0, cut);
    if (!detail::looks_like_code(code)) return why;
    const std::string known = server_error_text(code, l);
    return known.empty() ? why : known;
}

}  // namespace hmi
