// EH3D-NODE-HMI-GUI - core/gsl3680.hpp: the parts of the GSL3680 touch controller that need no hardware: the check of the
// firmware table the user provides and the reading of a data packet. GPL-3.0-or-later.
//
// The register layout, the packet and the coordinate parameters of the JC8012P4A1C panel are adapted from the component
// guition/esp_lcd_touch_gsl3680, MIT licence, Copyright (c) 2026 esp_lcd_touch_gsl3680 contributors (its licence text is
// in docs/licenses/esp_lcd_touch_gsl3680.txt). The controller firmware of that component has no licence and is not here:
// not in this repository and not in its releases.
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace hmi {
namespace gsl {

constexpr std::uint8_t kAddress = 0x40;
constexpr std::uint8_t kPageRegister = 0xf0;
constexpr std::uint8_t kDataRegister = 0x80;
constexpr std::uint8_t kStatusRegister = 0xb0;
/// A record of the table file: the register (1 byte) and the value (4 bytes, least significant first).
constexpr std::size_t kRecordBytes = 5;
constexpr std::size_t kMaxTableBytes = 64 * 1024;
constexpr std::size_t kMaxContacts = 10;
constexpr std::size_t kPacketBytes = 4 + 4 * kMaxContacts;
constexpr const char* kTableName = "gsl3680.fw";

/// Whether the bytes are a table the controller can be given: whole records, not empty, not oversized, and every record
/// either selects a page or writes an aligned word below the data register.
inline bool table_ok(const std::string& bytes) {
    if (bytes.empty() || bytes.size() > kMaxTableBytes || bytes.size() % kRecordBytes != 0) return false;
    for (std::size_t i = 0; i < bytes.size(); i += kRecordBytes) {
        const std::uint8_t reg = static_cast<std::uint8_t>(bytes[i]);
        if (reg == kPageRegister) continue;
        if (reg >= kDataRegister || reg % 4 != 0) return false;
    }
    return true;
}

struct Touch {
    bool down = false;
    std::uint16_t x = 0;
    std::uint16_t y = 0;
};

inline std::uint16_t scale(std::int32_t raw, std::uint32_t gain, std::uint32_t range, std::uint32_t extent) {
    if (raw < 0) raw = 0;
    const std::uint32_t corrected = (static_cast<std::uint32_t>(raw) * gain + 2048u) / 4096u;
    std::uint32_t pixel = corrected * extent / range;
    if (pixel == 0) pixel = 1;
    if (pixel >= extent) pixel = extent - 1;
    return static_cast<std::uint16_t>(pixel);
}

/// The first finger of a data packet, in the pixels of the panel standing up (the horizontal axis is mirrored, as this
/// panel needs). Nothing is down when the packet is short, says an impossible number of contacts or holds only key data.
inline Touch decode(const std::uint8_t* packet, std::size_t length, std::uint16_t width = 800, std::uint16_t height = 1280) {
    Touch out;
    if (packet == nullptr || length < kPacketBytes || width < 2 || height < 2 || packet[0] > kMaxContacts) return out;
    for (std::size_t i = 0; i < packet[0]; ++i) {
        const std::uint8_t* p = packet + 4 + 4 * i;
        if (p[1] & 0xf0) continue;  // flags and key data, not a position
        const std::int32_t raw_y = p[0] | (static_cast<std::int32_t>(p[1] & 0x0f) << 8);
        const std::int32_t raw_x = p[2] | (static_cast<std::int32_t>(p[3] & 0x0f) << 8);
        out.down = true;
        // the firmware of this panel reports 14 x 26 channels of 64 units, with the axes swapped
        out.x = static_cast<std::uint16_t>(width - scale(raw_y - 2, 4119, 14 * 64, width));
        out.y = scale(raw_x, 4113, 26 * 64, height);
        return out;
    }
    return out;
}

}  // namespace gsl
}  // namespace hmi
