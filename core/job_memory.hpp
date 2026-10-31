// EH3D-NODE-HMI-GUI - core/job_memory.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// What the screen keeps of the job it is following, so that after a power cut or a reset it follows the same job again: the id the server gave it, the machine and
// its name. The job itself lives in the server (it keeps its state and marks as failed a job that was running when it stopped); the screen only needs to know which
// one was its own. Nothing here touches the flash: the application stores the text.
#pragma once

#include <string>

namespace hmi {

struct JobMemory {
    std::string job_id;
    std::string machine;
    std::string name;
};

inline std::string memory_clean(std::string s) {
    for (char& c : s) {
        if (c == '\n' || c == '\r') c = ' ';
    }
    return s;
}

/// The text to store: one `key=value` per line.
inline std::string memory_text(const JobMemory& m) {
    return "job=" + memory_clean(m.job_id) + "\nmachine=" + memory_clean(m.machine) + "\nname=" + memory_clean(m.name) + "\n";
}

/// Reads what `memory_text` wrote. False when there is no job id (nothing to follow), so a damaged text follows nothing.
inline bool memory_parse(const std::string& text, JobMemory& out) {
    JobMemory m;
    std::size_t pos = 0;
    while (pos < text.size()) {
        std::size_t end = text.find('\n', pos);
        if (end == std::string::npos) end = text.size();
        const std::string line = text.substr(pos, end - pos);
        const std::size_t eq = line.find('=');
        if (eq != std::string::npos) {
            const std::string k = line.substr(0, eq), v = line.substr(eq + 1);
            if (k == "job") m.job_id = v;
            else if (k == "machine") m.machine = v;
            else if (k == "name") m.name = v;
        }
        pos = end + 1;
    }
    if (m.job_id.empty() || m.job_id.size() > 64) return false;
    out = m;
    return true;
}

}  // namespace hmi
