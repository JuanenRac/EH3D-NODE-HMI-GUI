// EH3D-NODE-HMI-GUI - core/job_tools.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// What a drilling or routing file of the card holds, read before it is sent: the tools it uses (and how many holes or cuts each one has), so the screen
// can show a summary of the file when it is chosen and let the operator switch tools off. A tool that is off is left out of the job by the server
// (`skip_tools` of the request of the job); nothing is changed in the file.
#pragma once

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace hmi {

/// One tool of the file: its number (the pen number of a routing file), its diameter in mm when the file says it (0 when it does not), and how many holes or cuts it has.
struct ToolInfo {
    int number = 0;
    double diameter_mm = 0;
    int count = 0;
    int use = 1 << 30;  // the place of the tool among those the work uses, in the order of its first hole or cut (the order the machine works in); large while it has none
};

/// The box that the coordinates of a file span, in mm (not known when the file holds none).
struct Box {
    bool known = false;
    double min_x = 0, min_y = 0, max_x = 0, max_y = 0;
    void add(double x, double y) {
        if (!known) {
            known = true;
            min_x = max_x = x;
            min_y = max_y = y;
            return;
        }
        if (x < min_x) min_x = x;
        if (x > max_x) max_x = x;
        if (y < min_y) min_y = y;
        if (y > max_y) max_y = y;
    }
    double width() const { return known ? max_x - min_x : 0; }
    double depth() const { return known ? max_y - min_y : 0; }
};

struct FileSummary {
    bool route = false;
    Box box;                      // where the holes or the cuts lie, as the file says it (a size to compare with the table)
    std::vector<ToolInfo> tools;  // by number
    int total = 0;                // the holes or the cuts of every tool
    int uses = 0;                 // how many tools have had their first hole or cut so far (gives `ToolInfo::use`)
    bool format_unknown = false;  // a drilling file whose numbers cannot be read without supposing (neither the file nor a choice says how): the box is not known
};

namespace detail {

inline ToolInfo& tool_entry(FileSummary& s, int number) {
    for (ToolInfo& t : s.tools)
        if (t.number == number) return t;
    s.tools.push_back(ToolInfo{});
    s.tools.back().number = number;
    return s.tools.back();
}

/// One more hole or cut for a tool; the first one fixes the place of the tool in the order of use.
inline void count_step(FileSummary& s, ToolInfo& t) {
    if (t.count == 0) t.use = s.uses++;
    ++t.count;
    ++s.total;
}

inline bool is_digit(char c) { return c >= '0' && c <= '9'; }

/// The most digits on either side of the point of a number format.
constexpr int kMaxFormatDigits = 6;

/// How the coordinates of a drilling file are written, as far as it is said: by the header of the file, completed with the format somebody chose (`drill_format`).
/// The rules are those the suite reads the same file with: what the file declares wins, and nothing is supposed.
struct DrillNumbers {
    int unit = 0;                    // how the numbers are measured: 0 not said, 1 millimetres, 2 inches
    int unit_said = 0;               // the unit the header names (an `INCH` with nothing else names it and still leaves the numbers open)
    int integer = 0, decimal = 0;    // the digits before and after the point; 0 not said
    bool trailing_left_out = false;  // `LZ`: the leading zeros are written and the trailing ones left out
    bool zeros_said = false;         // the header says `LZ` or `TZ`
    bool plain_inch = false;         // `INCH` or `M72` with no option

    /// Takes a header line (upper case, without blanks): `INCH,LZ`, `METRIC,TZ,000.000`, `M71`, `M72` or `;FILE_FORMAT=2:4`; false when it is none of them.
    bool header(const std::string& line) {
        if (line.compare(0, 13, ";FILE_FORMAT=") == 0) {
            const std::size_t colon = line.find(':', 13);
            if (colon != std::string::npos) {
                const int i = std::atoi(line.c_str() + 13), d = std::atoi(line.c_str() + colon + 1);
                if (i >= 1 && i <= kMaxFormatDigits && d >= 1 && d <= kMaxFormatDigits) {
                    integer = i;
                    decimal = d;
                    if (plain_inch) unit = 2;
                }
            }
            return true;
        }
        std::size_t comma = line.find(',');
        const std::string head = line.substr(0, comma);
        if (head == "METRIC" || head == "M71") unit = 1;
        else if (head == "INCH" || head == "M72") {
            unit = 2;
            plain_inch = true;
        } else {
            return false;
        }
        unit_said = unit;
        bool options = false;
        while (comma != std::string::npos) {
            options = true;
            const std::size_t from = comma + 1;
            comma = line.find(',', from);
            const std::string part = line.substr(from, comma == std::string::npos ? std::string::npos : comma - from);
            if (part == "LZ" || part == "TZ") {
                trailing_left_out = part == "LZ";
                zeros_said = true;
            } else {
                const std::size_t point = part.find('.');
                if (point != std::string::npos && part.find_first_not_of("0123456789.") == std::string::npos) {
                    integer = static_cast<int>(point);
                    decimal = static_cast<int>(part.size() - point - 1);
                }
            }
        }
        if (options) plain_inch = false;
        if (plain_inch) unit = 0;  // no options: other programs read it in more than one way
        return true;
    }
};

/// A format somebody chose for a file that does not say it (the names of `drill_format`: `mils`, `inch-2.4-tz`, `metric-3.3-lz`...).
struct ChosenFormat {
    bool set = false, mils = false, metric = false, trailing_left_out = false;
    int integer = 3, decimal = 3;
};

/// The format with this name; not set for an empty text or any other.
inline ChosenFormat chosen_format(const std::string& name) {
    ChosenFormat c;
    if (name == "mils") {
        c.set = c.mils = true;
        return c;
    }
    const bool metric = name.compare(0, 7, "metric-") == 0;
    if (!metric && name.compare(0, 5, "inch-") != 0) return c;
    const std::string rest = name.substr(metric ? 7 : 5);  // "2.4-tz"
    if (rest.size() != 6 || !is_digit(rest[0]) || rest[1] != '.' || !is_digit(rest[2]) || rest[3] != '-') return c;
    const std::string zeros = rest.substr(4);
    c.integer = rest[0] - '0';
    c.decimal = rest[2] - '0';
    if (c.integer < 1 || c.integer > kMaxFormatDigits || c.decimal < 1 || c.decimal > kMaxFormatDigits || (zeros != "tz" && zeros != "lz")) return c;
    c.metric = metric;
    c.trailing_left_out = zeros == "lz";
    c.set = true;
    return c;
}

/// What the header leaves open taken from the choice; false when the choice cannot be what the header says.
inline bool complete_numbers(DrillNumbers& n, const ChosenFormat& c) {
    const int unit = c.metric ? 1 : 2;
    if (n.unit_said != 0 && n.unit_said != unit) return false;
    if (c.mils && ((n.decimal != 0 && n.decimal != 3) || (n.zeros_said && n.trailing_left_out))) return false;
    n.unit = unit;
    if (n.decimal == 0) {
        n.integer = c.integer;
        n.decimal = c.decimal;
    }
    if (!n.zeros_said) n.trailing_left_out = c.trailing_left_out;
    return true;
}

/// The text of the number after an axis letter of a drilling line (`X02050Y01538`, `X1.25Y-3.5`); empty when the letter is not there or has no digits.
inline std::string axis_text(const std::string& line, char axis) {
    const std::size_t p = line.find(axis);
    if (p == std::string::npos) return "";
    std::size_t q = p + 1;
    const std::size_t start = q;
    if (q < line.size() && (line[q] == '-' || line[q] == '+')) ++q;
    std::size_t digits = 0;
    while (q < line.size() && (is_digit(line[q]) || line[q] == '.')) {
        if (line[q] != '.') ++digits;
        ++q;
    }
    return digits == 0 ? std::string() : line.substr(start, q - start);
}

/// Whether reading this number takes something that is not said: the unit, the digits, or the side of the zeros of a number that does not fill the digits.
/// With the unit known, a number with a point, and a zero, mean the same whatever the format.
inline bool needs_supposing(const std::string& number, const DrillNumbers& n) {
    if (n.unit == 0) return true;
    if (number.find('.') != std::string::npos || number.find_first_of("123456789") == std::string::npos) return false;
    if (n.decimal == 0) return true;
    const std::size_t written = number.size() - (number[0] == '-' || number[0] == '+' ? 1 : 0);
    return !n.zeros_said && written < static_cast<std::size_t>(n.integer + n.decimal);
}

/// The number in millimetres, for a format that says all it takes.
inline double axis_mm(const std::string& number, const DrillNumbers& n) {
    double value = std::strtod(number.c_str(), nullptr);
    if (number.find('.') == std::string::npos) {
        const bool negative = number[0] == '-';
        std::string digits = number.substr(number[0] == '-' || number[0] == '+' ? 1 : 0);
        const int integer = n.decimal != 0 ? n.integer : (n.unit == 1 ? 3 : 2);
        const int decimal = n.decimal != 0 ? n.decimal : (n.unit == 1 ? 3 : 4);
        if (n.trailing_left_out)
            while (static_cast<int>(digits.size()) < std::min(integer + decimal, 2 * kMaxFormatDigits)) digits += '0';
        value = std::strtod(digits.c_str(), nullptr);
        for (int i = 0; i < decimal; ++i) value /= 10.0;
        if (negative) value = -value;
    }
    return value * (n.unit == 2 ? 25.4 : 1.0);
}

inline std::string upper_trim(const std::string& line) {
    std::string out;
    for (char c : line) {
        if (c == ' ' || c == '\t' || c == '\r') continue;
        out += (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
    }
    return out;
}

}  // namespace detail

namespace detail {

/// One reading of a drilling file. `chosen` completes what the header leaves open (not set: only what the file says). `supposed` tells that a coordinate needed
/// something nobody said, and `misfit` that the chosen format cannot be what the header says; in both cases the box of this reading is not to be believed.
inline FileSummary read_drill(const std::string& text, const ChosenFormat& chosen, bool& supposed, bool& misfit) {
    FileSummary s;
    DrillNumbers numbers;
    int current = -1;
    double x = 0, y = 0;
    bool has_x = false, has_y = false;  // a coordinate holds until the next one is written
    std::vector<std::pair<int, double>> diameters;  // as written: their unit is known at the end
    std::size_t pos = 0;
    supposed = misfit = false;
    while (pos <= text.size() && s.tools.size() < 64) {
        std::size_t end = text.find('\n', pos);
        if (end == std::string::npos) end = text.size();
        const std::string line = upper_trim(text.substr(pos, end - pos));
        pos = end + 1;
        if (line.empty() || numbers.header(line) || line[0] == ';') continue;
        if (line.rfind("M30", 0) == 0) break;
        if (line[0] == 'T' && line.size() > 1 && is_digit(line[1])) {
            std::size_t i = 1;
            while (i < line.size() && is_digit(line[i])) ++i;
            const int number = std::atoi(line.substr(1, i - 1).c_str());
            const std::size_t c = line.find('C', i);
            if (c != std::string::npos && c + 1 < line.size()) {
                const double d = std::strtod(line.c_str() + c + 1, nullptr);  // a declaration: T1C0.8
                if (number > 0 && d > 0) {
                    tool_entry(s, number);
                    diameters.emplace_back(number, d);
                }
            } else {
                current = number > 0 ? number : -1;  // a choice: T1 (T0 puts the tool away)
                if (current > 0) tool_entry(s, current);
            }
        } else if ((line[0] == 'X' || line[0] == 'Y') && current > 0) {
            count_step(s, tool_entry(s, current));
            DrillNumbers used = numbers;
            if (chosen.set && !complete_numbers(used, chosen)) {
                misfit = true;
                continue;
            }
            const std::string tx = axis_text(line, 'X'), ty = axis_text(line, 'Y');
            // with a choice nothing more is supposed: it says what the header left open, and the side of the zeros is its own
            if (!chosen.set && ((!tx.empty() && needs_supposing(tx, used)) || (!ty.empty() && needs_supposing(ty, used)))) supposed = true;
            if (!tx.empty()) { x = axis_mm(tx, used); has_x = true; }
            if (!ty.empty()) { y = axis_mm(ty, used); has_y = true; }
            if (has_x && has_y) s.box.add(x, y);
        }
    }
    // a diameter has the unit the file names, else that of a choice that fits the file; with neither it is not known
    const int unit = numbers.unit_said != 0 ? numbers.unit_said : (chosen.set && !misfit ? (chosen.metric ? 1 : 2) : 0);
    for (const auto& d : diameters)
        if (unit != 0) tool_entry(s, d.first).diameter_mm = d.second * (unit == 2 ? 25.4 : 1.0);
    s.route = false;
    std::sort(s.tools.begin(), s.tools.end(), [](const ToolInfo& a, const ToolInfo& b) { return a.number < b.number; });
    return s;
}

}  // namespace detail

/// The summary of a drilling file (Excellon: `T1C0.8` declares a tool, a line `T1` chooses it and each `X..Y..` line is a hole). `drill_format` is the format chosen
/// for a file that does not say how its numbers are written (the setting of the same name; empty: none). A file that says it is read as it says. When neither says
/// enough, the holes and the tools are counted but the box (and a diameter with no unit) is not known: nothing is supposed.
inline FileSummary summarize_drill(const std::string& text, const std::string& drill_format = {}) {
    bool supposed = false, misfit = false;
    FileSummary s = detail::read_drill(text, detail::ChosenFormat{}, supposed, misfit);
    if (!supposed) return s;
    const detail::ChosenFormat chosen = detail::chosen_format(drill_format);
    if (chosen.set) s = detail::read_drill(text, chosen, supposed, misfit);
    if (supposed || misfit) {
        s.box = Box{};
        s.format_unknown = true;
    }
    return s;
}

/// The summary of a routing file (HP-GL: `SP2` takes pen 2, each `PD` is a cut with it).
inline FileSummary summarize_route(const std::string& text) {
    FileSummary s;
    s.route = true;
    int pen = 0;
    std::string word;
    for (std::size_t i = 0; i <= text.size() && s.tools.size() < 64; ++i) {
        const char c = i < text.size() ? text[i] : ';';
        if (c != ';' && c != '\n') {
            word += c;
            continue;
        }
        const std::string st = detail::upper_trim(word);
        word.clear();
        if (st.size() >= 2 && st[0] == 'S' && st[1] == 'P') {
            pen = std::atoi(st.c_str() + 2);
            if (pen > 0) detail::tool_entry(s, pen);
        } else if (st.size() >= 2 && st[0] == 'P' && st[1] == 'D' && pen > 0) {
            detail::count_step(s, detail::tool_entry(s, pen));
        }
        // the places of a move: PU, PD and PA are followed by pairs of numbers in plotter steps (40 to the millimetre)
        if (st.size() > 2 && st[0] == 'P' && (st[1] == 'U' || st[1] == 'D' || st[1] == 'A')) {
            std::vector<double> n;
            std::size_t k = 2;
            while (k < st.size()) {
                char* end = nullptr;
                const double v = std::strtod(st.c_str() + k, &end);
                if (end == st.c_str() + k) break;
                n.push_back(v);
                k = static_cast<std::size_t>(end - st.c_str());
                if (k < st.size() && st[k] == ',') ++k;
            }
            for (std::size_t m = 0; m + 1 < n.size(); m += 2) s.box.add(n[m] / 40.0, n[m + 1] / 40.0);
        }
    }
    std::sort(s.tools.begin(), s.tools.end(), [](const ToolInfo& a, const ToolInfo& b) { return a.number < b.number; });
    return s;
}

/// How a board lies on the table. The board may be turned a quarter, so a warning is only for a board that fits in neither way.
enum class FitKind { Unknown, Fits, FitsTurned, TooBig };
struct Fit {
    FitKind kind = FitKind::Unknown;
    double over_w = 0;  // TooBig: by how much it is wider than the table, in the way that goes over least
    double over_d = 0;  // TooBig: by how much it is deeper
};

/// Compares the box of a file with the table (width, depth, mm). A size that is not known (a file with no coordinates, a table of zero) gives `Unknown`.
inline Fit fit_on_table(const Box& board, double table_w, double table_d) {
    constexpr double kEps = 0.01;
    Fit f;
    if (!board.known || !(board.width() > 0 || board.depth() > 0) || !(table_w > 0) || !(table_d > 0)) return f;
    const double bw = board.width(), bd = board.depth();
    if (bw <= table_w + kEps && bd <= table_d + kEps) { f.kind = FitKind::Fits; return f; }
    if (bd <= table_w + kEps && bw <= table_d + kEps) { f.kind = FitKind::FitsTurned; return f; }
    auto pos = [](double v) { return v > 0 ? v : 0.0; };
    const double aw = pos(bw - table_w), ad = pos(bd - table_d);
    const double tw = pos(bd - table_w), td = pos(bw - table_d);
    f.kind = FitKind::TooBig;
    if (tw + td < aw + ad) { f.over_w = tw; f.over_d = td; }
    else { f.over_w = aw; f.over_d = ad; }
    return f;
}

/// The tools that are switched off for the job that is about to be sent. A new file starts with every tool on.
class ToolSwitch {
public:
    void reset(const FileSummary& s) {
        summary_ = s;
        off_.clear();
    }
    /// The same file read again (the chosen number format changed): the tools switched off stay off.
    void reread(const FileSummary& s) { summary_ = s; }
    void clear() { reset(FileSummary{}); }

    const FileSummary& summary() const { return summary_; }
    const std::vector<int>& off() const { return off_; }

    bool is_off(int number) const { return std::find(off_.begin(), off_.end(), number) != off_.end(); }

    /// Switches a tool of the file off or on; false when the file has no such tool.
    bool toggle(int number) {
        bool known = false;
        for (const ToolInfo& t : summary_.tools) known = known || t.number == number;
        if (!known) return false;
        auto it = std::find(off_.begin(), off_.end(), number);
        if (it != off_.end()) off_.erase(it);
        else off_.push_back(number);
        std::sort(off_.begin(), off_.end());
        return true;
    }

    /// The holes or cuts that stay in the job.
    int remaining() const {
        int n = 0;
        for (const ToolInfo& t : summary_.tools)
            if (!is_off(t.number)) n += t.count;
        return n;
    }

    /// True when tools were switched off and what is left of the file is nothing, so there is nothing to do.
    bool nothing_left() const { return !off_.empty() && remaining() == 0; }

private:
    FileSummary summary_;
    std::vector<int> off_;
};

}  // namespace hmi
