// EH3D-NODE-HMI-GUI - more host tests of the small rules of the screen: the languages and their fall-backs, the sound files that an operator puts on the card, the notices, the memory of
// the job that is running, the turn of the panel and the files of the card.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdint>
#include <cstdio>
#include <set>
#include <string>
#include <vector>

#include "../core/job_memory.hpp"
#include "../core/lang.hpp"
#include "../core/notices.hpp"
#include "../core/orientation.hpp"
#include "../core/sdfiles.hpp"
#include "../core/wav.hpp"

static int failures = 0;
static int checks = 0;
#define CHECK(condition)                                                \
  do {                                                                  \
    ++checks;                                                           \
    if (!(condition)) {                                                 \
      ++failures;                                                       \
      std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #condition);  \
    }                                                                   \
  } while (0)

using namespace hmi;

// ---- the languages ----------------------------------------------------------------------------------------------------------------------

static void every_language_has_one_code_that_reads_back() {
  std::set<std::string> seen;
  for (std::size_t i = 0; i < kLangCount; ++i) {
    const Lang l = static_cast<Lang>(i);
    const std::string code = lang_code(l);
    Lang back = Lang::Ja;
    CHECK(lang_from_code(code, back) && back == l);
    CHECK(!code.empty() && seen.insert(code).second);
    for (const char c : code) CHECK((c >= 'a' && c <= 'z') || c == '-');
  }
  CHECK(seen.size() == 37 && static_cast<std::size_t>(Lang::FrCa) + 1 == kLangCount && kBaseLangCount == 8);
}

static void a_code_that_is_not_exact_is_not_a_language() {
  Lang l = Lang::De;
  for (const char* bad : {"", "EN", "En", "en ", " en", "english", "pt_br", "PT-BR", "fr-CA", "e", "zh-cn", "\xE6\x97\xA5"}) CHECK(!lang_from_code(bad, l));
  CHECK(l == Lang::De);                                              // a refusal leaves the answer alone
}

static void the_variants_and_the_languages_without_a_table_fall_back_to_a_table() {
  for (std::size_t i = 0; i < kBaseLangCount; ++i) CHECK(lang_fallback(static_cast<Lang>(i)) == static_cast<Lang>(i));
  CHECK(lang_fallback(Lang::PtBr) == Lang::Pt && lang_fallback(Lang::FrCa) == Lang::Fr);
  for (std::size_t i = kBaseLangCount; i < kLangCount - 2; ++i) CHECK(lang_fallback(static_cast<Lang>(i)) == Lang::En);
  const Row row = {"en", "es", "it", "de", "fr", "pt", "zh", "ja"};
  CHECK(std::string(pick(row, Lang::PtBr)) == "pt" && std::string(pick(row, Lang::FrCa)) == "fr" && std::string(pick(row, Lang::Ru)) == "en" && std::string(pick(row, Lang::Ja)) == "ja");
}

// ---- the notices -------------------------------------------------------------------------------------------------------------------------

static void a_notice_setting_reads_back_for_every_language_and_voice() {
  for (std::size_t i = 0; i < kLangCount; ++i) {
    for (const bool female : {true, false}) {
      const NoticeSetting s{NoticeSetting::Kind::Voice, static_cast<Lang>(i), female};
      NoticeSetting back;
      CHECK(parse_notice(notice_text(s), back) && back.kind == NoticeSetting::Kind::Voice && back.lang == s.lang && back.female == female);
    }
  }
  NoticeSetting n;
  CHECK(parse_notice("off", n) && n.kind == NoticeSetting::Kind::Off && notice_text(n) == "off");
  CHECK(parse_notice("default", n) && n.kind == NoticeSetting::Kind::Tone && notice_text(n) == "default");
  CHECK(parse_notice("voice:pt-br:f", n) && n.lang == Lang::PtBr && n.female);
}

static void a_notice_that_is_not_written_exactly_is_refused_and_leaves_the_answer_alone() {
  NoticeSetting keep{NoticeSetting::Kind::Tone, Lang::De, false};
  for (const char* bad : {"", "Off", "OFF", "off ", " off", "default ", "voice", "voice:", "voice::f", "voice:en", "voice:en:", "voice:en:x", "voice:en:F", "voice:xx:f", "voice:EN:f", "voice:en:f ", "voice:en:ff", "voice:en-:f", "tone", "voice:en:f:extra"}) {
    NoticeSetting n = keep;
    CHECK(!parse_notice(bad, n) && n.kind == keep.kind && n.lang == keep.lang && n.female == keep.female);
  }
}

static void the_file_of_a_cue_depends_on_the_voice_only() {
  const NoticeSetting voice{NoticeSetting::Kind::Voice, Lang::PtBr, false};
  CHECK(notice_file("/sdcard", voice, Cue::Finished) == "/sdcard/voices/pt-br/m/finished.wav");
  CHECK(notice_file("/sdcard", voice, Cue::ToolChange) == "/sdcard/voices/pt-br/m/tool.wav");
  CHECK(notice_file("/sdcard", voice, Cue::CheckOk) == "/sdcard/voices/pt-br/m/check_ok.wav");
  CHECK(notice_file("/sdcard", voice, Cue::CheckNotOk) == "/sdcard/voices/pt-br/m/check_bad.wav");
  CHECK(notice_file("", NoticeSetting{NoticeSetting::Kind::Voice, Lang::En, true}, Cue::Finished) == "/voices/en/f/finished.wav");
  for (const Cue c : {Cue::Finished, Cue::ToolChange, Cue::CheckOk, Cue::CheckNotOk}) {
    CHECK(notice_file("/sdcard", NoticeSetting{}, c).empty() && notice_file("/sdcard", NoticeSetting{NoticeSetting::Kind::Tone, Lang::En, true}, c).empty());
  }
}

// ---- the sound files of the card ---------------------------------------------------------------------------------------------------------

static std::string wav_file(unsigned format, unsigned channels, unsigned rate, unsigned bits, const std::string& data, const std::string& before_data = "", bool data_first = false) {
  std::string body = "WAVE";
  const auto put16 = [](std::string& s, unsigned v) { s += static_cast<char>(v & 255); s += static_cast<char>((v >> 8) & 255); };
  const auto put32 = [&](std::string& s, std::uint32_t v) { put16(s, v & 0xFFFF); put16(s, v >> 16); };
  std::string fmt;
  put16(fmt, format); put16(fmt, channels); put32(fmt, rate); put32(fmt, rate * channels * bits / 8); put16(fmt, channels * bits / 8); put16(fmt, bits);
  std::string fmt_chunk = "fmt ";
  put32(fmt_chunk, 16);
  fmt_chunk += fmt;
  std::string data_chunk = "data";
  put32(data_chunk, static_cast<std::uint32_t>(data.size()));
  data_chunk += data;
  body += data_first ? data_chunk + before_data + fmt_chunk : fmt_chunk + before_data + data_chunk;
  std::string out = "RIFF";
  put32(out, static_cast<std::uint32_t>(body.size()));
  return out + body;
}

static std::string chunk(const char* id, const std::string& payload) {
  std::string c = id;
  const std::uint32_t n = static_cast<std::uint32_t>(payload.size());
  for (int s = 0; s < 32; s += 8) c += static_cast<char>((n >> s) & 255);
  c += payload;
  if (payload.size() & 1) c += '\0';           // chunks are padded to an even size
  return c;
}

static void the_data_is_found_after_other_chunks_and_their_padding() {
  const std::string pcm = std::string("\x01\x00\x02\x00\x03\x00", 6);
  WavInfo info;
  CHECK(parse_wav(wav_file(1, 1, 22050, 16, pcm), info) && info.data_offset == 44 && info.data_bytes == 6);
  const std::string list = chunk("LIST", "INFOISFT\x05\x00\x00\x00Lavf5");        // an odd size, so a pad byte follows it
  CHECK(parse_wav(wav_file(1, 1, 22050, 16, pcm, list), info) && info.data_offset == 44 + list.size() && info.data_bytes == 6);
  const std::vector<std::int16_t> s = wav_samples(wav_file(1, 1, 22050, 16, pcm, list), info);
  CHECK(s.size() == 3 && s[0] == 1 && s[1] == 2 && s[2] == 3);
  CHECK(!parse_wav(wav_file(1, 1, 22050, 16, pcm, "", true), info));                  // the data before the format: nothing says how to play it
}

static void only_plain_mono_16_bit_pcm_between_8_and_48_khz_is_played() {
  const std::string pcm(8, '\x10');
  WavInfo info;
  CHECK(parse_wav(wav_file(1, 1, 8000, 16, pcm), info) && parse_wav(wav_file(1, 1, 48000, 16, pcm), info));
  CHECK(!parse_wav(wav_file(1, 1, 7999, 16, pcm), info) && !parse_wav(wav_file(1, 1, 48001, 16, pcm), info));
  CHECK(!parse_wav(wav_file(3, 1, 22050, 16, pcm), info));                             // float
  CHECK(!parse_wav(wav_file(0xFFFE, 1, 22050, 16, pcm), info));                        // the extensible form
  CHECK(!parse_wav(wav_file(1, 0, 22050, 16, pcm), info) && !parse_wav(wav_file(1, 2, 22050, 16, pcm), info));
  CHECK(!parse_wav(wav_file(1, 1, 22050, 24, pcm), info) && !parse_wav(wav_file(1, 1, 22050, 8, pcm), info));
}

static void a_data_chunk_of_odd_size_or_larger_than_the_file_plays_what_is_whole() {
  WavInfo info;
  CHECK(parse_wav(wav_file(1, 1, 22050, 16, std::string(7, 'x')), info) && info.data_bytes == 6);
  std::string whole = wav_file(1, 1, 22050, 16, std::string(100, 'x'));
  CHECK(parse_wav(whole.substr(0, whole.size() - 41), info) && info.data_bytes == 58);
  CHECK(!parse_wav(whole.substr(0, 43), info) && !parse_wav(whole.substr(0, 44), info));   // a header and no sound
  CHECK(parse_wav(whole.substr(0, 46), info) && info.data_bytes == 2);
  std::string riff = whole;
  riff[0] = 'X';
  CHECK(!parse_wav(riff, info));
  std::string wave = whole;
  wave[8] = 'Q';
  CHECK(!parse_wav(wave, info));
}

static void a_chunk_that_says_it_is_bigger_than_the_file_does_not_make_the_search_loop() {
  WavInfo info;
  for (const std::uint32_t size : {0xFFFFFFF8u, 0xFFFFFFFFu, 0xFFFFFFF0u, 0x80000000u, 0x7FFFFFFFu}) {
    std::string junk = "JUNK";
    for (int s = 0; s < 32; s += 8) junk += static_cast<char>((size >> s) & 255);
    junk += "xxxxxxxx";
    CHECK(!parse_wav(wav_file(1, 1, 22050, 16, std::string(8, 'x'), junk), info));   // no way to the data: refused, and it comes back
  }
  // the same chunk, between the format and the data, makes the data unreachable
  std::string junk = "JUNK";
  for (int s = 0; s < 32; s += 8) junk += static_cast<char>((0xFFFFFFF8u >> s) & 255);
  CHECK(!parse_wav(wav_file(1, 1, 22050, 16, std::string(8, 'x'), junk), info));
}

static void the_tones_follow_the_volume_and_the_rate() {
  for (const Cue c : {Cue::Finished, Cue::ToolChange, Cue::CheckOk, Cue::CheckNotOk}) {
    std::size_t expected = 0;
    for (const Note& n : tone_notes(c)) expected += static_cast<std::size_t>(22050ull * n.ms / 1000);
    CHECK(tone_samples(c, 22050, 70).size() == expected && !tone_notes(c).empty());
    CHECK(tone_samples(c, 22050, 100) == tone_samples(c, 22050, 1000));                     // more than all of it is all of it
    CHECK(tone_samples(c, 22050, 70) == tone_samples(c, 22050, 70));
    CHECK(tone_samples(c, 11025, 70).size() * 2 <= expected + 12 && tone_samples(c, 11025, 70).size() * 2 + 12 >= expected);
  }
  std::vector<std::int16_t> ramp = {32767, -32768, 1, -1, 0};
  scale_samples(ramp, 100);
  CHECK(ramp[0] == 32767 && ramp[1] == -32768 && ramp[2] == 1 && ramp[3] == -1 && ramp[4] == 0);
  scale_samples(ramp, 50);
  CHECK(ramp[0] > 16000 && ramp[0] < 16400 && ramp[1] < -16000 && ramp[1] > -16400);
  std::vector<std::int16_t> none;
  scale_samples(none, 50);
  CHECK(none.empty());
}

// ---- the memory of the job ---------------------------------------------------------------------------------------------------------------

static void the_job_that_is_running_is_remembered_and_read_back() {
  const JobMemory m{"job-7", "bungard-1", "Placa ñandú 日本"};
  JobMemory back;
  CHECK(memory_parse(memory_text(m), back) && back.job_id == "job-7" && back.machine == "bungard-1" && back.name == m.name);
  const JobMemory breaks{"j", "a\nb", "x\r\ny=z"};
  CHECK(memory_parse(memory_text(breaks), back) && back.job_id == "j" && back.machine == "a b" && back.name == "x  y=z");   // a line break cannot add a key
  CHECK(memory_text(breaks).find("\r") == std::string::npos);
  CHECK(memory_parse("job=a=b\n", back) && back.job_id == "a=b");
  CHECK(memory_parse("name=n\njob=first\njob=second\nflavour=x\nno equals sign\n", back) && back.job_id == "second" && back.name == "n" && back.machine.empty());
  CHECK(memory_parse("job=" + std::string(64, 'j'), back) && back.job_id.size() == 64);
  JobMemory keep{"keep", "", ""};
  for (const std::string& bad : {std::string(""), std::string("name=x\n"), std::string("job=\n"), std::string("job=" + std::string(65, 'j')), std::string("garbage"), std::string("Job=abc\n")}) {
    JobMemory x = keep;
    CHECK(!memory_parse(bad, x) && x.job_id == "keep");
  }
}

// ---- the turn of the panel ----------------------------------------------------------------------------------------------------------------

static void the_touch_of_the_panel_lands_on_the_screen_that_is_drawn_whatever_the_turn() {
  for (const Orientation o : {Orientation::Landscape, Orientation::Portrait}) {
    const Layout l = layout_for(o, false);
    CHECK(l.width * l.height == kPanelWidth * kPanelHeight && (l.portrait ? l.width < l.height : l.width > l.height));
    std::set<std::pair<int, int>> hit;
    for (int px = 0; px < kPanelWidth; px += 37) {
      for (int py = 0; py < kPanelHeight; py += 41) {
        int x = -1, y = -1;
        touch_to_screen(l, px, py, x, y);
        CHECK(x >= 0 && x < l.width && y >= 0 && y < l.height);
        hit.insert({x, y});
      }
    }
    CHECK(hit.size() == static_cast<std::size_t>(((kPanelWidth + 36) / 37) * ((kPanelHeight + 40) / 41)));   // two touches never land on the same dot
  }
  int x = 0, y = 0;
  touch_to_screen(layout_for(Orientation::Landscape, false), kPanelWidth - 1, 0, x, y);
  CHECK(x == 0 && y == 0);                                           // the top right of the panel is the top left of the turned screen
  touch_to_screen(layout_for(Orientation::Landscape, false), 0, kPanelHeight - 1, x, y);
  CHECK(x == kPanelHeight - 1 && y == kPanelWidth - 1);
}

static void the_columns_fit_in_the_screen_with_their_gaps() {
  for (const Orientation o : {Orientation::Landscape, Orientation::Portrait, Orientation::Auto}) {
    for (const bool mounted : {true, false}) {
      const Layout l = layout_for(o, mounted);
      for (const int gap : {0, 6, 14, 24}) {
        const int w = column_width(l, gap);
        CHECK(w > 0 && w * l.columns + gap * (l.columns + 1) <= l.width && w * l.columns + gap * (l.columns + 1) > l.width - l.columns);
      }
    }
  }
  CHECK(layout_for(Orientation::Auto, true).portrait && !layout_for(Orientation::Auto, false).portrait);
  CHECK(layout_for(Orientation::Landscape, true).columns == 3 && layout_for(Orientation::Portrait, false).columns == 1);
  Orientation o = Orientation::Auto;
  for (const char* bad : {"", "Auto", "AUTO", "land", "portrait ", "vertical"}) CHECK(!orientation_from_text(bad, o) && o == Orientation::Auto);
  for (const char* good : {"auto", "landscape", "portrait"}) { CHECK(orientation_from_text(good, o)); CHECK(std::string(orientation_text(o)) == good); }
}

// ---- the files of the card -----------------------------------------------------------------------------------------------------------------

static void the_kind_of_a_file_is_its_last_extension_in_any_case() {
  CHECK(kind_of_file("A.NCD") == FileKind::Drill && kind_of_file("x.Drl") == FileKind::Drill && kind_of_file("x.xln") == FileKind::Drill && kind_of_file("x.EXC") == FileKind::Drill);
  CHECK(kind_of_file("a.plt") == FileKind::Route && kind_of_file("a.HPGL") == FileKind::Route);
  CHECK(kind_of_file("pcb.GKO") == FileKind::Gerber && kind_of_file("pcb.gbr") == FileKind::Gerber && kind_of_file("a.gm1") == FileKind::Gerber && kind_of_file("a.GML") == FileKind::Gerber);
  CHECK(kind_of_file("a.dxf") == FileKind::Dxf && kind_of_file("a.b.ZIP") == FileKind::Archive);
  CHECK(kind_of_file("a.nc") == FileKind::Gcode && kind_of_file("a.GCODE") == FileKind::Gcode && kind_of_file("a.tap") == FileKind::Gcode);
  CHECK(kind_of_file("machine.cfg") == FileKind::Settings && kind_of_file("a.txt") == FileKind::Other);
  for (const char* odd : {"", "noext", "a.", ".zip", ".ncd", "zip", "a.ncd.bak", "a. ncd", "a.ncd "}) CHECK(kind_of_file(odd) == FileKind::Other);
  CHECK(is_job_file("a.nc") && is_job_file("x.zip") && !is_job_file("m.cfg") && !is_job_file("readme.txt") && !is_job_file(".nc"));
}

static void the_names_that_could_leave_the_card_or_break_it_are_not_names() {
  CHECK(safe_file_name("pieza ñandú 日本.nc") && safe_file_name(std::string(64, 'a')) && safe_file_name("a b.nc") && safe_file_name("a-b_c(1).nc"));
  for (const std::string& bad : {std::string(""), std::string(".x"), std::string(".."), std::string("x."), std::string("x "), std::string(65, 'a'), std::string("a/b"), std::string("..\\x"), std::string("a:b"), std::string("a*"), std::string("a?"), std::string("a\"b"),
                                std::string("a<b"), std::string("a>b"), std::string("a|b"), std::string("a\tb"), std::string("a\nb"), std::string("a\0b", 3)})
    CHECK(!safe_file_name(bad));
  CHECK(sd_path("a.nc") == "/sdcard/a.nc" && sd_path("../a.nc").empty() && sd_path("").empty() && sd_path("a/b").empty());
}

static void a_listing_has_the_folders_first_and_leaves_out_what_cannot_be_opened() {
  std::vector<Entry> in = {{"b.nc", false, 10}, {"Zeta", true, 0}, {"alpha", true, 0}, {"A.nc", false, 5}, {"..", true, 0}, {"x/y", false, 1}, {".hidden", false, 1}, {"c.nc", false, 1}};
  const std::vector<Entry> out = sorted_listing(in);
  CHECK(out.size() == 5);
  CHECK(out[0].name == "alpha" && out[0].folder && out[1].name == "Zeta" && out[1].folder);
  CHECK(out[2].name == "A.nc" && out[3].name == "b.nc" && out[4].name == "c.nc");
  CHECK(sorted_listing({}).empty());
}

static void sizes_are_written_with_the_unit_that_fits() {
  CHECK(size_text(0) == "0 B" && size_text(1023) == "1023 B" && size_text(1024) == "1.0 KB" && size_text(1536) == "1.5 KB");
  CHECK(size_text(1024ull * 1024) == "1.0 MB" && size_text(5ull * 1024 * 1024 + 524288) == "5.5 MB");
  CHECK(size_text(40ull * 1024 * 1024 * 1024) == "40960.0 MB");                         // the card is not read in gigabytes: the number is still right
}

static void a_free_name_is_the_first_one_that_is_not_taken() {
  const auto taken = [](const std::set<std::string>& names) { return [names](const std::string& n) { return names.count(n) > 0; }; };
  CHECK(free_name("job", "nc", taken({})) == "job.nc");
  CHECK(free_name("job", "nc", taken({"job.nc"})) == "job-2.nc");
  CHECK(free_name("job", "nc", taken({"job.nc", "job-2.nc", "job-3.nc"})) == "job-4.nc");
  CHECK(free_name("job", "nc", taken({"job-2.nc"})) == "job.nc");
  CHECK(free_name("a", "b", taken({"a.b", "a-2.b", "a-4.b"})) == "a-3.b");
}

int main() {
  every_language_has_one_code_that_reads_back();
  a_code_that_is_not_exact_is_not_a_language();
  the_variants_and_the_languages_without_a_table_fall_back_to_a_table();
  a_notice_setting_reads_back_for_every_language_and_voice();
  a_notice_that_is_not_written_exactly_is_refused_and_leaves_the_answer_alone();
  the_file_of_a_cue_depends_on_the_voice_only();
  the_data_is_found_after_other_chunks_and_their_padding();
  only_plain_mono_16_bit_pcm_between_8_and_48_khz_is_played();
  a_data_chunk_of_odd_size_or_larger_than_the_file_plays_what_is_whole();
  a_chunk_that_says_it_is_bigger_than_the_file_does_not_make_the_search_loop();
  the_tones_follow_the_volume_and_the_rate();
  the_job_that_is_running_is_remembered_and_read_back();
  the_touch_of_the_panel_lands_on_the_screen_that_is_drawn_whatever_the_turn();
  the_columns_fit_in_the_screen_with_their_gaps();
  the_kind_of_a_file_is_its_last_extension_in_any_case();
  the_names_that_could_leave_the_card_or_break_it_are_not_names();
  a_listing_has_the_folders_first_and_leaves_out_what_cannot_be_opened();
  sizes_are_written_with_the_unit_that_fits();
  a_free_name_is_the_first_one_that_is_not_taken();
  std::printf("%d checks, %d failures\n", checks, failures);
  return failures == 0 ? 0 : 1;
}
