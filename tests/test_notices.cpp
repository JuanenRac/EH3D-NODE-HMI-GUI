// EH3D-NODE-HMI-GUI host tests: the notices that a recorded voice says (the end of a job, the request for a tool) and where their files are.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdio>
#include <string>
#include <vector>

#include "../core/hmi_config.hpp"
#include "../core/notices.hpp"

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

    std::printf("%d checks, %d failed\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
