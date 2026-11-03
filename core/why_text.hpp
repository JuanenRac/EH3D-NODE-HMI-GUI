// EH3D-NODE-HMI-GUI - core/why_text.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The reasons the job workflow gives when it cannot do what the operator asked. They are plain identifiers (English words, never shown as they are): the screen looks each one up in
// status.hpp and shows it in the language of the operator.
#pragma once

namespace hmi {
namespace reasons {

inline constexpr const char* kNotAJobFile = "only drilling, routing and G-code files can be sent as a job from the card";
inline constexpr const char* kEmptyFile = "the file is empty";
inline constexpr const char* kNotStopped = "the job is not stopped";
inline constexpr const char* kOldServerNoFile = "the server is an old one and this screen has no file to send again";
inline constexpr const char* kNothingToGoOn = "there is nothing to go on from";
inline constexpr const char* kJobRunning = "a job is running, so the file cannot be changed";

}  // namespace reasons
}  // namespace hmi
