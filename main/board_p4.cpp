// EH3D-NODE-HMI-GUI - main/board_p4.cpp: the Guition JC8012P4A1C module (ESP32-P4, 10.1" JD9365 800x1280 MIPI-DSI with touch,
// ES8311 codec, NS4150 amplifier). The display and the touch start through the board support package of the component registry;
// this file has not been built or run on a module yet. GPL-3.0-or-later.
#include "board.hpp"

#include <dirent.h>
#include <sys/stat.h>

#include <cstdio>

#include "bsp/esp-bsp.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "sdkconfig.h"

namespace board {

bool display_start() { return bsp_display_start() != nullptr; }

void lvgl_lock() { bsp_display_lock(0); }
void lvgl_unlock() { bsp_display_unlock(); }

void set_brightness(unsigned percent) {
    if (percent < 10) percent = 10;
    if (percent > 100) percent = 100;
    bsp_display_brightness_set(static_cast<int>(percent));
}

bool audio_start() { return bsp_audio_codec_speaker_init() != nullptr && bsp_audio_codec_microphone_init() != nullptr; }

void read_hardware_stop(bool& wired, bool& pressed) {
    static bool configured = false;
    wired = CONFIG_HMI_HW_STOP_GPIO >= 0;
    pressed = false;
    if (!wired) return;
    const gpio_num_t pin = static_cast<gpio_num_t>(CONFIG_HMI_HW_STOP_GPIO);
    if (!configured) {
        gpio_config_t cfg = {};
        cfg.pin_bit_mask = 1ULL << pin;
        cfg.mode = GPIO_MODE_INPUT;
#if CONFIG_HMI_HW_STOP_ACTIVE_LOW
        cfg.pull_up_en = GPIO_PULLUP_ENABLE;
#else
        cfg.pull_down_en = GPIO_PULLDOWN_ENABLE;
#endif
        gpio_config(&cfg);
        configured = true;
    }
    const int level = gpio_get_level(pin);
#if CONFIG_HMI_HW_STOP_ACTIVE_LOW
    pressed = level == 0;
#else
    pressed = level != 0;
#endif
}

void set_rotation(int degrees) {
    lv_display_t* d = lv_display_get_default();
    if (d == nullptr) return;
    lv_display_set_rotation(d, degrees == 90 ? LV_DISPLAY_ROTATION_90 : LV_DISPLAY_ROTATION_0);
}

bool sd_mount() { return bsp_sdcard_mount() == ESP_OK; }

std::vector<hmi::Entry> sd_list() {
    std::vector<hmi::Entry> out;
    DIR* dir = opendir(hmi::kSdMount);
    if (dir == nullptr) return out;
    while (dirent* e = readdir(dir)) {
        hmi::Entry entry;
        entry.name = e->d_name;
        std::string path = std::string(hmi::kSdMount) + "/" + entry.name;
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            entry.folder = S_ISDIR(st.st_mode);
            entry.size = static_cast<std::uint64_t>(st.st_size);
        }
        out.push_back(entry);
    }
    closedir(dir);
    return hmi::sorted_listing(out);
}

bool sd_read(const std::string& name, std::string& out, std::size_t max_bytes) {
    const std::string path = hmi::sd_path(name);
    if (path.empty()) return false;
    FILE* f = std::fopen(path.c_str(), "rb");
    if (f == nullptr) return false;
    out.clear();
    char buf[512];
    std::size_t n;
    bool ok = true;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) {
        if (out.size() + n > max_bytes) { ok = false; break; }
        out.append(buf, n);
    }
    std::fclose(f);
    return ok;
}

bool sd_write(const std::string& name, const std::string& text) {
    const std::string path = hmi::sd_path(name);
    if (path.empty()) return false;
    FILE* f = std::fopen(path.c_str(), "wb");
    if (f == nullptr) return false;
    const bool ok = std::fwrite(text.data(), 1, text.size(), f) == text.size();
    return std::fclose(f) == 0 && ok;
}

std::uint32_t now_ms() { return static_cast<std::uint32_t>(esp_timer_get_time() / 1000); }

}  // namespace board
