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
#include <vector>

namespace hmi {

/// One tool of the file: its number (the pen number of a routing file), its diameter in mm when the file says it (0 when it does not), and how many holes or cuts it has.
struct ToolInfo {
    int number = 0;
    double diameter_mm = 0;
    int count = 0;
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
};

namespace detail {

inline ToolInfo& tool_entry(FileSummary& s, int number) {
    for (ToolInfo& t : s.tools)
        if (t.number == number) return t;
    s.tools.push_back(ToolInfo{});
    s.tools.back().number = number;
    return s.tools.back();
}

inline bool is_digit(char c) { return c >= '0' && c <= '9'; }

/// The number after an axis letter of a drilling line (`X02050Y01538`, `X1.25Y-3.5`). A number with a point is as written; one without is taken in thousandths, as the
/// files of the Bungard programs and the usual 2.3 and 3.3 formats write them. False when the letter is not there or has no digits.
inline bool axis_value(const std::string& line, char axis, double& out) {
    const std::size_t p = line.find(axis);
    if (p == std::string::npos) return false;
    std::size_t q = p + 1;
    const std::size_t start = q;
    if (q < line.size() && (line[q] == '-' || line[q] == '+')) ++q;
    bool point = false;
    std::size_t digits = 0;
    while (q < line.size() && (is_digit(line[q]) || line[q] == '.')) {
        if (line[q] == '.') point = true;
        else ++digits;
        ++q;
    }
    if (digits == 0) return false;
    out = std::strtod(line.substr(start, q - start).c_str(), nullptr);
    if (!point) out /= 1000.0;
    return true;
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

/// The summary of a drilling file (Excellon: `T1C0.8` declares a tool, a line `T1` chooses it and each `X..Y..` line is a hole).
inline FileSummary summarize_drill(const std::string& text) {
    FileSummary s;
    bool inch = false;
    int current = -1;
    double x = 0, y = 0;
    bool has_x = false, has_y = false;  // a coordinate holds until the next one is written
    std::size_t pos = 0;
    while (pos <= text.size() && s.tools.size() < 64) {
        std::size_t end = text.find('\n', pos);
        if (end == std::string::npos) end = text.size();
        const std::string line = detail::upper_trim(text.substr(pos, end - pos));
        pos = end + 1;
        if (line.empty() || line[0] == ';') continue;
        if (line.compare(0, 4, "INCH") == 0 || line.find(",INCH") != std::string::npos) inch = true;
        if (line.compare(0, 6, "METRIC") == 0 || line.find(",METRIC") != std::string::npos) inch = false;
        if (line.rfind("M30", 0) == 0) break;
        if (line[0] == 'T' && line.size() > 1 && detail::is_digit(line[1])) {
            std::size_t i = 1;
            while (i < line.size() && detail::is_digit(line[i])) ++i;
            const int number = std::atoi(line.substr(1, i - 1).c_str());
            const std::size_t c = line.find('C', i);
            if (c != std::string::npos && c + 1 < line.size()) {
                double d = std::strtod(line.c_str() + c + 1, nullptr);  // a declaration: T1C0.8
                if (inch) d *= 25.4;
                if (number > 0 && d > 0) detail::tool_entry(s, number).diameter_mm = d;
            } else {
                current = number > 0 ? number : -1;  // a choice: T1 (T0 puts the tool away)
                if (current > 0) detail::tool_entry(s, current);
            }
        } else if ((line[0] == 'X' || line[0] == 'Y') && current > 0) {
            ++detail::tool_entry(s, current).count;
            ++s.total;
            double v = 0;
            if (detail::axis_value(line, 'X', v)) { x = v; has_x = true; }
            if (detail::axis_value(line, 'Y', v)) { y = v; has_y = true; }
            if (has_x && has_y) s.box.add(x * (inch ? 25.4 : 1.0), y * (inch ? 25.4 : 1.0));
        }
    }
    s.route = false;
    std::sort(s.tools.begin(), s.tools.end(), [](const ToolInfo& a, const ToolInfo& b) { return a.number < b.number; });
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
            ++detail::tool_entry(s, pen).count;
            ++s.total;
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
