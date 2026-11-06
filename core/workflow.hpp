// EH3D-NODE-HMI-GUI - core/workflow.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The flow of a job between the screen, the server, the node and the desktop programs, as the screen sees it: a file of the card is chosen,
// sent to the server (which plans it and keeps it READY; nothing starts by itself), started by the operator through the gate, followed
// in the fleet as it runs (the desktop programs see the same job at the same time), and answered when it waits for the operator at a tool
// change. A job that was stopped goes on from the holes (or paths) already done, and a drilling file and a routing file chosen together run one after the other.
// Every step is a request to the server or a reading of its answer, so the flow is tested without a server.
#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "hmi_config.hpp"
#include "job_bar.hpp"
#include "job_tools.hpp"
#include "rack.hpp"
#include "sdfiles.hpp"
#include "server_link.hpp"
#include "why_text.hpp"

namespace hmi {

enum class Stage { Nothing, FileChosen, Ready, Running, Waiting, Finished, Failed, Stopped };

/// How the job in turn ended, as the server says it (not as the machine happened to stop showing it): `None` while it is not known yet, which includes the moment after the
/// job left the machine and before the server was asked. Only `Done` is a finished job; a job that was cancelled or failed did not do all its work.
enum class Outcome { None, Done, Cancelled, Failed };

/// What the screen shows next to the chosen file besides its tools: how the board lies on the table of the machine, and what the server noted about the job it made.
struct JobAdvice {
    Fit fit;                           // the board against the table (Unknown when either is not known)
    double board_w = 0, board_d = 0;   // the size of the box of the file, mm
    double table_w = 0, table_d = 0;   // the table, mm (0 when not known)
    std::vector<SpindleNote> notes;    // what the job asks for beyond the spindle (the server made it anyway)
    PlanView plan;                     // what the server says about the plan it made: the time and what looks wrong in the work (not known before the job is made)
};

/// Whether the server makes the new job of a stopped one by itself (`POST /jobs/:id/resume-from`): not known until it was asked once; a server that answers 404 or 405 to
/// the route is an old one and the screen goes on with the file it keeps.
enum class ResumeApi { Unknown, Yes, No };
/// What the answer of the server to `resume-from` came to: the new job is made (ready, waiting for the operator), the old way must be used (the screen sends the file it
/// keeps again), or the job cannot go on (the reason is given).
enum class ResumeStep { Made, Fallback, Refused };

class Workflow {
public:
    /// The operator chose a file of the card (its name and text). A file of another kind than a drilling or routing job is refused. A drilling file and a routing
    /// file chosen one after the other are two programs of one job: the drilling goes first, the routing after it. Choosing a file once a job was sent starts again.
    bool choose(const std::string& name, const std::string& text, std::string& why) {
        FileKind k = kind_of_file(name);
        if (k != FileKind::Drill && k != FileKind::Route && k != FileKind::Gcode) {
            why = reasons::kNotAJobFile;
            return false;
        }
        if (text.empty()) {
            why = reasons::kEmptyFile;
            return false;
        }
        // the work in hand is locked while the machine runs it: another file would make the screen forget the job it is following
        if (stage_ == Stage::Running || stage_ == Stage::Waiting) {
            why = reasons::kJobRunning;
            return false;
        }
        const bool gcode = k == FileKind::Gcode;
        // a G-code program is a job of its own: it is never paired with a drilling or a routing file
        if ((stage_ != Stage::Nothing && stage_ != Stage::FileChosen) || gcode || slots_[0].gcode) {
            slots_[0] = Slot{};
            slots_[1] = Slot{};
        }
        const bool route = k == FileKind::Route;
        Slot& s = slots_[route ? 1 : 0];
        s = Slot{};
        s.loaded = true;
        s.route = route;
        s.gcode = gcode;
        s.name = name;
        s.text = text;
        if (!gcode) s.sw.reset(route ? summarize_route(text) : summarize_drill(text));  // every tool starts on; a G-code program has no tools to switch
        last_ = route ? 1 : 0;
        cur_ = first_to_do();
        job_id_.clear();
        reset_run();
        rack_fallback_ = false;
        stage_ = Stage::FileChosen;
        // the places of a program that has no file any more go; the tools of the new file take the free places of the bar
        for (int i = 0; i < 2; ++i) clear_rack_of(i == 1, !slots_[i].loaded);
        rack_fill();
        return true;
    }

    /// The advice for the screen: the box of the file in turn against `table` (the table of the machine, or the one the settings give when the server does not say it) and
    /// the notes of the server about the job made from it. It does not block anything: the server refuses a hole outside the table (`out_of_table`) by itself.
    JobAdvice advice(const TableSize& table) const {
        JobAdvice a;
        const Box& b = slots_[cur_].sw.summary().box;
        a.board_w = b.width();
        a.board_d = b.depth();
        a.table_w = table.width;
        a.table_d = table.depth;
        a.fit = fit_on_table(b, table.width, table.depth);
        a.notes = notes_;
        a.plan = plan_;
        return a;
    }

    /// The tool table that goes with the file just chosen (the same name with the extension of the table, read by the application when it exists).
    void set_tools(const std::string& text) { slots_[last_].tools = text; }

    /// What the file of the program in turn holds (its tools and how many holes or cuts each has) and which tools the operator switched off; the screen shows both.
    const ToolSwitch& tool_switch() const { return slots_[cur_].sw; }
    /// Switches a tool of the chosen file off or on (only before the job is sent); false when there is no such tool or the job is already made.
    bool toggle_tool(int number) {
        if (stage_ != Stage::FileChosen || !slots_[cur_].sw.toggle(number)) return false;
        rack_fill();
        return true;
    }

    // ---- the bar of the tool changer (see rack.hpp)
    /// Takes the settings of the bar: the places the machine defines, the switch that blocks the start while they are wrong and the one that sends them with the job.
    void configure_rack(const Settings& s) {
        std::vector<int> places;
        if (!parse_places(s.rack_places, places) || places.empty()) places = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
        places_ = places;
        rack_check_ = s.rack_check;
        rack_chain_ = s.rack_chain;
        rack_fill();
    }
    /// True when the machine changes the tools itself (the model of the fleet says so) and the places of the bar count.
    bool atc() const { return changes_tools_itself(model_); }
    const std::vector<int>& rack_places() const { return places_; }
    /// The tools the job does, over both programs that are still to do: the drills first, then the cutters, each in the order the job uses them.
    std::vector<RackTool> rack_tools() const {
        std::vector<RackTool> out;
        for (int i = 0; i < 2; ++i) {
            const ProgramTools p = program_tools(i);
            out.insert(out.end(), p.tools.begin(), p.tools.end());
        }
        return out;
    }
    const RackAssignment& rack_assignment() const { return rack_; }
    int rack_slot(bool route, int tool) const { return slot_of(rack_, route, tool); }
    /// The operator puts a tool in a place of the bar (0 takes the place away). Not while the job runs.
    bool set_rack_place(bool route, int tool, int slot) {
        if (stage_ == Stage::Running || stage_ == Stage::Waiting) return false;
        bool known = false;
        for (const RackTool& t : rack_tools()) known = known || (t.route == route && t.tool == tool);
        if (!known) return false;
        set_slot(rack_, route, tool, slot);  // a place the machine does not define is kept: the check tells it before the start
        return true;
    }
    /// Gives a place to every tool of the job that has none (the small drills first, then the cutters, into the free places from the lowest).
    void rack_fill() { fill_in_order(rack_tools(), rack_, places_); }
    /// What stops the job from starting: empty when the machine does not change the tools itself, when the check is turned off or when all is well.
    std::vector<RackProblem> rack_problems() const {
        if (!atc() || !rack_check_) return {};
        return check_rack(rack_tools(), rack_, places_);
    }
    /// The squares of the bar: every tool of the job with its place and where it is in the work.
    std::vector<BarTool> rack_bar() const {
        std::vector<BarTool> out;
        const bool running = stage_ == Stage::Running || stage_ == Stage::Waiting;
        const JobBar b = bar();
        for (int i = 0; i < 2; ++i) {
            if (!slots_[i].loaded) continue;
            const ProgramTools p = program_tools(i, true);
            for (const RackTool& t : p.tools) {
                BarTool q;
                q.route = t.route;
                q.tool = t.tool;
                q.diameter_mm = t.diameter_mm;
                q.slot = slot_of(rack_, t.route, t.tool);
                q.state = BarState::Pending;
                if (slots_[i].done) {
                    q.state = BarState::Done;
                } else if (i == cur_) {
                    bool found = false;
                    for (std::size_t k = 0; k < b.tools.size(); ++k) {
                        if (b.tools[k].number != t.tool) continue;
                        found = true;
                        if (b.tools[k].finished()) q.state = BarState::Done;
                        else if (running && static_cast<int>(k) == b.current) q.state = BarState::InUse;
                    }
                    if (!found && b.known) q.state = BarState::Done;  // the steps of this tool were done before this job
                }
                out.push_back(q);
            }
        }
        return out;
    }
    /// How the server took the bar of the job it made: not sent (no machine with the changer, or the server refused it), planned with it, or made without it.
    enum class RackReply { NotSent, Applied, NotApplied };
    RackReply rack_reply() const { return rack_reply_; }
    /// True once after the server turned the places down with 400 or 422: the job is to be sent again without them.
    bool take_rack_retry() {
        const bool r = rack_retry_;
        rack_retry_ = false;
        return r;
    }
    /// How many times a program of the job ended well while another was still to come, and the name of the program that came next: the window between the stages.
    int stages_done() const { return stages_done_; }
    const std::string& stage_next_name() const { return stage_next_; }
    /// True when the whole job is over and the server said it did all its work: the board can be taken out. A drilling that ends with a routing still to do is not that.
    bool job_complete() const { return ended_well() && next_name().empty(); }

    /// The request that sends the program in turn to the machine as a job, without the tools that are switched off and without the holes (paths) already done; empty
    /// path when there is nothing to send, no machine is set, or every tool is off (`nothing_to_do`).
    Request create_request(const std::string& machine) const {
        const Slot& s = slots_[cur_];
        if (stage_ != Stage::FileChosen || machine.empty() || !s.loaded || s.sw.nothing_left()) return {};
        if (s.gcode) return job_gcode_request(machine, s.name, s.text, s.skip);
        RackPlan plan;
        const RackPlan* rack = rack_for_request(plan) ? &plan : nullptr;
        return job_request(machine, s.route, s.name, s.text, s.tools, s.sw.off(), s.skip, rack);
    }
    /// True when the operator switched off every tool of the program in turn.
    bool nothing_to_do() const { return stage_ == Stage::FileChosen && slots_[cur_].loaded && slots_[cur_].sw.nothing_left(); }
    /// The program in turn has nothing left to do (all its tools are off) but the other one has: goes on to it. False when there is no other program to go to.
    bool skip_empty() {
        if (!nothing_to_do()) return false;
        const int other = 1 - cur_;
        if (!slots_[other].loaded || slots_[other].done || slots_[other].sw.nothing_left()) return false;
        slots_[cur_].done = true;
        cur_ = other;
        return true;
    }

    /// The answer of the server to the creation of the job.
    bool on_created(const Response& r, std::string& why) {
        Json j;
        RackPlan sent;
        const bool with_rack = rack_for_request(sent);
        if ((r.status != 200 && r.status != 201) || !parse_json(r.body, j) || j.str("id").empty()) {
            why = error_line(r);
            stage_ = Stage::FileChosen;
            // a server that turns the bar down (400 or 422) gets the job again without it: the tools are then taken in the order of the job
            if (with_rack && (r.status == 400 || r.status == 422)) {
                rack_fallback_ = true;
                rack_retry_ = true;
            }
            return false;
        }
        job_id_ = j.str("id");
        notes_ = parse_job_notes(r.body);
        plan_ = parse_plan_view(r.body);
        rack_reply_ = !with_rack ? RackReply::NotSent : (parse_rack_applied(r.body) ? RackReply::Applied : RackReply::NotApplied);
        stage_ = Stage::Ready;
        return true;
    }

    /// The request that starts the job. Only a job that is ready can be started: the caller sends it after the gate has confirmed.
    Request start_request() const { return stage_ == Stage::Ready ? job_start_request(job_id_) : Request{}; }
    Request cancel_request() const { return (stage_ == Stage::Running || stage_ == Stage::Waiting || stage_ == Stage::Ready) ? job_cancel_request(job_id_) : Request{}; }
    /// The requests of a stop: the job the fleet shows on the machine (also one that another program or screen started) and, when it is another one, the job of this screen
    /// that is ready or running. Empty when no job is known. Cancelling is what every kind of job allows.
    std::vector<Request> stop_requests(const FleetView& f, const std::string& machine) const {
        std::vector<Request> out;
        std::string on_machine;
        for (const MachineRow& m : f.machines)
            if (m.id == machine && !m.job_id.empty()) on_machine = m.job_id;
        if (!on_machine.empty()) out.push_back(job_cancel_request(on_machine));
        const Request own = cancel_request();
        if (!own.path.empty() && job_id_ != on_machine) out.push_back(own);
        return out;
    }
    /// Holds the running job (the server holds only G-code jobs: for a drilling or routing job it answers with the reason, which the screen shows; the job is never cancelled).
    Request pause_request() const { return stage_ == Stage::Running ? job_pause_request(job_id_) : Request{}; }
    /// True while the operator's hold keeps the job waiting (the server shows it waiting with a `hold` or `resume` pause), as opposed to a tool change.
    bool held() const { return stage_ == Stage::Waiting && (pause_ == "hold" || pause_ == "resume"); }
    /// True while the job waits for the operator at a tool change.
    bool tool_wait() const { return stage_ == Stage::Waiting && !held(); }
    /// The request that lets a held job go on.
    Request release_request() const { return held() ? job_resume_request(job_id_) : Request{}; }
    /// The request that answers the dialog the job waits at (a tool change that the operator has done).
    Request confirm_request() const { return tool_wait() ? job_confirm_request(job_id_) : Request{}; }

    /// The answer of the server to the start: it is running, or it was refused (the machine is busy, the job is gone).
    bool on_started(const Response& r, std::string& why) {
        if (r.status != 200 && r.status != 202) {
            why = error_line(r);
            return false;
        }
        stage_ = Stage::Running;
        return true;
    }

    /// After a reset: follow the job that this screen had (its id and name come from `job_memory.hpp`). It counts as running until the next reading of the fleet says
    /// what it is: still running or waiting, or over (the server no longer shows it on the machine). False when there is no id. The text of the file is not kept, but the
    /// server keeps the request of the job: one followed this way goes on through `resume-from` when the server has the route.
    bool adopt(const std::string& job_id, const std::string& name) {
        if (job_id.empty()) return false;
        slots_[0] = Slot{};
        slots_[1] = Slot{};
        cur_ = 0;
        last_ = 0;
        slots_[0].name = name;
        job_id_ = job_id;
        reset_run();
        stage_ = Stage::Running;
        return true;
    }

    /// A reading of the fleet: where the job of this screen is, as the server says (the job may have been started or cancelled from another
    /// program; the screen follows what the server says).
    void on_fleet(const FleetView& f, const std::string& machine) {
        for (const MachineRow& m : f.machines)
            if (m.id == machine) model_ = m.model;  // the model is known whether a job is followed or not
        if (job_id_.empty() || stage_ == Stage::Nothing || stage_ == Stage::FileChosen) return;
        for (const MachineRow& m : f.machines) {
            if (m.id != machine) continue;
            percent_ = 0;
            if (m.job_id == job_id_) {
                percent_ = m.percent < 0 ? 0 : m.percent;
                steps_done_ = m.steps_done;
                steps_total_ = m.steps_total;
                estimate_s_ = m.estimate_s;
                pause_tool_ = m.pause_tool;
                stage_ = (m.state == "waiting" || !m.pause_kind.empty()) ? Stage::Waiting : Stage::Running;
                pause_ = m.pause_kind;
            } else if (stage_ == Stage::Running || stage_ == Stage::Waiting) {
                // the machine has another job or none: ours is over; how it ended (done, stopped, failed) is asked of the server next
                stage_ = Stage::Finished;
                percent_ = 100;
                pause_.clear();
                pause_tool_ = 0;
                unreported_ = true;
            }
            return;
        }
    }

    /// True once a job ended and the screen has not yet asked the server how: the application sends `report_request` and gives the answer to `on_report`.
    bool want_report() const { return unreported_ && !job_id_.empty(); }
    Request report_request() const { return want_report() ? job_report_request(job_id_) : Request{}; }
    /// The answer to `report_request`: a job that is `done` stays finished; one that was cancelled or failed becomes `Stopped`, with the holes (paths) it did. Any
    /// answer ends the asking. True when the answer said how the job ended.
    bool on_report(const Response& r) {
        unreported_ = false;
        JobReport j;
        if (stage_ != Stage::Finished || r.status != 200 || !parse_job_report(r.body, j)) return false;
        if (j.state == "done") {
            reported_done_ = true;
            outcome_ = Outcome::Done;
            return true;
        }
        if (j.state == "cancelled" || j.state == "failed") {
            stage_ = Stage::Stopped;
            outcome_ = j.state == "failed" ? Outcome::Failed : Outcome::Cancelled;
            if (!slots_[cur_].loaded && !j.kind.empty()) {  // a job followed again after a reset: holes, cuts or lines
                slots_[cur_].route = j.kind == "route";
                slots_[cur_].gcode = j.kind == "gcode";
            }
            done_ = j.holes_done;
            total_ = j.holes_total;
            message_ = j.message;
            return true;
        }
        return false;
    }

    /// How the job in turn ended. `Done` only when the server said `done`; a job that left the machine and was not asked about yet is `None`, not finished.
    Outcome outcome() const { return outcome_; }
    /// True when the job in turn is over and the server said it did all its work: the notice of the end and the forgetting of the job wait for this.
    bool ended_well() const { return stage_ == Stage::Finished && reported_done_; }
    /// True when the end of the job is known for good: well (`ended_well`), or stopped or failed (the job is kept to go on from). False while the server has not been asked.
    bool settled() const { return ended_well() || stage_ == Stage::Stopped || stage_ == Stage::Failed; }
    /// The notes of the server about the job that was made (see `advice`).
    const std::vector<SpindleNote>& notes() const { return notes_; }
    /// The model of the machine as the server holds it (empty until the fleet was read) and the tool the job waits for at a tool change (0 when the server does not say it).
    const std::string& model() const { return model_; }
    int pause_tool() const { return pause_tool_; }
    /// The bar of the job in turn: the tool at work, the next one, the tools finished and the time left, from the tools of the file and the progress the server reports.
    JobBar bar() const {
        const Slot& s = slots_[cur_];
        if (!s.loaded || s.gcode) return JobBar{};
        const bool running = stage_ == Stage::Running || stage_ == Stage::Waiting;
        const double estimate = estimate_s_ > 0 ? estimate_s_ : plan_.estimate_s;
        return make_job_bar(s.sw, s.skip, running ? steps_done_ : 0, running ? steps_total_ : 0, estimate);
    }

    /// A job that was stopped can go on when it did some of its work and either the server makes the new job itself (it keeps the request, so a screen that was reset and
    /// has no file can go on too) or the file is still here to be sent again. Not when the server said there is nothing left to do.
    bool can_resume() const {
        if (stage_ != Stage::Stopped || done_ <= 0 || no_work_) return false;
        return resumes_by_server() || (slots_[cur_].loaded && !slots_[cur_].gcode);  // the lines of a program are counted by the server: no file to send again
    }
    /// True when the way to go on is the server's: the job has an id and the server has not shown itself to be one without the route.
    bool resumes_by_server() const { return stage_ == Stage::Stopped && !job_id_.empty() && api_ != ResumeApi::No; }
    /// The request that asks the server to make the new job from the saved one, empty when it cannot be asked (not stopped, the server is an old one) or when it was just
    /// asked and the answer has not come (a second touch does not make a second job). The answer goes to `on_resume_from`; when the request could not be sent `resume_failed`.
    Request resume_request() {
        if (!can_resume() || !resumes_by_server() || resume_sent_) return {};
        resume_sent_ = true;
        return job_resume_from_request(job_id_);
    }
    /// The request could not be sent (no link): it may be asked again.
    void resume_failed() { resume_sent_ = false; }
    /// The answer of the server to `resume_request`. `Made`: the job of the answer is ready, not started (the operator starts it, through the gate), and `resumed_from()` names
    /// the one it goes on from. `Fallback`: the server is an old one (404 or 405 without the error of an unknown job) and the screen still has the file: the job is prepared as
    /// `resume` does and the application sends it. `Refused`: `why` says it (the job is not stopped for the server, nothing is left, no file to fall back on...).
    ResumeStep on_resume_from(const Response& r, std::string& why) {
        resume_sent_ = false;
        if (stage_ != Stage::Stopped) {
            why = reasons::kNotStopped;
            return ResumeStep::Refused;
        }
        Json j;
        const bool parsed = parse_json(r.body, j) && j.type == Json::Type::Object;
        if ((r.status == 200 || r.status == 201) && parsed && !j.str("id").empty()) {
            api_ = ResumeApi::Yes;
            const std::string from = j.str("resumed_from", job_id_);
            slots_[cur_].skip += done_;
            reset_run();
            job_id_ = j.str("id");
            notes_ = parse_job_notes(r.body);
            plan_ = parse_plan_view(r.body);
            resumed_from_ = from;
            stage_ = Stage::Ready;
            return ResumeStep::Made;
        }
        if ((r.status == 404 || r.status == 405) && !(parsed && j.str("error") == "unknown_job")) {
            api_ = ResumeApi::No;
            if (slots_[cur_].loaded && resume(why)) return ResumeStep::Fallback;
            why = reasons::kOldServerNoFile;
            return ResumeStep::Refused;
        }
        if (r.status == 422 && parsed && j.str("error") == "no_work") no_work_ = true;
        why = error_line(r);
        return ResumeStep::Refused;
    }
    /// The job that the job in turn goes on from (the id of the stopped job), empty when it is not the continuation of another.
    const std::string& resumed_from() const { return resumed_from_; }

    /// The old way, for a server without the route: prepares the job to go on from the holes (paths) already done: the program in turn is ready to be sent again with `skip`;
    /// the tools switched off stay off. The application sends it, and the start goes through the gate as any other.
    bool resume(std::string& why) {
        if (!can_resume() || !slots_[cur_].loaded || slots_[cur_].gcode) {
            why = reasons::kNothingToGoOn;
            return false;
        }
        slots_[cur_].skip += done_;
        job_id_.clear();
        reset_run();
        stage_ = Stage::FileChosen;
        return true;
    }

    /// The job that was running ended well and the other program of the job is still to do (the routing after the drilling): it comes next. The application sends it
    /// and the operator starts it, after changing the tool. False when there is nothing after it.
    bool advance() {
        if (stage_ != Stage::Finished || !reported_done_) return false;
        const int other = 1 - cur_;
        if (!slots_[other].loaded || slots_[other].done) return false;
        // the drilling left its last tool in the chuck only if the server planned it so; the routing then takes it over
        RackPlan was;
        slots_[cur_].chained = rack_for_request(was) && was.carry_out && rack_reply_ == RackReply::Applied;
        slots_[cur_].done = true;
        cur_ = other;
        job_id_.clear();
        reset_run();
        ++stages_done_;
        stage_next_ = slots_[cur_].name;
        stage_ = Stage::FileChosen;
        return true;
    }
    /// The name of the program that comes after the one in turn, empty when there is none.
    const std::string& next_name() const {
        static const std::string none;
        const Slot& o = slots_[1 - cur_];
        return o.loaded && !o.done ? o.name : none;
    }
    /// True when the program in turn is a routing one.
    bool route() const { return slots_[cur_].route; }
    /// True when the program in turn is a G-code program (its counts are lines, not holes or cuts).
    bool gcode() const { return slots_[cur_].gcode; }
    /// The holes (paths) already done before the job in turn, and the ones a stopped job did of the ones it had; for the screen.
    int skipped() const { return slots_[cur_].skip; }
    int done() const { return done_; }
    int total() const { return total_; }
    const std::string& message() const { return message_; }

    Stage stage() const { return stage_; }
    int percent() const { return percent_; }
    const std::string& job_id() const { return job_id_; }
    const std::string& name() const { return slots_[cur_].name; }
    /// `tool_change` while the job waits for the operator.
    const std::string& pause() const { return pause_; }
    /// A name for the report of the job on the card, free of anything that is already there.
    std::string report_base() const {
        std::string b = slots_[cur_].name;
        std::size_t dot = b.rfind('.');
        if (dot != std::string::npos && dot > 0) b.resize(dot);
        return b + "-job";
    }

private:
    struct Slot {
        bool loaded = false;
        bool route = false;
        bool gcode = false;
        bool done = false;  // this program of the job has been done
        std::string name, text, tools;
        ToolSwitch sw;
        int skip = 0;  // the holes (paths) already done before this job
        bool chained = false;  // it ran with its last tool left in the chuck (the server planned it so)
    };

    /// The tools of one program that the job does, in the order it uses them (switched off ones are left out). With `all`, a program already done counts too.
    ProgramTools program_tools(int i, bool all = false) const {
        ProgramTools p;
        const Slot& s = slots_[i];
        p.loaded = s.loaded && !s.gcode;
        p.done = s.done;
        p.chained = s.chained;
        if (!p.loaded || (s.done && !all)) return p;
        std::vector<const ToolInfo*> order;
        for (const ToolInfo& t : s.sw.summary().tools)
            if (t.count > 0 && !s.sw.is_off(t.number)) order.push_back(&t);
        std::sort(order.begin(), order.end(), [](const ToolInfo* a, const ToolInfo* b) { return a->use < b->use; });
        for (const ToolInfo* t : order) p.tools.push_back({i == 1, t->number, t->diameter_mm});
        return p;
    }

    /// The bar of the request of the program in turn; false when none goes (a G-code job, a machine with the manual change, the switch off, or a server that turned it down).
    bool rack_for_request(RackPlan& out) const {
        if (!rack_chain_ || rack_fallback_ || !atc() || !slots_[cur_].loaded || slots_[cur_].gcode) return false;
        out = plan_rack(program_tools(0, true), program_tools(1, true), cur_ == 1, rack_);
        return out.present();
    }

    /// Takes the places of a program away when it has no file any more (a new job replaced it).
    void clear_rack_of(bool route, bool unloaded) {
        if (!unloaded) return;
        rack_.erase(std::remove_if(rack_.begin(), rack_.end(), [&](const RackPlace& p) { return p.route == route; }), rack_.end());
    }

    int first_to_do() const {
        for (int i = 0; i < 2; ++i)
            if (slots_[i].loaded && !slots_[i].done) return i;
        return slots_[0].loaded ? 0 : 1;
    }
    void reset_run() {
        percent_ = 0;
        done_ = 0;
        total_ = 0;
        message_.clear();
        pause_.clear();
        unreported_ = false;
        reported_done_ = false;
        no_work_ = false;
        resume_sent_ = false;
        resumed_from_.clear();
        outcome_ = Outcome::None;
        notes_.clear();
        plan_ = PlanView{};
        steps_done_ = steps_total_ = 0;
        estimate_s_ = 0;
        pause_tool_ = 0;
        rack_reply_ = RackReply::NotSent;
        rack_retry_ = false;
    }

    Stage stage_ = Stage::Nothing;
    Slot slots_[2];  // 0: the drilling program, 1: the routing one
    int cur_ = 0;    // the program in turn
    int last_ = 0;   // the program whose file was chosen last (it receives its tool table)
    std::string job_id_, pause_, message_, resumed_from_;
    int percent_ = 0;
    int done_ = 0, total_ = 0;
    bool unreported_ = false, reported_done_ = false;
    Outcome outcome_ = Outcome::None;
    std::vector<SpindleNote> notes_;
    PlanView plan_;
    std::string model_;
    int steps_done_ = 0, steps_total_ = 0, pause_tool_ = 0;
    double estimate_s_ = 0;
    ResumeApi api_ = ResumeApi::Unknown;
    bool no_work_ = false, resume_sent_ = false;
    RackAssignment rack_;
    std::vector<int> places_ = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    bool rack_check_ = true, rack_chain_ = true, rack_fallback_ = false, rack_retry_ = false;
    RackReply rack_reply_ = RackReply::NotSent;
    int stages_done_ = 0;
    std::string stage_next_;
};

}  // namespace hmi
