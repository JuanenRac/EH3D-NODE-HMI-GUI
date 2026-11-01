// EH3D-NODE-HMI-GUI - core/workflow.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The flow of a job between the screen, the server, the node and the desktop programs, as the screen sees it: a file of the card is chosen,
// sent to the server (which plans it and keeps it READY; nothing starts by itself), started by the operator through the gate, followed
// in the fleet as it runs (the desktop programs see the same job at the same time), and answered when it waits for the operator at a tool
// change. A job that was stopped goes on from the holes (or paths) already done, and a drilling file and a routing file chosen together run one after the other.
// Every step is a request to the server or a reading of its answer, so the flow is tested without a server.
#pragma once

#include <string>

#include "job_tools.hpp"
#include "sdfiles.hpp"
#include "server_link.hpp"

namespace hmi {

enum class Stage { Nothing, FileChosen, Ready, Running, Waiting, Finished, Failed, Stopped };

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
        if (k != FileKind::Drill && k != FileKind::Route) {
            why = "only drilling and routing files can be sent as a job from the card";
            return false;
        }
        if (text.empty()) {
            why = "the file is empty";
            return false;
        }
        if (stage_ != Stage::Nothing && stage_ != Stage::FileChosen) {
            slots_[0] = Slot{};
            slots_[1] = Slot{};
        }
        const bool route = k == FileKind::Route;
        Slot& s = slots_[route ? 1 : 0];
        s = Slot{};
        s.loaded = true;
        s.route = route;
        s.name = name;
        s.text = text;
        s.sw.reset(route ? summarize_route(text) : summarize_drill(text));  // every tool starts on
        last_ = route ? 1 : 0;
        cur_ = first_to_do();
        job_id_.clear();
        reset_run();
        stage_ = Stage::FileChosen;
        return true;
    }

    /// The tool table that goes with the file just chosen (the same name with the extension of the table, read by the application when it exists).
    void set_tools(const std::string& text) { slots_[last_].tools = text; }

    /// What the file of the program in turn holds (its tools and how many holes or cuts each has) and which tools the operator switched off; the screen shows both.
    const ToolSwitch& tool_switch() const { return slots_[cur_].sw; }
    /// Switches a tool of the chosen file off or on (only before the job is sent); false when there is no such tool or the job is already made.
    bool toggle_tool(int number) { return stage_ == Stage::FileChosen && slots_[cur_].sw.toggle(number); }

    /// The request that sends the program in turn to the machine as a job, without the tools that are switched off and without the holes (paths) already done; empty
    /// path when there is nothing to send, no machine is set, or every tool is off (`nothing_to_do`).
    Request create_request(const std::string& machine) const {
        const Slot& s = slots_[cur_];
        if (stage_ != Stage::FileChosen || machine.empty() || !s.loaded || s.sw.nothing_left()) return {};
        return job_request(machine, s.route, s.name, s.text, s.tools, s.sw.off(), s.skip);
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
        if ((r.status != 200 && r.status != 201) || !parse_json(r.body, j) || j.str("id").empty()) {
            why = error_line(r);
            stage_ = Stage::FileChosen;
            return false;
        }
        job_id_ = j.str("id");
        stage_ = Stage::Ready;
        return true;
    }

    /// The request that starts the job. Only a job that is ready can be started: the caller sends it after the gate has confirmed.
    Request start_request() const { return stage_ == Stage::Ready ? job_start_request(job_id_) : Request{}; }
    Request cancel_request() const { return (stage_ == Stage::Running || stage_ == Stage::Waiting || stage_ == Stage::Ready) ? job_cancel_request(job_id_) : Request{}; }
    /// The request that answers the dialog the job waits at (a tool change that the operator has done).
    Request confirm_request() const { return stage_ == Stage::Waiting ? job_confirm_request(job_id_) : Request{}; }

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
        if (job_id_.empty() || stage_ == Stage::Nothing || stage_ == Stage::FileChosen) return;
        for (const MachineRow& m : f.machines) {
            if (m.id != machine) continue;
            percent_ = 0;
            if (m.job_id == job_id_) {
                percent_ = m.percent < 0 ? 0 : m.percent;
                stage_ = (m.state == "waiting" || !m.pause_kind.empty()) ? Stage::Waiting : Stage::Running;
                pause_ = m.pause_kind;
            } else if (stage_ == Stage::Running || stage_ == Stage::Waiting) {
                // the machine has another job or none: ours is over; how it ended (done, stopped, failed) is asked of the server next
                stage_ = Stage::Finished;
                percent_ = 100;
                pause_.clear();
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
            return true;
        }
        if (j.state == "cancelled" || j.state == "failed") {
            stage_ = Stage::Stopped;
            if (!slots_[cur_].loaded && !j.kind.empty()) slots_[cur_].route = j.kind == "route";  // a job followed again after a reset: holes or cuts
            done_ = j.holes_done;
            total_ = j.holes_total;
            message_ = j.message;
            return true;
        }
        return false;
    }

    /// A job that was stopped can go on when it did some of its work and either the server makes the new job itself (it keeps the request, so a screen that was reset and
    /// has no file can go on too) or the file is still here to be sent again. Not when the server said there is nothing left to do.
    bool can_resume() const {
        if (stage_ != Stage::Stopped || done_ <= 0 || no_work_) return false;
        return resumes_by_server() || slots_[cur_].loaded;
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
            why = "the job is not stopped";
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
            resumed_from_ = from;
            stage_ = Stage::Ready;
            return ResumeStep::Made;
        }
        if ((r.status == 404 || r.status == 405) && !(parsed && j.str("error") == "unknown_job")) {
            api_ = ResumeApi::No;
            if (slots_[cur_].loaded && resume(why)) return ResumeStep::Fallback;
            why = "the server is an old one and this screen has no file to send again";
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
        if (!can_resume() || !slots_[cur_].loaded) {
            why = "there is nothing to go on from";
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
        slots_[cur_].done = true;
        cur_ = other;
        job_id_.clear();
        reset_run();
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
        bool done = false;  // this program of the job has been done
        std::string name, text, tools;
        ToolSwitch sw;
        int skip = 0;  // the holes (paths) already done before this job
    };

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
    }

    Stage stage_ = Stage::Nothing;
    Slot slots_[2];  // 0: the drilling program, 1: the routing one
    int cur_ = 0;    // the program in turn
    int last_ = 0;   // the program whose file was chosen last (it receives its tool table)
    std::string job_id_, pause_, message_, resumed_from_;
    int percent_ = 0;
    int done_ = 0, total_ = 0;
    bool unreported_ = false, reported_done_ = false;
    ResumeApi api_ = ResumeApi::Unknown;
    bool no_work_ = false, resume_sent_ = false;
};

}  // namespace hmi
