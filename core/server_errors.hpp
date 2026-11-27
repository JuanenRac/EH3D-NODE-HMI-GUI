// EH3D-NODE-HMI-GUI - core/server_errors.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// What the screen says when the server refuses: the server answers `{"error": "<code>", "detail": "..."}` and the screen shows the text of the code in the language of the operator
// (the table is core/server_errors_gen.hpp, in the eight languages of the tables; the languages the project does not have are not offered). A code the table does not have (a newer server) is shown
// as the code itself and its detail, so nothing is hidden. A text with a character that the fonts of the screen do not hold is not drawn (it would be boxes): the English text is
// shown instead. The fonts hold the characters of the table, and the host tests fail when a text of it cannot be drawn.
#pragma once

#include <cstddef>
#include <cstring>
#include <string>

#include "font_ranges.hpp"
#include "hmi_config.hpp"
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

/// Whether the server refused a drilling file because it does not declare how its numbers are written (or the chosen format does not fit it).
inline bool needs_drill_format(const std::string& code, const std::string& detail) { return code == "bad_ncd" && detail.find("EH3D-FORMATS-014") != std::string::npos; }

/// What the screen says then, in the eight languages of the tables: where to choose the format.
inline std::string drill_format_note(Lang l) {
    static const char* const texts[8] = {
        "The drill file does not say its number format: choose it in the settings of the screen",
        "El taladrado no dice su formato num\u00e9rico: el\u00edjalo en los ajustes de la pantalla",
        "La foratura non dice il formato numerico: sceglierlo nelle impostazioni dello schermo",
        "Die Bohrdatei nennt ihr Zahlenformat nicht: in den Einstellungen des Bildschirms w\u00e4hlen",
        "Le per\u00e7age ne dit pas son format : choisissez-le dans les r\u00e9glages de l'\u00e9cran",
        "A fura\u00e7\u00e3o n\u00e3o diz o formato num\u00e9rico: escolha-o nas defini\u00e7\u00f5es do ecr\u00e3",
        "钻孔文件未声明数值格式：请在屏幕设置中选择",
        "ドリルファイルに数値の書式がありません。画面の設定で選んでください",
    };
    const char* wanted = pick(texts, l);
    return std::string(text_in_font(wanted) ? wanted : texts[0]) + " (EH3D-FORMATS-014)";
}

/// The title of the choice of the number format in the settings of the screen.
inline const char* drill_format_title(Lang l) {
    static const char* const texts[8] = {"Drill file format", "Formato de taladrado", "Formato foratura", "Bohrformat", "Format de per\u00e7age", "Formato de fura\u00e7\u00e3o", "钻孔格式", "ドリル書式"};
    const char* wanted = pick(texts, l);
    return text_in_font(wanted) ? wanted : texts[0];
}
/// The words of a choice (its place in `kDrillFormats`): the name of the format, or that none is chosen.
inline std::string drill_format_label(std::size_t place, Lang l) {
    if (place > 0 && place < kDrillFormatCount) return kDrillFormats[place];
    static const char* const texts[8] = {"Not chosen", "Sin elegir", "Non scelto", "Nicht gew\u00e4hlt", "Non choisi", "N\u00e3o escolhido", "未选择", "未選択"};
    const char* wanted = pick(texts, l);
    return text_in_font(wanted) ? wanted : texts[0];
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
        if (needs_drill_format(code, j.str("detail"))) return drill_format_note(l);
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
    if (needs_drill_format(code, why)) return drill_format_note(l);
    if (!detail::looks_like_code(code)) return why;
    const std::string known = server_error_text(code, l);
    return known.empty() ? why : known;
}

/// The note of a `resume-from` the server refused. For `bad_request` the server says why it does not go on by itself: that reason is kept after the words of the
/// screen (it is the server's own text). Any other refusal is told as `reason_note` tells it.
inline std::string resume_note(const std::string& why, Lang l) {
    const std::string refused = "bad_request: ";
    if (why.size() > refused.size() && why.compare(0, refused.size(), refused) == 0) return std::string(text(T::ResumeRefused, l)) + ": " + why.substr(refused.size());
    return reason_note(why, l);
}

}  // namespace hmi
