// EH3D-NODE-HMI-GUI host tests: the notices that a recorded voice says (the end of a job, the request for a tool) and where their files are.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "../core/hmi_config.hpp"
#include "../core/notices.hpp"
#include "../core/wav.hpp"

using namespace hmi;

static int failures = 0;
static int checks = 0;

#define CHECK(cond)                                                     \
    do {                                                                \
        ++checks;                                                       \
        if (!(cond)) {                                                  \
            ++failures;                                                 \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond); \
        }                                                               \
    } while (0)

int main() {
    NoticeSetting n;
    CHECK(parse_notice("off", n) && n.kind == NoticeSetting::Kind::Off);
    CHECK(parse_notice("default", n) && n.kind == NoticeSetting::Kind::Tone);
    CHECK(parse_notice("voice:es:f", n) && n.kind == NoticeSetting::Kind::Voice && n.lang == Lang::Es && n.female);
    CHECK(parse_notice("voice:de:m", n) && n.lang == Lang::De && !n.female);
    for (const char* bad : {"", "voice", "voice:xx:f", "voice:es:x", "voice:es", "voice:es:f:x", "Voice:es:f", "tone:3", "voice:ES:f"}) CHECK(!parse_notice(bad, n));

    // every language and gender comes back from its text and has its two files
    for (std::size_t i = 0; i < kLangCount; ++i) {
        for (bool female : {true, false}) {
            NoticeSetting s{NoticeSetting::Kind::Voice, static_cast<Lang>(i), female};
            NoticeSetting back;
            CHECK(parse_notice(notice_text(s), back) && back.lang == s.lang && back.female == female);
            std::string base = std::string("/sdcard/voices/") + lang_code(s.lang) + "/" + (female ? "f" : "m") + "/";
            CHECK(notice_file("/sdcard", s, Cue::Finished) == base + "finished.wav");
            CHECK(notice_file("/sdcard", s, Cue::ToolChange) == base + "tool.wav");
        }
    }
    CHECK(notice_file("/sdcard", NoticeSetting{}, Cue::Finished).empty());
    CHECK(notice_text(NoticeSetting{}) == "off");

    // the settings file
    std::vector<Problem> p;
    Settings d = parse_settings("", p);
    CHECK(p.empty() && d.sound_finish.kind == NoticeSetting::Kind::Off && d.sound_tool_change.kind == NoticeSetting::Kind::Off);
    Settings s = parse_settings("sound_finish = voice:it:m\nsound_tool_change = voice:fr:f\n", p);
    CHECK(p.empty() && s.sound_finish.lang == Lang::It && !s.sound_finish.female && s.sound_tool_change.lang == Lang::Fr && s.sound_tool_change.female);
    p.clear();
    Settings b = parse_settings("sound_finish = voice:it\nsound_tool_change = loud\n", p);
    CHECK(p.size() == 2 && b.sound_finish.kind == NoticeSetting::Kind::Off && b.sound_tool_change.kind == NoticeSetting::Kind::Off);

    // the visual check of the board: off until the person chooses, a recorded voice or the tone, and the files of the voices
    p.clear();
    CHECK(d.sound_check_ok.kind == NoticeSetting::Kind::Off && d.sound_check_bad.kind == NoticeSetting::Kind::Off);
    Settings k = parse_settings("sound_check_ok = voice:es:f\nsound_check_bad = default\n", p);
    CHECK(p.empty() && k.sound_check_ok.lang == Lang::Es && k.sound_check_ok.female && k.sound_check_bad.kind == NoticeSetting::Kind::Tone);
    Settings kb = parse_settings("sound_check_ok = voice:xx:f\nsound_check_bad = loud\n", p);
    CHECK(p.size() == 2 && kb.sound_check_ok.kind == NoticeSetting::Kind::Off && kb.sound_check_bad.kind == NoticeSetting::Kind::Off);
    CHECK(notice_file("/sdcard", k.sound_check_ok, Cue::CheckOk) == "/sdcard/voices/es/f/check_ok.wav");
    CHECK(notice_file("/sdcard", k.sound_check_ok, Cue::CheckNotOk) == "/sdcard/voices/es/f/check_bad.wav");
    CHECK(notice_file("/sdcard", k.sound_check_ok, Cue::Finished) == "/sdcard/voices/es/f/finished.wav");
    CHECK(notice_file("/sdcard", k.sound_check_bad, Cue::CheckNotOk).empty());  // the tone has no file

    // the recordings: a 16-bit mono WAV is read, anything else is refused (the screen then plays the tone instead of noise)
    auto wav = [](unsigned channels, unsigned bits, unsigned rate, const std::vector<std::int16_t>& samples) {
        std::string s = "RIFF";
        auto put32 = [&s](std::uint32_t v) { for (int i = 0; i < 4; ++i) s += static_cast<char>((v >> (8 * i)) & 0xFF); };
        auto put16 = [&s](unsigned v) { for (int i = 0; i < 2; ++i) s += static_cast<char>((v >> (8 * i)) & 0xFF); };
        put32(static_cast<std::uint32_t>(36 + samples.size() * 2));
        s += "WAVEfmt ";
        put32(16);
        put16(1);
        put16(channels);
        put32(rate);
        put32(rate * channels * bits / 8);
        put16(channels * bits / 8);
        put16(bits);
        s += "data";
        put32(static_cast<std::uint32_t>(samples.size() * 2));
        for (std::int16_t v : samples) put16(static_cast<unsigned>(static_cast<std::uint16_t>(v)));
        return s;
    };
    WavInfo info;
    const std::string good = wav(1, 16, 22050, {0, 1000, -1000, 32767});
    CHECK(parse_wav(good, info) && info.rate == 22050 && info.channels == 1 && info.bits == 16 && info.data_bytes == 8 && info.data_offset == 44);
    std::vector<std::int16_t> got = wav_samples(good, info);
    CHECK(got.size() == 4 && got[1] == 1000 && got[2] == -1000 && got[3] == 32767);
    CHECK(!parse_wav(wav(2, 16, 22050, {1, 2}), info));      // stereo
    CHECK(!parse_wav(wav(1, 8, 22050, {1, 2}), info));       // not 16 bits
    CHECK(!parse_wav(wav(1, 16, 4000, {1, 2}), info));       // a rate the codec is not asked for
    CHECK(!parse_wav(wav(1, 16, 22050, {}), info));          // no samples
    CHECK(!parse_wav("not a wav file at all, not even close to it, just text of some length", info));
    CHECK(!parse_wav("", info));
    // a header that says more than the file has plays what is there
    std::string cut = good.substr(0, good.size() - 4);
    CHECK(parse_wav(cut, info) && info.data_bytes == 4);
    // the volume scales the samples
    std::vector<std::int16_t> loud = {10000, -10000};
    scale_samples(loud, 50);
    CHECK(loud[0] == 5000 && loud[1] == -5000);
    scale_samples(loud, 0);
    CHECK(loud[0] == 0 && loud[1] == 0);

    // the tones: each cue has its own, the check that is correct rises and the one that is not falls, and the volume is the volume
    const std::vector<std::int16_t> ok = tone_samples(Cue::CheckOk, 22050, 60), bad = tone_samples(Cue::CheckNotOk, 22050, 60);
    CHECK(!ok.empty() && !bad.empty() && ok != bad);
    CHECK(ok.size() == static_cast<std::size_t>(22050 * 110 / 1000) * 2 + static_cast<std::size_t>(22050 * 320 / 1000));
    CHECK(tone_samples(Cue::Finished, 22050, 60) != ok && tone_samples(Cue::ToolChange, 22050, 60) != bad);
    CHECK(tone_notes(Cue::CheckOk).back().hz > tone_notes(Cue::CheckOk).front().hz);
    CHECK(tone_notes(Cue::CheckNotOk).back().hz < tone_notes(Cue::CheckNotOk).front().hz);
    int peak = 0;
    for (std::int16_t v : ok) peak = std::max(peak, std::abs(static_cast<int>(v)));
    CHECK(peak > 3000 && peak < 22000);
    const std::vector<std::int16_t> silent = tone_samples(Cue::CheckOk, 22050, 0);
    CHECK(!silent.empty() && std::all_of(silent.begin(), silent.end(), [](std::int16_t v) { return v == 0; }));
    int peak_full = 0;
    for (std::int16_t v : tone_samples(Cue::CheckOk, 22050, 100)) peak_full = std::max(peak_full, std::abs(static_cast<int>(v)));
    CHECK(peak_full > peak);

    std::printf("%d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
