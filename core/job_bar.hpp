// EH3D-NODE-HMI-GUI - core/job_bar.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// What the machine page says about the work in hand, the way the desktop program's bar under the views does: the model of the machine and how it changes the tools, the tool that
// works now with its holes (cuts) done, the tool that comes next, how many tools are finished and the time left. It is worked out from the tools of the chosen file and from the
// progress the server reports, so it needs nothing the server does not already say; and the summary of the plan the operator sees before the start (the estimated time and what looks
// wrong in the work). Everything here is text built from numbers, so it is tested on a computer.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "job_tools.hpp"
#include "lang.hpp"
#include "server_link.hpp"
#include "ui_text.hpp"

namespace hmi {

/// True when the model the server holds for the machine is one with the tool changer (a word `ATC` in its name: `ccd-atc`, `CCD/2 ATC`); the models with the manual change
/// (`MTC`) and a model that says nothing are not.
inline bool changes_tools_itself(const std::string& model) {
    std::string word;
    for (std::size_t i = 0; i <= model.size(); ++i) {
        const char c = i < model.size() ? model[i] : ' ';
        const bool alnum = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        if (alnum) {
            word += static_cast<char>(c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c);
            continue;
        }
        if (word == "ATC") return true;
        word.clear();
    }
    return false;
}

/// The model as the screen writes it: capitals, with a blank where the name has a dash or an underscore (`ccd-mtc` is `CCD MTC`).
inline std::string model_label(const std::string& model) {
    std::string out;
    for (char c : model) {
        if (c == '-' || c == '_') out += ' ';
        else out += static_cast<char>(c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c);
    }
    return out;
}

/// A time as `m:ss`, or `h:mm:ss` from an hour.
inline std::string clock_text(long seconds) {
    if (seconds < 0) seconds = 0;
    char b[24];
    if (seconds >= 3600) std::snprintf(b, sizeof b, "%ld:%02ld:%02ld", seconds / 3600, seconds / 60 % 60, seconds % 60);
    else std::snprintf(b, sizeof b, "%ld:%02ld", seconds / 60, seconds % 60);
    return b;
}

/// The steps of one tool of the job: the holes it drills or the cuts it makes, and how many of them are done.
struct ToolProgress {
    int number = 0;
    double diameter_mm = 0;
    int total = 0;
    int done = 0;
    bool finished() const { return total > 0 && done >= total; }
};

/// Where the work is among its tools.
struct JobBar {
    bool known = false;               // the file of the job is here and has steps to do
    std::vector<ToolProgress> tools;  // the tools that still have steps in this job, in the order the machine works them
    int current = -1;                 // the index of the tool that works now (the first that is not finished), -1 when all are done
    int next = -1;                    // the index of the next tool that has steps to do after the current one, -1 when none
    int tools_done = 0;               // the tools finished
    int steps_done = 0, steps_total = 0;
    long seconds_left = -1;           // -1 while it cannot be told
};

/// The bar of a job. `sw` holds the tools of the chosen file and the ones switched off; `skipped` is the number of steps done before this job (a job that goes on after a stop);
/// `server_done` and `server_total` are the steps the server says are done and in all (0 when it says none); `estimate_s` is the time the server estimated for the whole job
/// (0 when it did not). The tools are taken in the order of their first hole or cut, which is the order the server works them in; the steps done are shared out over them in that
/// order, scaled when the server counts its steps in another way than the screen reads them (the paths of a routing file are not its pen-down commands).
inline JobBar make_job_bar(const ToolSwitch& sw, int skipped, int server_done, int server_total, double estimate_s) {
    JobBar bar;
    std::vector<const ToolInfo*> order;
    for (const ToolInfo& t : sw.summary().tools)
        if (t.count > 0 && !sw.is_off(t.number)) order.push_back(&t);
    std::sort(order.begin(), order.end(), [](const ToolInfo* a, const ToolInfo* b) { return a->use < b->use; });
    int to_drop = skipped > 0 ? skipped : 0;
    for (const ToolInfo* t : order) {
        int left = t->count;
        const int dropped = to_drop < left ? to_drop : left;
        to_drop -= dropped;
        left -= dropped;
        if (left > 0) bar.tools.push_back({t->number, t->diameter_mm, left, 0});
    }
    for (const ToolProgress& t : bar.tools) bar.steps_total += t.total;
    if (bar.steps_total == 0) return bar;
    bar.known = true;
    long pos = 0;
    if (server_total > 0 && server_done > 0) {
        const double f = std::min(1.0, static_cast<double>(server_done) / server_total);
        pos = std::lround(f * bar.steps_total);
    }
    bar.steps_done = static_cast<int>(pos);
    long rest = pos;
    for (ToolProgress& t : bar.tools) {
        const long here = rest < t.total ? rest : t.total;
        t.done = static_cast<int>(here);
        rest -= here;
    }
    for (std::size_t i = 0; i < bar.tools.size(); ++i) {
        if (bar.tools[i].finished()) {
            ++bar.tools_done;
        } else if (bar.current < 0) {
            bar.current = static_cast<int>(i);
        } else if (bar.next < 0) {
            bar.next = static_cast<int>(i);
        }
    }
    if (estimate_s > 0 && server_total > 0) {
        const double f = std::min(1.0, static_cast<double>(server_done) / server_total);
        bar.seconds_left = std::lround(estimate_s * (1.0 - f));
    }
    return bar;
}

namespace detail {
inline std::string tool_words(const ToolProgress& t, Lang l) {
    char b[96];
    if (t.diameter_mm > 0) std::snprintf(b, sizeof b, "%s %d  %.2f mm", text(T::Tool, l), t.number, t.diameter_mm);
    else std::snprintf(b, sizeof b, "%s %d", text(T::Tool, l), t.number);
    return b;
}
}  // namespace detail

/// The lines of the machine page: the model and how it changes the tools; at a tool change, which tool to put in; the tool at work with its steps; the next tool; the tools finished
/// and the time left. Empty lines are left out. `route` says whether the steps are cuts or holes; `tool_wait` and `wait_tool` say the job waits for the operator to change the tool.
inline std::string hud_text(const JobBar& bar, Lang l, const std::string& model, bool route, bool tool_wait, int wait_tool) {
    std::string out;
    auto line = [&](const std::string& s) {
        if (s.empty()) return;
        if (!out.empty()) out += '\n';
        out += s;
    };
    const bool atc = changes_tools_itself(model);
    if (!model.empty()) line(model_label(model) + (atc ? std::string("  |  ") + text(T::AtcAuto, l) : std::string()));
    if (tool_wait) {
        line(std::string(text(T::PutTool, l)) + (wait_tool > 0 ? " " + std::to_string(wait_tool) : std::string()));
        if (atc) line(text(T::AtcByHand, l));
    }
    if (bar.known && bar.current >= 0) {
        const ToolProgress& c = bar.tools[static_cast<std::size_t>(bar.current)];
        line(detail::tool_words(c, l) + "   " + std::to_string(c.done) + "/" + std::to_string(c.total) + " " + text(route ? T::Cuts : T::Holes, l));
        if (bar.next >= 0) line(std::string(text(T::Next, l)) + ": " + detail::tool_words(bar.tools[static_cast<std::size_t>(bar.next)], l));
    }
    if (bar.known) {
        std::string tail = std::string(text(T::Tools, l)) + " " + std::to_string(bar.tools_done) + "/" + std::to_string(bar.tools.size());
        if (bar.seconds_left >= 0) tail += std::string("   ") + text(T::TimeLeft, l) + " " + clock_text(bar.seconds_left);
        line(tail);
    }
    return out;
}

/// What the screen says about the plan the server made, before the start: the estimated time, and each thing that looks wrong in the work (a work that is tiny or far from the
/// corner, bigger than the table, a scale that is not 1, a plan that goes past the course of an axis). Empty when the server told nothing about the plan.
inline std::string plan_text(const PlanView& p, Lang l) {
    if (!p.known) return "";
    std::string out;
    auto line = [&](const std::string& s) {
        if (!out.empty()) out += '\n';
        out += s;
    };
    if (p.estimate_s > 0) line(std::string(text(T::EstTime, l)) + " " + clock_text(std::lround(p.estimate_s)));
    bool course = false;
    for (const PlanWarning& w : p.warnings) {
        const char* what = nullptr;
        if (w.code == "tiny") what = text(T::WarnTiny, l);
        else if (w.code == "far") what = text(T::WarnFar, l);
        else if (w.code == "scale") what = text(T::WarnScale, l);
        else if (w.code == "too_big") what = text(T::BoardTooBig, l);
        else if (w.code == "out_of_course") {
            what = text(T::WarnCourse, l);
            course = true;
        }
        if (what == nullptr) continue;
        line(std::string("! ") + what + (w.code == "out_of_course" && !w.axis.empty() ? " (" + w.axis + ")" : std::string()));
    }
    if (p.course_known && !p.within_course && !course) line(std::string("! ") + text(T::WarnCourse, l));
    return out;
}

}  // namespace hmi
