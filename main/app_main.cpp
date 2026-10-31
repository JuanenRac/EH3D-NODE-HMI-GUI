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
#include "../core/sdfiles.hpp"
#include "../core/ui_text.hpp"
#include "../core/voice.hpp"
#include "../core/workflow.hpp"
#include "board.hpp"
#include "server_link.hpp"
#include "ui.hpp"

static const char* TAG = "hmi";
static hmi::Settings g_settings;
static QueueHandle_t g_requests = nullptr;  // from the screens to the application task: ui::Request
static QueueHandle_t g_to_server = nullptr; // from the application task to the link task: ServerJob
static SemaphoreHandle_t g_lock = nullptr;  // guards what the link task shares: the fleet, the note, the workflow

// what the link task is asked to do
struct ServerJob {
    enum class Do { Create, Start, Cancel, Confirm } what = Do::Create;
};

static hmi::FleetView g_fleet;
static bool g_fleet_new = false;
static bool g_server_ok = false;
static std::string g_note;
static hmi::Workflow g_flow;


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

// The task of the link: asks for the fleet every `poll_s` seconds and does what the operator asked, one request at a time.
static void link_task(void*) {
    TickType_t next_poll = 0;
    for (;;) {
        ServerJob job;
        const bool asked = xQueueReceive(g_to_server, &job, pdMS_TO_TICKS(200)) == pdTRUE;
        std::string error;
        hmi::Response r;
        if (asked) {
            hmi::Request req;
            xSemaphoreTake(g_lock, portMAX_DELAY);
            switch (job.what) {
                case ServerJob::Do::Create: req = g_flow.create_request(g_settings.server_machine); break;
                case ServerJob::Do::Start: req = g_flow.start_request(); break;
                case ServerJob::Do::Cancel: req = g_flow.cancel_request(); break;
                case ServerJob::Do::Confirm: req = g_flow.confirm_request(); break;
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
                xSemaphoreGive(g_lock);
            } else {
                xSemaphoreTake(g_lock, portMAX_DELAY);
                g_server_ok = false;
                xSemaphoreGive(g_lock);
            }
        }
    }
}

static void rebuild(bool mounted_portrait) {
    const hmi::Layout layout = hmi::layout_for(g_settings.orientation, mounted_portrait);
    board::lvgl_lock();
    board::set_rotation(layout.rotation);
    ui::build(g_settings, layout, post);
    ui::show_card(board::sd_list(), g_settings);
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

    hmi::Gate gate(g_settings);
    ui::State state;
    for (;;) {
        bool wired = false, pressed = false;
        board::read_hardware_stop(wired, pressed);
        gate.hardware_stop(wired, pressed);
        gate.link(state.armed);  // `armed` follows the job of the server; until a job runs the gate refuses every move
        gate.tick(board::now_ms());

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
                        if (d.verdict == hmi::Verdict::NeedConfirm) ui::ask(&r.action, g_settings);
                        else if (d.verdict == hmi::Verdict::Refused) ui::refused(d.why, g_settings);
                    }
                    break;
                case ui::What::SetOrientation:
                    g_settings.orientation = r.orientation;  // kept until the next start: hmi.cfg holds the lasting choice
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
                    break;
                }
                case ui::What::SendJob: {
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
        const hmi::FleetView fleet = g_fleet;
        g_fleet_new = false;
        state.server_ok = g_server_ok;
        state.note = g_note;
        state.percent = g_flow.percent();
        // the machine counts as armed while the job of this screen runs or waits for the operator
        state.armed = g_flow.stage() == hmi::Stage::Running || g_flow.stage() == hmi::Stage::Waiting;
        const bool over = g_flow.stage() == hmi::Stage::Finished || g_flow.stage() == hmi::Stage::Failed;
        xSemaphoreGive(g_lock);
        if (over && g_remembered) memory_clear();
        state.stop_wired = wired;
        state.stop_pressed = pressed;
        state.stop_latched = gate.latched();
        board::lvgl_lock();
        ui::show(state, g_settings);
        if (fleet_new) ui::show_fleet(fleet, g_settings);
        if (!gate.pending()) ui::ask(nullptr, g_settings);
        board::lvgl_unlock();
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
