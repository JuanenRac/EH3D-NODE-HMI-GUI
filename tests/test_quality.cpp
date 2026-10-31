// EH3D-NODE-HMI-GUI host tests: the quality check from the screen: the requests to the server, the reading of its report and of the ring of LEDs, and the words of the
// verdict and of every finding in the eight languages.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
#include <cstdio>
#include <cstring>
#include <string>

#include "../core/gate.hpp"
#include "../core/quality.hpp"
#include "../core/ui_text.hpp"

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

static void the_requests() {
    Request r = light_request("bench", 60);
    CHECK(r.method == "POST" && r.path == "/api/v1/machines/bench/head/light?on=1&level=60&tone=white&sectors=all" && r.body.empty());
    r = light_request("bench", 0);
    CHECK(r.path == "/api/v1/machines/bench/head/light?on=0");
    CHECK(light_request("bench", 900).path.find("level=100") != std::string::npos);
    CHECK(light_status_request("bench").method == "GET");

    r = inspect_request("bench", 60, 0.8);
    CHECK(r.method == "POST" && r.path == "/api/v1/machines/bench/head/inspect");
    Json j;
    CHECK(parse_json(r.body, j));
    const Json* holes = j.get("holes");
    CHECK(holes && holes->type == Json::Type::Array && holes->a.size() == 1);
    CHECK(holes->a[0].num("dia_mm") == 0.8 && holes->a[0].num("x_mm") == 0 && holes->a[0].num("y_mm") == 0);
    CHECK(j.num("light") == 60);
    const Json* restore = j.get("restore_light");
    CHECK(restore && restore->type == Json::Type::Bool && restore->b);
    // without a hole nothing is asked of the holes, and without a light the ring is left alone
    r = inspect_request("bench", 0, 0.0);
    CHECK(parse_json(r.body, j) && j.get("holes")->a.empty() && j.get("light") == nullptr);
    // the request is a request the server reads and the wire carries the token
    CHECK(wire(inspect_request("bench", 30, 1.0), "srv:18200", "tok").find("Authorization: Bearer tok") != std::string::npos);

    CHECK(inspect_last_request("bench").method == "GET");
    CHECK(preset_request("bench", 1).body == "{\"preset\":\"fine_pitch\"}" && preset_request("bench", 1).method == "PUT");
    CHECK(preset_request("bench", 0).body == "{\"preset\":\"standard\"}" && preset_request("bench", 99).body == "{\"preset\":\"prototype\"}" && preset_request("bench", -4).body == "{\"preset\":\"standard\"}");
    CHECK(calibrate_request("bench").path == "/api/v1/machines/bench/head/inspect/calibrate");
}

static void the_report() {
    const char* good =
        "{\"verdict\":\"ok\",\"profile\":\"standard\",\"holes\":[{\"index\":0,\"ok\":true},{\"index\":1,\"ok\":true}],\"milling\":{\"samples\":120,\"width_mm\":0.21,\"ok\":true},\"findings\":[]}";
    QualityView q;
    CHECK(parse_quality(good, q) && q.verdict == QualityVerdict::Ok && q.holes_total == 2 && q.holes_ok == 2 && q.findings.empty());
    CHECK(q.channel_mm > 0.2 && q.channel_mm < 0.22);

    const char* bad =
        "{\"verdict\":\"not_ok\",\"holes\":[{\"ok\":true},{\"ok\":false},{\"ok\":false}],\"milling\":null,"
        "\"findings\":[{\"code\":\"hole_missing\",\"hole\":1,\"fails\":true},{\"code\":\"hole_out_of_view\",\"hole\":2,\"fails\":false},{\"code\":\"mill_pinched\",\"hole\":null,\"fails\":true}]}";
    CHECK(parse_quality(bad, q) && q.verdict == QualityVerdict::NotOk && q.holes_total == 3 && q.holes_ok == 1 && q.channel_mm == 0.0);
    CHECK(q.findings.size() == 3 && q.findings[0].hole == 1 && q.findings[0].fails && !q.findings[1].fails && q.findings[2].hole == -1);
    CHECK(finding_line(q.findings[0], Lang::En) == "#2: no hole there");
    CHECK(finding_line(q.findings[2], Lang::En) == "a channel pinches: risk of a short");

    CHECK(parse_quality("{\"verdict\":\"unknown\",\"findings\":[{\"code\":\"image_too_dark\",\"fails\":false}]}", q) && q.verdict == QualityVerdict::Unknown && !q.findings[0].fails);
    // what is not a report
    for (const char* no : {"", "[]", "{}", "{\"verdict\":\"maybe\"}", "{\"error\":\"no_inspection\"}", "not json"}) CHECK(!parse_quality(no, q));
    // a channel width counts only when channels were seen
    CHECK(parse_quality("{\"verdict\":\"ok\",\"milling\":{\"samples\":0,\"width_mm\":0.5}}", q) && q.channel_mm == 0.0);
}

static void the_ring() {
    LightView l;
    CHECK(parse_light("{\"light\":true,\"on\":true,\"level\":60,\"max_level\":80}", l) && l.known && l.on && l.level == 60);
    CHECK(parse_light("{\"on\":false,\"level\":0}", l) && !l.on && l.level == 0);
    CHECK(!parse_light("{\"error\":\"no_light\"}", l) && !parse_light("nope", l));
}

static void the_words() {
    const unsigned count = static_cast<unsigned>(Q::Count);
    for (unsigned i = 0; i < count; ++i) {
        const Row& row = quality_row(static_cast<Q>(i));
        int same_as_english = 0;
        for (std::size_t l = 0; l < kBaseLangCount; ++l) {
            CHECK(row[l] != nullptr && row[l][0] != '\0');
            if (l > 0 && std::strcmp(row[l], row[0]) == 0) ++same_as_english;
        }
        CHECK(same_as_english <= 5);
        // Chinese and Japanese are written in their own letters
        for (std::size_t l : {static_cast<std::size_t>(6), static_cast<std::size_t>(7)}) {
            bool own = false;
            for (const char* p = row[l]; *p; ++p) own |= (static_cast<unsigned char>(*p) >= 0xE0);
            CHECK(own);
        }
    }
    // every code of the analyser has words in the eight languages, and a word the screen does not know is shown as it is
    for (const char* code : {"hole_missing", "hole_wrong_size", "hole_off_centre", "hole_not_round", "hole_debris", "hole_out_of_view", "mill_no_channels", "mill_too_narrow", "mill_too_wide",
                             "mill_pinched", "mill_uneven", "image_too_dark", "image_too_bright", "image_low_contrast", "image_blurry", "not_calibrated", "nothing_to_check", "no_expected_holes"}) {
        for (std::size_t l = 0; l < kLangCount; ++l) {
            const char* t = finding_text(code, static_cast<Lang>(l));
            CHECK(std::strcmp(t, code) != 0 && t[0] != '\0');
        }
    }
    const std::string unknown = "something_new";
    CHECK(std::strcmp(finding_text(unknown, Lang::Es), "something_new") == 0);
    CHECK(std::strcmp(quality_text(Q::VerdictOk, Lang::Es), "CORRECTO") == 0 && std::strcmp(quality_text(Q::VerdictNotOk, Lang::De), "NICHT IN ORDNUNG") == 0);
    CHECK(kLightLevels[0] == 0 && kLightLevels[3] == 80 && kHoleWidths[0] == 0.0);
}

int main() {
    the_requests();
    the_report();
    the_ring();
    the_words();
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
