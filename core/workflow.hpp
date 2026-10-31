// EH3D-NODE-HMI-GUI - core/workflow.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The flow of a job between the screen, the server, the node and the desktop programs, as the screen sees it: a file of the card is chosen,
// sent to the server (which plans it and keeps it READY; nothing starts by itself), started by the operator through the gate, followed
// in the fleet as it runs (the desktop programs see the same job at the same time), and answered when it waits for the operator at a tool
// change. Every step is a request to the server or a reading of its answer, so the flow is tested without a server.
#pragma once

#include <string>

#include "sdfiles.hpp"
#include "server_link.hpp"

namespace hmi {

enum class Stage { Nothing, FileChosen, Ready, Running, Waiting, Finished, Failed };

class Workflow {
public:
    /// The operator chose a file of the card (its name and text). A file of another kind than a drilling or routing job is refused.
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
        name_ = name;
        text_ = text;
        route_ = k == FileKind::Route;
        job_id_.clear();
        stage_ = Stage::FileChosen;
        return true;
    }

    /// The tool table that goes with the chosen file (the same name with the extension of the table, read by the application when it exists).
    void set_tools(const std::string& text) { tools_ = text; }

    /// The request that sends the chosen file to the machine as a job; empty path when there is nothing to send or no machine is set.
    Request create_request(const std::string& machine) const {
        if (stage_ != Stage::FileChosen || machine.empty()) return {};
        return job_request(machine, route_, name_, text_, tools_);
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
    /// what it is: still running or waiting, or over (the server no longer shows it on the machine). False when there is no id.
    bool adopt(const std::string& job_id, const std::string& name) {
        if (job_id.empty()) return false;
        job_id_ = job_id;
        name_ = name;
        text_.clear();
        tools_.clear();
        pause_.clear();
        percent_ = 0;
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
                // the machine has another job or none: ours is over
                stage_ = Stage::Finished;
                percent_ = 100;
                pause_.clear();
            }
            return;
        }
    }

    Stage stage() const { return stage_; }
    int percent() const { return percent_; }
    const std::string& job_id() const { return job_id_; }
    const std::string& name() const { return name_; }
    /// `tool_change` while the job waits for the operator.
    const std::string& pause() const { return pause_; }
    /// A name for the report of the job on the card, free of anything that is already there.
    std::string report_base() const {
        std::string b = name_;
        std::size_t dot = b.rfind('.');
        if (dot != std::string::npos && dot > 0) b.resize(dot);
        return b + "-job";
    }

private:
    Stage stage_ = Stage::Nothing;
    std::string name_, text_, tools_, job_id_, pause_;
    bool route_ = false;
    int percent_ = 0;
};

}  // namespace hmi
