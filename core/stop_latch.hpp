// EH3D-NODE-HMI-GUI - core/stop_latch.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The stop asked from the screen, the voice or the hardware input, as a flag that the task of the stop reads. It does not travel in the queue of the touches: that queue holds eight,
// and a busy task leaves it full, so a stop that did not fit was lost without a word. The task of the stop takes the flag, sends the stop and, when the server could not be reached,
// puts the flag back and tries again after a wait (`stop_retry_after`), so the stop keeps being tried and the screen keeps saying that it has not been sent.
#pragma once

#include <atomic>
#include <cstdint>

namespace hmi {

class StopLatch {
public:
    /// The stop is asked (callable from any task, never waits).
    void ask() noexcept { asked_.store(true, std::memory_order_release); }
    /// The task of the stop takes it: true when a stop was asked since the last take. Two stops asked before it takes are one.
    bool take() noexcept { return asked_.exchange(false, std::memory_order_acq_rel); }
    bool pending() const noexcept { return asked_.load(std::memory_order_acquire); }
    /// The stop was not delivered: it is asked again.
    void give_back() noexcept { ask(); }

private:
    std::atomic<bool> asked_{false};
};

/// What follows a try of the stop: `round` is how many rounds have been made (the first is 1), `max_rounds` how many the settings allow (`stop_retries`) and `wait_ms` the wait between them.
struct StopRetry {
    bool retry = false;
    std::uint32_t wait_ms = 0;
};

inline StopRetry stop_retry_after(bool delivered, std::uint32_t round, std::uint32_t max_rounds, std::uint32_t wait_ms) {
    if (delivered || round > max_rounds) return {};
    return {true, wait_ms};
}

}  // namespace hmi
