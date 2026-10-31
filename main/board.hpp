// EH3D-NODE-HMI-GUI - main/board.hpp: what the application needs from the module (display, touch, stop input, sound).
// GPL-3.0-or-later.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../core/sdfiles.hpp"

namespace board {

/// Starts the display and the touch and the LVGL port; false when the panel does not answer.
bool display_start();
/// Takes and gives back the lock of LVGL (every call into LVGL from another task is between the two).
void lvgl_lock();
void lvgl_unlock();
/// Brightness of the backlight, 10 to 100.
void set_brightness(unsigned percent);
/// Starts the codec, the speaker and the microphone; false when the codec does not answer.
bool audio_start();
/// Reads the hardware stop input: `wired` is false when no pin is set.
void read_hardware_stop(bool& wired, bool& pressed);
/// Turns the picture to the layout: 0 (standing up) or 90 degrees (lying down).
void set_rotation(int degrees);
/// Mounts the microSD card; false when there is no card or it cannot be read.
bool sd_mount();
/// The files of the card (the root folder), as the list shows them.
std::vector<hmi::Entry> sd_list();
/// Reads a file of the card whole (at most `max_bytes`); the name must be a safe one.
bool sd_read(const std::string& name, std::string& out, std::size_t max_bytes);
/// Writes a file on the card (replacing it); the name must be a safe one.
bool sd_write(const std::string& name, const std::string& text);
/// Milliseconds since start (it wraps around after 49 days; the gate compares differences).
std::uint32_t now_ms();

}  // namespace board
