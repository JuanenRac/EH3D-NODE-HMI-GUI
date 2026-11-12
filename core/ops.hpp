// EH3D-NODE-HMI-GUI - core/ops.hpp: the orders of the screen as an enum, not as text.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// A button of the screen used to say what it asks with a text (`"pause"`, `"start job"`) that the application compared again to know what to send to the server: a text that is
// changed in one place and not in the other is an order that does nothing. Every order the screen sends is now an `Op`; the text (`op_id`) stays as the id the confirmation shows and
// the voice uses, and `op_of` reads it back, so the two cannot drift: a test goes through the whole table.
#pragma once

#include <string>

namespace hmi {

enum class Op { None, Stop, StartJob, Pause, ResumeJob, ToolChanged, Home, SpindleOn };

struct OpEntry {
    Op op;
    const char* id;
};

/// Every order with its id (the text of the action). `SpindleOn` is the start of a longer id (`spindle on 12000 rpm`: the gate writes the turns after it).
inline constexpr OpEntry kOps[] = {
    {Op::Stop, "stop"},        {Op::StartJob, "start job"},   {Op::Pause, "pause"},        {Op::ResumeJob, "resume job"},
    {Op::ToolChanged, "tool changed"}, {Op::Home, "home"},    {Op::SpindleOn, "spindle on"},
};

inline const char* op_id(Op op) {
    for (const OpEntry& e : kOps)
        if (e.op == op) return e.id;
    return "";
}

/// The order of an id; `SpindleOn` also for the ids that start with it. An id that is not an order is `None`.
inline Op op_of(const std::string& id) {
    for (const OpEntry& e : kOps) {
        const std::string name = e.id;
        if (id == name || (e.op == Op::SpindleOn && id.compare(0, name.size(), name) == 0)) return e.op;
    }
    return Op::None;
}

}  // namespace hmi
