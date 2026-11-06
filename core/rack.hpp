// EH3D-NODE-HMI-GUI - core/rack.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The bar of the tool changer, as the desktop program keeps it: the place of each tool of the job in the bar (the tools do not have to be in the bar in the order the job
// uses them), what is wrong with those places (a tool without a place, a place the machine does not have, two tools in one place), the chaining of a drilling program and a
// routing program in one change (`rack.first`, `rack.carry_in`, `rack.carry_out`, `rack.slots`, `rack.carry_slot` of the request of the job) and the row of the bar with the state
// of every tool. There is no screen in this file: the window asks it and keeps only the buttons.
#pragma once

#include <algorithm>
#include <cstdlib>
#include <string>
#include <utility>
#include <vector>

namespace hmi {

/// The places of the bar that can hold a tool: 1 to 14 (the place 0 is the test pin and 15 the detector of a broken drill).
constexpr int kMaxToolSlot = 14;

/// A tool of the job, told by its program (drill or routing) and its number; its size lets the screen put the small drills first.
struct RackTool {
    bool route = false;
    int tool = 0;
    double diameter_mm = 0;
    bool operator==(const RackTool& o) const { return route == o.route && tool == o.tool; }
};

/// Where each tool is: the program, the tool and its place.
struct RackPlace {
    bool route = false;
    int tool = 0;
    int slot = 0;
};
using RackAssignment = std::vector<RackPlace>;

/// What stops a job from starting.
struct RackProblem {
    enum class Kind { Unassigned, Undefined, Shared } kind = Kind::Unassigned;
    bool route = false;
    int tool = 0;
    int slot = 0;        // Undefined and Shared: the place
    int other_tool = 0;  // Shared: another tool in the same place (the first one found)
    bool other_route = false;
};

/// The places the settings define for the bar (`rack_places = 1-14` or `1,2,5-8`): the ones from 1 to 14, in order, once each. False when the text is not a list of places.
inline bool parse_places(const std::string& text, std::vector<int>& out) {
    std::vector<int> v;
    std::size_t i = 0;
    auto skip = [&]() { while (i < text.size() && (text[i] == ' ' || text[i] == '\t')) ++i; };
    auto number = [&](int& n) {
        skip();
        if (i >= text.size() || text[i] < '0' || text[i] > '9') return false;
        long x = 0;
        while (i < text.size() && text[i] >= '0' && text[i] <= '9') {
            x = x * 10 + (text[i] - '0');
            if (x > 1000) x = 1000;
            ++i;
        }
        n = static_cast<int>(x);
        return true;
    };
    skip();
    if (i >= text.size()) return false;
    while (i < text.size()) {
        int a = 0, b = 0;
        if (!number(a)) return false;
        b = a;
        skip();
        if (i < text.size() && text[i] == '-') {
            ++i;
            if (!number(b) || b < a) return false;
        }
        for (int p = a; p <= b && p <= 1000; ++p) v.push_back(p);
        skip();
        if (i < text.size()) {
            if (text[i] != ',') return false;
            ++i;
            skip();
            if (i >= text.size()) return false;
        }
    }
    std::vector<int> ok;
    for (int p : v)
        if (p >= 1 && p <= kMaxToolSlot) ok.push_back(p);
    std::sort(ok.begin(), ok.end());
    ok.erase(std::unique(ok.begin(), ok.end()), ok.end());
    out = ok;
    return true;
}

/// The places a tool can be given: the defined ones from 1 to 14, in order.
inline std::vector<int> selectable_places(const std::vector<int>& defined) {
    std::vector<int> v;
    for (int p : defined)
        if (p >= 1 && p <= kMaxToolSlot) v.push_back(p);
    std::sort(v.begin(), v.end());
    v.erase(std::unique(v.begin(), v.end()), v.end());
    return v;
}

/// The place of a tool, 0 when it has none.
inline int slot_of(const RackAssignment& a, bool route, int tool) {
    for (const RackPlace& p : a)
        if (p.route == route && p.tool == tool) return p.slot;
    return 0;
}

/// Gives a tool a place, or takes its place away (`slot` 0).
inline void set_slot(RackAssignment& a, bool route, int tool, int slot) {
    a.erase(std::remove_if(a.begin(), a.end(), [&](const RackPlace& p) { return p.route == route && p.tool == tool; }), a.end());
    if (slot > 0) a.push_back({route, tool, slot});
}

/// What is wrong with the places of the tools of the job: the tools without a place, the places the machine does not define and the places two tools share. Empty: the job
/// can start.
inline std::vector<RackProblem> check_rack(const std::vector<RackTool>& tools, const RackAssignment& a, const std::vector<int>& defined) {
    const std::vector<int> ok = selectable_places(defined);
    std::vector<RackProblem> out;
    for (std::size_t i = 0; i < tools.size(); ++i) {
        const RackTool& t = tools[i];
        const int s = slot_of(a, t.route, t.tool);
        if (s == 0) {
            RackProblem p;
            p.kind = RackProblem::Kind::Unassigned;
            p.route = t.route;
            p.tool = t.tool;
            out.push_back(p);
            continue;
        }
        if (!std::binary_search(ok.begin(), ok.end(), s)) {
            RackProblem p;
            p.kind = RackProblem::Kind::Undefined;
            p.route = t.route;
            p.tool = t.tool;
            p.slot = s;
            out.push_back(p);
        }
        // a place shared with an earlier tool is told once, on the later one
        for (std::size_t k = 0; k < i; ++k) {
            if (slot_of(a, tools[k].route, tools[k].tool) == s) {
                RackProblem p;
                p.kind = RackProblem::Kind::Shared;
                p.route = t.route;
                p.tool = t.tool;
                p.slot = s;
                p.other_route = tools[k].route;
                p.other_tool = tools[k].tool;
                out.push_back(p);
                break;
            }
        }
    }
    return out;
}

/// Gives a place to each tool that has none, in order: the drills before the cutters, smaller before bigger, into the free places from the lowest. A tool that already has a place
/// keeps it; when the places run out the tools that are left stay without one.
inline void fill_in_order(const std::vector<RackTool>& tools, RackAssignment& a, const std::vector<int>& defined) {
    std::vector<int> free_places;
    for (int p : selectable_places(defined)) {
        bool used = false;
        for (const RackTool& t : tools) used = used || slot_of(a, t.route, t.tool) == p;
        if (!used) free_places.push_back(p);
    }
    std::vector<RackTool> todo;
    for (const RackTool& t : tools)
        if (slot_of(a, t.route, t.tool) == 0) todo.push_back(t);
    std::stable_sort(todo.begin(), todo.end(), [](const RackTool& x, const RackTool& y) {
        if (x.route != y.route) return !x.route;
        if (x.diameter_mm != y.diameter_mm) return x.diameter_mm < y.diameter_mm;
        return x.tool < y.tool;
    });
    std::size_t next = 0;
    for (const RackTool& t : todo) {
        if (next >= free_places.size()) break;
        set_slot(a, t.route, t.tool, free_places[next++]);
    }
}

/// What the request of a job says about the bar. A job without chaining and without places has none of it and its request is the one of the first version.
struct RackPlan {
    int first = 0;           // how many places of the bar the tools of the jobs before this one take
    bool carry_in = false;   // the chuck comes holding the last tool of the job before
    bool carry_out = false;  // this job leaves its last tool in the chuck for the next one
    std::vector<std::pair<int, int>> slots;  // the tool and its place
    int carry_slot = 0;      // with carry_in, the place of the tool the chuck comes holding (0: the place `first`)
    bool present() const { return first > 0 || carry_in || carry_out || !slots.empty(); }
};

/// The `rack` object of the request of a job (the request then says `request_version` 2).
inline std::string rack_json(const RackPlan& r) {
    std::string out = "{\"first\":" + std::to_string(r.first) + ",\"carry_in\":" + (r.carry_in ? "true" : "false") + ",\"carry_out\":" + (r.carry_out ? "true" : "false");
    if (!r.slots.empty()) {
        out += ",\"slots\":[";
        for (std::size_t i = 0; i < r.slots.size(); ++i) out += (i ? "," : "") + std::string("{\"tool\":") + std::to_string(r.slots[i].first) + ",\"place\":" + std::to_string(r.slots[i].second) + "}";
        out += "]";
    }
    if (r.carry_in && r.carry_slot > 0) out += ",\"carry_slot\":" + std::to_string(r.carry_slot);
    return out + "}";
}

/// The tools of one program that the job does, in the order it uses them (the tools switched off are left out by the caller).
struct ProgramTools {
    bool loaded = false;
    bool done = false;
    bool chained = false;             // this program ran with its last tool left in the chuck (the server planned it so)
    std::vector<RackTool> tools;      // in the order of use
};

/// The plan of the bar for the program in turn. `drill` and `route` are the two programs of the job; `route_in_turn` says which is the one sent now. The drilling leaves its last
/// tool in the chuck when a routing program is still to come, and the routing takes it over when the drilling ran chained before it.
inline RackPlan plan_rack(const ProgramTools& drill, const ProgramTools& route, bool route_in_turn, const RackAssignment& a) {
    RackPlan r;
    const ProgramTools& mine = route_in_turn ? route : drill;
    const ProgramTools& other = route_in_turn ? drill : route;
    for (const RackTool& t : mine.tools) {
        const int s = slot_of(a, t.route, t.tool);
        if (s > 0) r.slots.push_back({t.tool, s});
    }
    if (!route_in_turn) {
        // the drilling goes first; its last tool stays in the chuck for the routing that comes after it
        r.carry_out = other.loaded && !other.done && !other.tools.empty() && !mine.tools.empty();
    } else if (other.loaded && other.done && other.chained && !other.tools.empty()) {
        r.first = static_cast<int>(other.tools.size());
        r.carry_in = true;
        const RackTool& last = other.tools.back();
        r.carry_slot = slot_of(a, last.route, last.tool);
    }
    return r;
}

/// The state of a tool in the bar.
enum class BarState { Pending, InUse, Done };

/// A square of the bar: the tool, its place and where it is in the work.
struct BarTool {
    bool route = false;
    int tool = 0;
    double diameter_mm = 0;
    int slot = 0;
    BarState state = BarState::Pending;
};

}  // namespace hmi
