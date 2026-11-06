// EH3D-NODE-HMI-GUI - core/stage_notice.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// What the screen says when a program of the job ends, and the words about the bar of the tool changer. A drilling that ends with a routing still to come is a STAGE: its own
// notice (a spoken `stage.wav`, or a tone) and a window that says the routing comes next, with the buttons to follow it or to stop; it is not the end of the job, so the sound of
// the end and the order to take the board out wait until the whole job is done. The decision is made here from two numbers of the workflow, so it is tested on a computer.
#pragma once

#include <string>
#include <vector>

#include "rack.hpp"
#include "ui_text.hpp"

namespace hmi {

enum class EndNotice { None, StageDone, BoardOut };

/// Looks at the workflow now and then and says, once, what is new: a stage done (the drilling is over and the routing comes next) or the end of the whole job (the board can come out).
class EndWatcher {
public:
    /// `stages_done` is how many programs ended well with another still to come; `job_complete` is true once the whole job is over and the server said it did all its work.
    EndNotice update(int stages_done, bool job_complete) {
        if (stages_done > stages_) {
            stages_ = stages_done;
            return EndNotice::StageDone;
        }
        if (job_complete && !complete_) {
            complete_ = true;
            return EndNotice::BoardOut;
        }
        if (!job_complete) complete_ = false;
        return EndNotice::None;
    }
    /// A new job starts counting from zero again.
    void reset(int stages_done = 0) {
        stages_ = stages_done;
        complete_ = false;
    }

private:
    int stages_ = 0;
    bool complete_ = false;
};

/// The text of the window between the stages: what is done and which program comes next.
inline std::string stage_text(const std::string& next_name, Lang l) {
    std::string out = text(T::StageDone, l);
    if (!next_name.empty()) out += "\n" + next_name;
    return out;
}

/// The lines about the places of the bar that stop the start: the tool, its place when it has one, and what is wrong. `T3 (4): shares its place...`
inline std::string rack_problem_text(const std::vector<RackProblem>& problems, Lang l) {
    std::string out;
    for (const RackProblem& p : problems) {
        if (!out.empty()) out += '\n';
        out += std::string(text(T::Tool, l)) + " " + std::to_string(p.tool) + (p.route ? " (R)" : "");
        if (p.slot > 0) out += " @" + std::to_string(p.slot);
        const T what = p.kind == RackProblem::Kind::Unassigned ? T::RackNoSlot : (p.kind == RackProblem::Kind::Undefined ? T::RackUndefined : T::RackShared);
        out += std::string(": ") + text(what, l);
    }
    return out;
}

}  // namespace hmi
