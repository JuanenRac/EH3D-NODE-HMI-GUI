// EH3D-NODE-HMI-GUI - main/touch_gsl3680.cpp: the GSL3680 touch controller. It has no memory of its own for its program:
// the table is loaded at each start from a file the user provides (`gsl3680.fw`, made with tools/gsl3680_table.py), first
// from the settings partition and then from the card. Without the file the screen starts without touch.
// Not run on a module. GPL-3.0-or-later.
//
// The start-up sequence is adapted from the component guition/esp_lcd_touch_gsl3680, MIT licence, Copyright (c) 2026
// esp_lcd_touch_gsl3680 contributors (see docs/licenses/esp_lcd_touch_gsl3680.txt).
#include "module_p4.h"

#include <cstdio>
#include <cstring>
#include <string>

#include "../core/gsl3680.hpp"
#include "../core/sdfiles.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"

namespace {

const char* const TAG = "gsl3680";
i2c_master_dev_handle_t s_device = nullptr;

void wait(unsigned ms) { vTaskDelay(pdMS_TO_TICKS(ms)); }

bool send(std::uint8_t reg, const std::uint8_t* data, std::size_t count) {
    std::uint8_t frame[5];
    if (count > 4) return false;
    frame[0] = reg;
    std::memcpy(frame + 1, data, count);
    return i2c_master_transmit(s_device, frame, count + 1, 50) == ESP_OK;
}

bool send_byte(std::uint8_t reg, std::uint8_t value) { return send(reg, &value, 1); }

bool receive(std::uint8_t reg, std::uint8_t* out, std::size_t count) { return i2c_master_transmit_receive(s_device, &reg, 1, out, count, 50) == ESP_OK; }

bool read_table_from(const std::string& path, std::string& out) {
    FILE* f = std::fopen(path.c_str(), "rb");
    if (f == nullptr) return false;
    out.clear();
    char buf[1024];
    std::size_t n;
    while ((n = std::fread(buf, 1, sizeof buf, f)) > 0) {
        out.append(buf, n);
        if (out.size() > hmi::gsl::kMaxTableBytes) break;
    }
    std::fclose(f);
    if (hmi::gsl::table_ok(out)) return true;
    ESP_LOGW(TAG, "%s is not a table of the touch controller", path.c_str());
    return false;
}

bool find_table(std::string& out) {
    if (read_table_from(std::string("/config/") + hmi::gsl::kTableName, out)) return true;
#if CONFIG_HMI_TOUCH_TABLE_FROM_CARD
    if (module_sd_mount(hmi::kSdMount) && read_table_from(std::string(hmi::kSdMount) + "/" + hmi::gsl::kTableName, out)) return true;
#endif
    return false;
}

bool restart_registers() {
    const std::uint8_t zero[4] = {0, 0, 0, 0};
    if (!send_byte(0xe0, 0x88)) return false;
    wait(10);
    if (!send_byte(0xe4, 0x04)) return false;
    wait(10);
    if (!send(0xbc, zero, sizeof zero)) return false;
    wait(10);
    return true;
}

bool load(const std::string& table, gpio_num_t reset) {
    gpio_set_level(reset, 0);
    wait(20);
    gpio_set_level(reset, 1);
    wait(20);
    if (!send_byte(0x88, 0x01)) return false;
    wait(10);
    if (!send_byte(0xe4, 0x04)) return false;
    wait(10);
    if (!send_byte(0xe0, 0x00)) return false;
    wait(20);
    if (!restart_registers()) return false;
    for (std::size_t i = 0; i < table.size(); i += hmi::gsl::kRecordBytes) {
        const std::uint8_t* record = reinterpret_cast<const std::uint8_t*>(table.data()) + i;
        // the page register takes one byte, every other one the four of the word
        if (!send(record[0], record + 1, record[0] == hmi::gsl::kPageRegister ? 1 : 4)) return false;
    }
    if (!send_byte(0xe0, 0x00)) return false;
    wait(10);
    if (!restart_registers()) return false;
    if (!send_byte(0xe0, 0x00)) return false;
    wait(40);
    std::uint8_t status[4] = {};
    if (!receive(hmi::gsl::kStatusRegister, status, sizeof status)) return false;
    return status[0] == 0x5a && status[1] == 0x5a && status[2] == 0x5a && status[3] == 0x5a;
}

void read_touch(lv_indev_t* /*indev*/, lv_indev_data_t* data) {
    static hmi::gsl::Touch last;
    std::uint8_t packet[hmi::gsl::kPacketBytes];
    hmi::gsl::Touch now;
    if (receive(hmi::gsl::kDataRegister, packet, sizeof packet)) now = hmi::gsl::decode(packet, sizeof packet);
    if (now.down) last = now;
    // a release is reported where the finger was
    data->point.x = last.x;
    data->point.y = last.y;
    data->state = now.down ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

}  // namespace

extern "C" bool touch_gsl3680_start(i2c_master_bus_handle_t bus, lv_display_t* display, int reset_gpio) {
    if (i2c_master_probe(bus, hmi::gsl::kAddress, 50) != ESP_OK) return false;
    std::string table;
    if (!find_table(table)) {
        ESP_LOGW(TAG, "a GSL3680 answers but there is no %s: no touch (see docs/HARDWARE.md)", hmi::gsl::kTableName);
        return false;
    }
    i2c_device_config_t config = {};
    config.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    config.device_address = hmi::gsl::kAddress;
    config.scl_speed_hz = 100000;
    if (i2c_master_bus_add_device(bus, &config, &s_device) != ESP_OK) return false;
    if (!load(table, static_cast<gpio_num_t>(reset_gpio))) {
        ESP_LOGW(TAG, "the controller does not start with this table: no touch");
        i2c_master_bus_rm_device(s_device);
        s_device = nullptr;
        return false;
    }
    if (!lvgl_port_lock(0)) return false;
    lv_indev_t* indev = lv_indev_create();
    if (indev != nullptr) {
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(indev, read_touch);
        lv_indev_set_display(indev, display);
    }
    lvgl_port_unlock();
    return indev != nullptr;
}
