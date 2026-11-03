// EH3D-NODE-HMI-GUI - core/wav.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// What the screen needs to say a notice: to read a WAV file of a recorded voice (16 bits, mono, any rate) and to make the tones itself when there is no recording (the beeps of the
// notices: a short rise for a check that is correct, three low falling notes for one that is not, the arpeggio at the end of a job, two bells for a tool). Both are pure, so they are
// tested on a computer; the codec of the board only receives the samples (main/board_p4.cpp).
#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "notices.hpp"

namespace hmi {

struct WavInfo {
    unsigned rate = 0;
    unsigned channels = 0;
    unsigned bits = 0;
    std::size_t data_offset = 0;
    std::size_t data_bytes = 0;
};

namespace detail {
inline unsigned le16(const std::string& s, std::size_t i) { return static_cast<unsigned char>(s[i]) | (static_cast<unsigned>(static_cast<unsigned char>(s[i + 1])) << 8); }
inline std::uint32_t le32(const std::string& s, std::size_t i) { return le16(s, i) | (static_cast<std::uint32_t>(le16(s, i + 2)) << 16); }
}  // namespace detail

/// Reads the header of a WAV file (PCM, 16 bits, mono, 8000 to 48000 Hz). False for anything else: the screen then plays the tone instead of noise.
inline bool parse_wav(const std::string& bytes, WavInfo& out) {
    if (bytes.size() < 44 || bytes.compare(0, 4, "RIFF") != 0 || bytes.compare(8, 4, "WAVE") != 0) return false;
    WavInfo info;
    bool have_format = false;
    std::size_t pos = 12;
    while (pos + 8 <= bytes.size()) {
        const std::uint32_t size = detail::le32(bytes, pos + 4);
        if (bytes.compare(pos, 4, "fmt ") == 0 && size >= 16 && pos + 8 + 16 <= bytes.size()) {
            if (detail::le16(bytes, pos + 8) != 1) return false;  // PCM only
            info.channels = detail::le16(bytes, pos + 10);
            info.rate = detail::le32(bytes, pos + 12);
            info.bits = detail::le16(bytes, pos + 22);
            have_format = true;
        } else if (bytes.compare(pos, 4, "data") == 0) {
            if (!have_format) return false;
            info.data_offset = pos + 8;
            // a file cut short plays what is there; a header that says more than is there is not trusted
            info.data_bytes = std::min<std::size_t>(size, bytes.size() - info.data_offset) & ~static_cast<std::size_t>(1);
            if (info.channels != 1 || info.bits != 16 || info.rate < 8000 || info.rate > 48000 || info.data_bytes == 0) return false;
            out = info;
            return true;
        }
        // a chunk that ends past the end of the file leaves nothing more to look at (and on a 32-bit chip its size would wrap the position back onto itself)
        const std::uint64_t next = static_cast<std::uint64_t>(pos) + 8 + size + (size & 1);
        if (next > bytes.size()) break;
        pos = static_cast<std::size_t>(next);
    }
    return false;
}

/// The samples of the data of a WAV file that `parse_wav` accepted.
inline std::vector<std::int16_t> wav_samples(const std::string& bytes, const WavInfo& info) {
    std::vector<std::int16_t> out(info.data_bytes / 2);
    for (std::size_t i = 0; i < out.size(); ++i) out[i] = static_cast<std::int16_t>(detail::le16(bytes, info.data_offset + i * 2));
    return out;
}

struct Note {
    float hz;  // 0 is a silence
    unsigned ms;
};

/// The notes of the tone of a cue.
inline std::vector<Note> tone_notes(Cue cue) {
    switch (cue) {
        case Cue::Finished: return {{523.25f, 140}, {659.25f, 140}, {783.99f, 140}, {1046.5f, 420}};
        case Cue::ToolChange: return {{880.0f, 160}, {0.0f, 60}, {880.0f, 160}, {1174.66f, 300}};
        case Cue::CheckOk: return {{659.25f, 110}, {987.77f, 110}, {1318.5f, 320}};
        case Cue::CheckNotOk: return {{392.0f, 260}, {0.0f, 80}, {329.63f, 260}, {0.0f, 80}, {261.63f, 440}};
    }
    return {};
}

/// The tone as samples (16 bits, mono), with a soft start and a long fade so it does not click; `volume_percent` scales it.
inline std::vector<std::int16_t> tone_samples(Cue cue, unsigned rate, unsigned volume_percent) {
    std::vector<std::int16_t> out;
    const double gain = 22000.0 * (volume_percent > 100 ? 100 : volume_percent) / 100.0;
    for (const Note& note : tone_notes(cue)) {
        const std::size_t n = static_cast<std::size_t>(static_cast<std::uint64_t>(rate) * note.ms / 1000);
        for (std::size_t i = 0; i < n; ++i) {
            const double t = static_cast<double>(i) / rate;
            const double env = std::fmin(static_cast<double>(i) / 200.0, 1.0) * std::pow(1.0 - static_cast<double>(i) / static_cast<double>(n), 0.6);
            const double v = note.hz == 0.0f ? 0.0 : std::sin(t * note.hz * 6.283185307179586) * 0.55 + std::sin(t * note.hz * 2.0 * 6.283185307179586) * 0.12;
            out.push_back(static_cast<std::int16_t>(v * env * gain));
        }
    }
    return out;
}

/// The same samples with the volume of the settings (a recorded voice is scaled; the tone is made at the volume asked).
inline void scale_samples(std::vector<std::int16_t>& samples, unsigned volume_percent) {
    const int v = static_cast<int>(volume_percent > 100 ? 100 : volume_percent);
    for (auto& s : samples) s = static_cast<std::int16_t>(static_cast<int>(s) * v / 100);
}

}  // namespace hmi
