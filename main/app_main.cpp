// EH3D-NODE-HMI-GUI - main/app_main.cpp: reads the settings, starts the module, and runs every request of the operator, from a touch or
// from the voice, through the gate (core/gate.hpp). A second task is the client of EH3D-SERVER: it reads the fleet every few seconds and
// sends what the operator asked (a job made of a file of the microSD card, its start, its confirmation), so the desktop programs and the
// node see the same jobs at the same time. This has not been built or run on a module yet. GPL-3.0-or-later.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "esp_log.h"
#include "esp_spiffs.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "sdkconfig.h"

#include "../core/hmi_config.hpp"
#include "../core/job_memory.hpp"
#include "../core/orientation.hpp"
#include "../core/quality.hpp"
#include "../core/sdfiles.hpp"
#include "../core/ui_text.hpp"
#include "../core/voice.hpp"
#include "../core/wav.hpp"
#include "../core/workflow.hpp"
#include "board.hpp"
#include "server_link.hpp"
#include "ui.hpp"

static const char* TAG = "hmi";
static hmi::Settings g_settings;
static QueueHandle_t g_requests = nullptr;  // from the screens to the application task: ui::Request
static QueueHandle_t g_to_server = nullptr; // from the application task to the link task: ServerJob
static QueueHandle_t g_say = nullptr;       // the notices to say: hmi::Cue
static SemaphoreHandle_t g_lock = nullptr;  // guards what the link task shares: the fleet, the note, the workflow

// what the link task is asked to do
struct ServerJob {
    enum class Do { Create, Start, Cancel, Confirm, Light, Inspect, Preset, Calibrate, Last } what = Do::Create;
};

static hmi::FleetView g_fleet;
static bool g_fleet_new = false;
static bool g_server_ok = false;
static std::string g_note;
static hmi::Workflow g_flow;
static hmi::SpindleView g_spindle;  // the spindle of the machine, as the server says it (asked now and then)
static bool g_spindle_new = false;

// the quality check: what the server last said, the ring of LEDs, and what the operator chose for the next request
static hmi::QualityView g_quality;
static hmi::LightView g_light;
static bool g_quality_new = false;
static bool g_quality_running = false;
static int g_arg_light = 0;
static int g_arg_hole = 0;
static int g_arg_preset = 0;


// ------------------------------------------------------------------------------------------------------------------------------ the job that this screen follows
// The job lives in the server. The screen keeps only which one is its own (its id, the machine, its name) in the flash, from the moment the operator starts it until it
// is over, so that after a power cut or a reset it follows the same job again: the next reading of the fleet says if it still runs, waits for the operator or is over.

static const char* kMemoryKey = "job";
static bool g_remembered = false;

static void memory_save(const hmi::JobMemory& m) {
    nvs_handle_t h;
    if (nvs_open("eh3d_hmi", NVS_READWRITE, &h) != ESP_OK) return;
    if (nvs_set_str(h, kMemoryKey, hmi::memory_text(m).c_str()) == ESP_OK) nvs_commit(h);
    nvs_close(h);
    g_remembered = true;
}

static void memory_clear() {
    nvs_handle_t h;
    if (nvs_open("eh3d_hmi", NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_key(h, kMemoryKey);
        nvs_commit(h);
        nvs_close(h);
    }
    g_remembered = false;
}

static bool memory_load(hmi::JobMemory& m) {
    nvs_handle_t h;
    if (nvs_open("eh3d_hmi", NVS_READONLY, &h) != ESP_OK) return false;
    std::size_t n = 0;
    bool ok = false;
    if (nvs_get_str(h, kMemoryKey, nullptr, &n) == ESP_OK && n > 1 && n < 512) {
        std::string text(n, '\0');
        if (nvs_get_str(h, kMemoryKey, &text[0], &n) == ESP_OK) {
            text.resize(std::strlen(text.c_str()));
            ok = hmi::memory_parse(text, m);
        }
    }
    nvs_close(h);
    return ok;
}

// ------------------------------------------------------------------------------------------------------------------------------ the notices
// A notice is off, the tone of the screen or a recorded voice (voices/LANG/f|m/<name>.wav on the card); a voice that is not there, or is not 16-bit mono, plays the tone instead of
// nothing. They are said by a task of their own, because playing takes a second or two and the screen and the gate must not wait for it.

static void say_task(void*) {
    for (;;) {
        hmi::Cue cue;
        if (xQueueReceive(g_say, &cue, portMAX_DELAY) != pdTRUE) continue;
        const hmi::NoticeSetting* setting = nullptr;
        switch (cue) {
            case hmi::Cue::Finished: setting = &g_settings.sound_finish; break;
            case hmi::Cue::ToolChange: setting = &g_settings.sound_tool_change; break;
            case hmi::Cue::CheckOk: setting = &g_settings.sound_check_ok; break;
            case hmi::Cue::CheckNotOk: setting = &g_settings.sound_check_bad; break;
        }
        if (setting == nullptr || setting->kind == hmi::NoticeSetting::Kind::Off) continue;
        const unsigned volume = g_settings.volume_percent;
        if (setting->kind == hmi::NoticeSetting::Kind::Voice) {
            std::string bytes;
            hmi::WavInfo info;
            if (board::read_file(hmi::notice_file("/sdcard", *setting, cue), bytes, 400 * 1024) && hmi::parse_wav(bytes, info)) {
                std::vector<std::int16_t> samples = hmi::wav_samples(bytes, info);
                hmi::scale_samples(samples, volume);
                board::play(samples.data(), samples.size(), info.rate);
                continue;
            }
        }
        const std::vector<std::int16_t> tone = hmi::tone_samples(cue, 22050, volume);
        board::play(tone.data(), tone.size(), 22050);
    }
}

static void say(hmi::Cue cue) {
    if (g_say != nullptr) xQueueSend(g_say, &cue, 0);
}

// called by the screens in the LVGL task: the request is only queued
static void post(const ui::Request& r) {
    if (g_requests != nullptr) xQueueSend(g_requests, &r, 0);
}

static std::string read_file(const char* path) {
    std::string text;
    if (FILE* f = std::fopen(path, "rb")) {
        char buf[256];
        std::size_t n;
        while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) text.append(buf, n);
        std::fclose(f);
    }
    return text;
}

static void set_note(const std::string& s) {
    xSemaphoreTake(g_lock, portMAX_DELAY);
    g_note = s;
    xSemaphoreGive(g_lock);
}

// A request of the quality check: the ring of LEDs, a check, a preset of the profile, the calibration of the scale, or the last check. Everything goes through the server of
// the machine, which holds the token of the node of the drill head.
static void quality_job(const ServerJob& job) {
    if (!hmi::controls_for(g_settings.machine).quality_check) return;  // a laser or a pick and place machine has no camera for the visual check
    if (g_settings.server_machine.empty()) {
        set_note(hmi::quality_text(hmi::Q::NeedServer, g_settings.language));
        return;
    }
    int light = 0, hole = 0, preset = 0;
    xSemaphoreTake(g_lock, portMAX_DELAY);
    light = g_arg_light;
    hole = g_arg_hole;
    preset = g_arg_preset;
    xSemaphoreGive(g_lock);
    hmi::Request req;
    switch (job.what) {
        case ServerJob::Do::Light: req = hmi::light_request(g_settings.server_machine, hmi::kLightLevels[light]); break;
        case ServerJob::Do::Inspect: req = hmi::inspect_request(g_settings.server_machine, hmi::kLightLevels[light], hmi::kHoleWidths[hole]); break;
        case ServerJob::Do::Preset: req = hmi::preset_request(g_settings.server_machine, preset); break;
        case ServerJob::Do::Calibrate: req = hmi::calibrate_request(g_settings.server_machine); break;
        case ServerJob::Do::Last: req = hmi::inspect_last_request(g_settings.server_machine); break;
        default: return;
    }
    hmi::Response r;
    std::string error;
    const bool reached = srvlink::call(g_settings, req, r, error);
    xSemaphoreTake(g_lock, portMAX_DELAY);
    if (job.what == ServerJob::Do::Inspect) g_quality_running = false;
    bool say_ok = false, say_bad = false;
    if (!reached) {
        g_note = error;
    } else if (r.status == 200) {
        hmi::QualityView q;
        hmi::LightView l;
        if ((job.what == ServerJob::Do::Inspect || job.what == ServerJob::Do::Last) && hmi::parse_quality(r.body, q)) {
            g_quality = q;
            // only a check that was just made is said, never the last one that is shown when the page opens; "cannot judge" is neither correct nor not correct
            if (job.what == ServerJob::Do::Inspect) {
                say_ok = q.verdict == hmi::QualityVerdict::Ok;
                say_bad = q.verdict == hmi::QualityVerdict::NotOk;
            }
        }
        if (job.what == ServerJob::Do::Light && hmi::parse_light(r.body, l)) g_light = l;
        if (job.what == ServerJob::Do::Preset || job.what == ServerJob::Do::Calibrate) g_note = "ok";
        g_quality_new = true;
    } else if (job.what != ServerJob::Do::Last) {
        g_note = hmi::error_line(r);  // the screen that has never been checked has no last check: that is not an error to show
    }
    xSemaphoreGive(g_lock);
    if (say_ok) say(hmi::Cue::CheckOk);
    if (say_bad) say(hmi::Cue::CheckNotOk);
}

// The task of the link: asks for the fleet every `poll_s` seconds and does what the operator asked, one request at a time.
static void link_task(void*) {
    TickType_t next_poll = 0;
    unsigned polls = 0;
    for (;;) {
        ServerJob job;
        const bool asked = xQueueReceive(g_to_server, &job, pdMS_TO_TICKS(200)) == pdTRUE;
        std::string error;
        hmi::Response r;
        if (asked && job.what >= ServerJob::Do::Light) {
            quality_job(job);
        } else if (asked) {
            hmi::Request req;
            xSemaphoreTake(g_lock, portMAX_DELAY);
            switch (job.what) {
                case ServerJob::Do::Create: req = g_flow.create_request(g_settings.server_machine); break;
                case ServerJob::Do::Start: req = g_flow.start_request(); break;
                case ServerJob::Do::Cancel: req = g_flow.cancel_request(); break;
                case ServerJob::Do::Confirm: req = g_flow.confirm_request(); break;
                // the quality check has its own function (quality_job): it never gets here
                case ServerJob::Do::Light:
                case ServerJob::Do::Inspect:
                case ServerJob::Do::Preset:
                case ServerJob::Do::Calibrate:
                case ServerJob::Do::Last: break;
            }
            xSemaphoreGive(g_lock);
            if (req.path.empty()) {
                set_note("nothing to send: choose a file and set the machine of the server");
            } else if (!srvlink::call(g_settings, req, r, error)) {
                set_note(error);
            } else {
                std::string why;
                xSemaphoreTake(g_lock, portMAX_DELAY);
                bool ok = true;
                if (job.what == ServerJob::Do::Create) ok = g_flow.on_created(r, why);
                else if (job.what == ServerJob::Do::Start) ok = g_flow.on_started(r, why);
                else if (r.status != 200 && r.status != 202) { ok = false; why = hmi::error_line(r); }
                const std::string base = g_flow.report_base();
                const std::string name = g_flow.name();
                const std::string job_id = g_flow.job_id();
                xSemaphoreGive(g_lock);
                if (ok && job.what == ServerJob::Do::Start) memory_save({job_id, g_settings.server_machine, name});
                set_note(ok ? "ok" : why);
                if (ok && job.what == ServerJob::Do::Create) {
                    // the answer of the server is kept on the card, without replacing an older report
                    const std::string file = hmi::free_name(base, "txt", [](const std::string& n) {
                        std::string tmp;
                        return board::sd_read(n, tmp, 1);
                    });
                    board::sd_write(file, "job " + name + "\n" + r.body + "\n");
                }
            }
        }
        if (xTaskGetTickCount() >= next_poll) {
            next_poll = xTaskGetTickCount() + pdMS_TO_TICKS(g_settings.poll_s * 1000);
            hmi::Response fr;
            hmi::FleetView view;
            if (srvlink::call(g_settings, hmi::fleet_request(), fr, error) && fr.status == 200 && hmi::parse_fleet(fr.body, view)) {
                xSemaphoreTake(g_lock, portMAX_DELAY);
                g_fleet = view;
                g_fleet_new = true;
                g_server_ok = true;
                g_flow.on_fleet(view, g_settings.server_machine);
                const hmi::Request ended = g_flow.report_request();
                xSemaphoreGive(g_lock);
                // a job that left the machine: the server says if it was done or stopped (a stopped one can go on from the holes it did)
                if (!ended.path.empty()) {
                    hmi::Response rr;
                    const bool got = srvlink::call(g_settings, ended, rr, error);
                    xSemaphoreTake(g_lock, portMAX_DELAY);
                    if (got) g_flow.on_report(rr);
                    // the drilling is done and the routing comes after it (or the other way round for the files chosen): its job is made and waits for the operator
                    const bool next = got && g_flow.advance();
                    const std::string next_name = g_flow.name();
                    xSemaphoreGive(g_lock);
                    if (next) {
                        ServerJob again;
                        again.what = ServerJob::Do::Create;
                        xQueueSend(g_to_server, &again, 0);
                        set_note("next: " + next_name);
                    }
                }
                // the spindle of the machine: asked at the first reading and then every tenth
                if (!g_settings.server_machine.empty() && (polls++ % 10) == 0) {
                    hmi::Response sr;
                    hmi::SpindleView sv;
                    if (srvlink::call(g_settings, hmi::spindle_request(g_settings.server_machine), sr, error) && sr.status == 200 && hmi::parse_spindle(sr.body, sv)) {
                        xSemaphoreTake(g_lock, portMAX_DELAY);
                        g_spindle = sv;
                        g_spindle_new = true;
                        xSemaphoreGive(g_lock);
                    }
                }
            } else {
                xSemaphoreTake(g_lock, portMAX_DELAY);
                g_server_ok = false;
                xSemaphoreGive(g_lock);
            }
        }
    }
}

// Shows the tools of the chosen file and the spindle of the machine (the LVGL lock is taken by the caller).
static void refresh_job() {
    xSemaphoreTake(g_lock, portMAX_DELAY);
    const hmi::ToolSwitch tools = g_flow.tool_switch();
    const hmi::SpindleView spindle = g_spindle;
    ui::FlowState flow;
    flow.stopped = g_flow.stage() == hmi::Stage::Stopped;
    flow.can_resume = g_flow.can_resume();
    flow.route = g_flow.route();
    flow.done = g_flow.done();
    flow.total = g_flow.total();
    flow.skipped = g_flow.skipped();
    flow.next = g_flow.next_name();
    flow.message = g_flow.message();
    xSemaphoreGive(g_lock);
    ui::show_job(tools, spindle, g_settings);
    ui::show_flow(flow, g_settings);
}

static void rebuild(bool mounted_portrait) {
    const hmi::Layout layout = hmi::layout_for(g_settings.orientation, mounted_portrait);
    board::lvgl_lock();
    board::set_rotation(layout.rotation);
    ui::build(g_settings, layout, post);
    ui::show_card(board::sd_list(), g_settings);
    refresh_job();
    board::lvgl_unlock();
}

extern "C" void app_main() {
    {
        const esp_err_t e = nvs_flash_init();
        if (e == ESP_ERR_NVS_NO_FREE_PAGES || e == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            nvs_flash_erase();
            nvs_flash_init();
        }
    }
    esp_vfs_spiffs_conf_t fs = {"/config", "config", 4, false};
    if (esp_vfs_spiffs_register(&fs) != ESP_OK) ESP_LOGW(TAG, "no config partition: the defaults are used");
    std::vector<hmi::Problem> problems;
    g_settings = hmi::parse_settings(read_file(CONFIG_HMI_CONFIG_PATH), problems);
    for (const auto& p : problems) ESP_LOGW(TAG, "settings line %d (%s): %s", p.line, p.key.c_str(), p.what.c_str());

    if (!board::display_start()) {
        ESP_LOGE(TAG, "the display does not start");
        return;
    }
    board::set_brightness(g_settings.brightness_percent);
    if (!board::audio_start()) ESP_LOGW(TAG, "no sound: the alerts are only shown");
    if (!board::sd_mount()) ESP_LOGW(TAG, "no microSD card: files cannot be opened or saved");

    g_requests = xQueueCreate(8, sizeof(ui::Request));
    g_to_server = xQueueCreate(4, sizeof(ServerJob));
    g_say = xQueueCreate(4, sizeof(hmi::Cue));
    xTaskCreate(say_task, "say", 8192, nullptr, 3, nullptr);
    g_lock = xSemaphoreCreateMutex();
    // a job that was running when the screen went down: follow it again; the operator can leave it with the cancel button
    {
        hmi::JobMemory m;
        if (memory_load(m) && m.machine == g_settings.server_machine && g_flow.adopt(m.job_id, m.name)) {
            g_remembered = true;
            set_note("following the job that was left running: " + m.name);
        } else {
            memory_clear();  // what was kept is not for this machine, or it is damaged
        }
    }
    xTaskCreate(link_task, "link", 8192, nullptr, 4, nullptr);
    rebuild(false);
    if (hmi::controls_for(g_settings.machine).quality_check) {
        // the last check of the machine, so the screen shows what the desktop program found too
        ServerJob last;
        last.what = ServerJob::Do::Last;
        xQueueSend(g_to_server, &last, 0);
    }

    hmi::Gate gate(g_settings);
    ui::State state;
    hmi::Stage last_stage = hmi::Stage::Nothing;
    for (;;) {
        bool wired = false, pressed = false;
        board::read_hardware_stop(wired, pressed);
        gate.hardware_stop(wired, pressed);
        gate.link(state.armed);  // `armed` follows the job of the server; until a job runs the gate refuses every move
        gate.tick(board::now_ms());
        {
            // the top speed of the spindle the machine carries (the server says it): an order for more turns is cut to it or refused
            xSemaphoreTake(g_lock, portMAX_DELAY);
            gate.machine_spindle_max(g_spindle.known ? g_spindle.max_rpm : 0);
            xSemaphoreGive(g_lock);
        }

        ui::Request r;
        while (xQueueReceive(g_requests, &r, 0) == pdTRUE) {
            board::lvgl_lock();
            switch (r.what) {
                case ui::What::AckRelease:
                    gate.acknowledge_release();
                    break;
                case ui::What::Cancel:
                    gate.cancel();
                    ui::ask(nullptr, g_settings);
                    break;
                case ui::What::Confirm: {
                    hmi::Action go;
                    if (gate.confirm(hmi::Origin::Touch, board::now_ms(), go)) {
                        ESP_LOGI(TAG, "confirmed: %s", go.what.c_str());
                        ServerJob job;
                        job.what = go.what == "start job" ? ServerJob::Do::Start : ServerJob::Do::Confirm;
                        if (go.what == "start job" || go.what == "tool changed") xQueueSend(g_to_server, &job, 0);
                        else set_note("this move needs the link to the node of the machine, which is not written yet");
                    }
                    ui::ask(nullptr, g_settings);
                    break;
                }
                case ui::What::Action:
                    if (r.action.kind == hmi::Kind::Stop) {
                        ServerJob job;
                        job.what = ServerJob::Do::Cancel;
                        xQueueSend(g_to_server, &job, 0);  // a stop always goes: the job of the server is cancelled
                    }
                    {
                        hmi::Decision d = gate.request(r.action, hmi::Origin::Touch, board::now_ms());
                        if (d.verdict == hmi::Verdict::NeedConfirm) {
                            // the confirmation shows the turns the spindle will really get; when they were cut the screen says so
                            ui::ask(&gate.pending_action(), g_settings);
                            if (d.clipped) set_note(std::string(hmi::text(hmi::T::SpindleCut, g_settings.language)) + ": " + std::to_string(d.asked_rpm) + " > " + std::to_string(d.rpm) + " rpm");
                        } else if (d.verdict == hmi::Verdict::Refused) {
                            ui::refused(d.why, g_settings);
                        } else if (d.clipped) {
                            set_note(std::string(hmi::text(hmi::T::SpindleCut, g_settings.language)) + ": " + std::to_string(d.asked_rpm) + " > " + std::to_string(d.rpm) + " rpm");
                        }
                    }
                    break;
                case ui::What::SetSpindleOver:
                    g_settings.spindle_over = r.refuse ? hmi::SpindleOver::Refuse : hmi::SpindleOver::Clip;  // kept until the next start: hmi.cfg (spindle_over = ...) holds the lasting choice
                    break;
                case ui::What::Resume: {
                    std::string why;
                    xSemaphoreTake(g_lock, portMAX_DELAY);
                    const bool ok = g_flow.resume(why);
                    xSemaphoreGive(g_lock);
                    if (ok) {
                        // the same file is sent again leaving out the holes (paths) already done; the start goes through the gate as any other
                        ServerJob job;
                        job.what = ServerJob::Do::Create;
                        xQueueSend(g_to_server, &job, 0);
                        set_note("going on from the holes already done");
                        refresh_job();
                    } else {
                        set_note(why);
                    }
                    break;
                }
                case ui::What::SetOrientation:
                    g_settings.orientation = r.orientation;  // kept until the next start: hmi.cfg holds the lasting choice
                    board::lvgl_unlock();
                    rebuild(false);
                    board::lvgl_lock();
                    break;
                case ui::What::SetLanguage:
                    g_settings.language = r.language;  // kept until the next start: hmi.cfg (language = ...) holds the lasting choice
                    board::lvgl_unlock();
                    rebuild(false);
                    board::lvgl_lock();
                    break;
                case ui::What::RefreshCard:
                    ui::show_card(board::sd_list(), g_settings);
                    break;
                case ui::What::OpenCard: {
                    std::string text, why;
                    xSemaphoreTake(g_lock, portMAX_DELAY);
                    bool ok = board::sd_read(r.file, text, 512 * 1024) && g_flow.choose(r.file, text, why);
                    if (ok) {
                        // the tool table of the same name, when the card has one
                        std::string base = r.file;
                        base.resize(base.rfind('.'));
                        std::string tools;
                        board::sd_read(base + (hmi::kind_of_file(r.file) == hmi::FileKind::Route ? ".rtd" : ".dtd"), tools, 64 * 1024);
                        g_flow.set_tools(tools);
                    }
                    xSemaphoreGive(g_lock);
                    set_note(ok ? std::string("file: ") + r.file : (why.empty() ? "the file cannot be read" : why));
                    if (ok) refresh_job();
                    break;
                }
                case ui::What::ToggleTool: {
                    xSemaphoreTake(g_lock, portMAX_DELAY);
                    g_flow.toggle_tool(r.tool);
                    xSemaphoreGive(g_lock);
                    refresh_job();
                    break;
                }
                case ui::What::LightSet:
                case ui::What::InspectRun:
                case ui::What::InspectPreset:
                case ui::What::InspectCalibrate: {
                    if (!hmi::controls_for(g_settings.machine).quality_check) break;
                    ServerJob job;
                    xSemaphoreTake(g_lock, portMAX_DELAY);
                    if (r.what == ui::What::LightSet) { g_arg_light = r.light; job.what = ServerJob::Do::Light; }
                    else if (r.what == ui::What::InspectRun) { g_arg_light = r.light; g_arg_hole = r.hole; g_quality_running = true; job.what = ServerJob::Do::Inspect; }
                    else if (r.what == ui::What::InspectPreset) { g_arg_preset = r.preset; job.what = ServerJob::Do::Preset; }
                    else job.what = ServerJob::Do::Calibrate;
                    xSemaphoreGive(g_lock);
                    xQueueSend(g_to_server, &job, 0);
                    break;
                }
                case ui::What::SendJob: {
                    xSemaphoreTake(g_lock, portMAX_DELAY);
                    // a drilling file with every tool off while a routing file waits: the routing is the job
                    const bool moved_on = g_flow.nothing_to_do() && g_flow.skip_empty();
                    const std::string moved_to = g_flow.name();
                    const bool nothing = g_flow.nothing_to_do();
                    xSemaphoreGive(g_lock);
                    if (moved_on) {
                        set_note("next: " + moved_to);
                        refresh_job();
                    }
                    if (nothing) {
                        set_note("every tool of the file is switched off: nothing to send");
                        break;
                    }
                    ServerJob job;
                    job.what = ServerJob::Do::Create;
                    xQueueSend(g_to_server, &job, 0);
                    break;
                }
            }
            board::lvgl_unlock();
        }

        // what the link task found out: the fleet, the note and the stage of the job
        xSemaphoreTake(g_lock, portMAX_DELAY);
        const bool fleet_new = g_fleet_new;
        const bool spindle_new = g_spindle_new;
        g_spindle_new = false;
        const hmi::FleetView fleet = g_fleet;
        g_fleet_new = false;
        const hmi::QualityView quality = g_quality;
        const hmi::LightView light = g_light;
        const bool quality_new = g_quality_new || g_quality_running;
        const bool quality_running = g_quality_running;
        g_quality_new = false;
        state.server_ok = g_server_ok;
        state.note = g_note;
        state.percent = g_flow.percent();
        // the machine counts as armed while the job of this screen runs or waits for the operator
        state.armed = g_flow.stage() == hmi::Stage::Running || g_flow.stage() == hmi::Stage::Waiting;
        const bool over = g_flow.stage() == hmi::Stage::Finished || g_flow.stage() == hmi::Stage::Failed || g_flow.stage() == hmi::Stage::Stopped;
        const hmi::Stage stage_now = g_flow.stage();
        xSemaphoreGive(g_lock);
        if (over && g_remembered) memory_clear();
        // the notices of the job, once each time the stage changes into them
        bool stage_changed = false;
        if (stage_now != last_stage) {
            if (stage_now == hmi::Stage::Finished) say(hmi::Cue::Finished);
            if (stage_now == hmi::Stage::Waiting) say(hmi::Cue::ToolChange);
            last_stage = stage_now;
            stage_changed = true;
        }
        state.stop_wired = wired;
        state.stop_pressed = pressed;
        state.stop_latched = gate.latched();
        board::lvgl_lock();
        ui::show(state, g_settings);
        if (fleet_new) ui::show_fleet(fleet, g_settings);
        if (spindle_new || stage_changed) refresh_job();
        if (quality_new && hmi::controls_for(g_settings.machine).quality_check) ui::show_quality(quality, light, quality_running, g_settings);
        if (!gate.pending()) ui::ask(nullptr, g_settings);
        board::lvgl_unlock();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
