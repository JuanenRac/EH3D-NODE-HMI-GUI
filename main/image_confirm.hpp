// EH3D-NODE-HMI-GUI - main/image_confirm.hpp: a new firmware image is confirmed only once the screen has run for a while.
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
//
// With the roll-back of the boot loader on (CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE), an image that is written by an update starts "on probation": if it resets before it is confirmed, the boot
// loader goes back to the image that ran before. The screen confirms its image when the display has started and the main loop has run for `kSeconds`; when the display does not start, an
// image on probation goes back at once. An image that was flashed through the cable is not on probation and none of this does anything to it. Nothing here has run on a board.
#pragma once

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace imgconfirm {

constexpr int kSeconds = 30;

inline bool on_probation() {
    esp_ota_img_states_t state;
    const esp_partition_t* running = esp_ota_get_running_partition();
    return running != nullptr && esp_ota_get_state_partition(running, &state) == ESP_OK && state == ESP_OTA_IMG_PENDING_VERIFY;
}

inline void confirm_task(void*) {
    vTaskDelay(pdMS_TO_TICKS(kSeconds * 1000));
    if (on_probation()) {
        esp_ota_mark_app_valid_cancel_rollback();
        ESP_LOGI("image", "the new firmware is confirmed");
    }
    vTaskDelete(nullptr);
}

// Called once the display has started.
inline void start() { xTaskCreate(confirm_task, "confirm", 3072, nullptr, 2, nullptr); }

// Called when the display does not start: an image on probation goes back to the previous one.
inline void display_failed() {
    if (on_probation()) {
        ESP_LOGE("image", "the display does not start: going back to the previous firmware");
        esp_ota_mark_app_invalid_rollback_and_reboot();
    }
}

}  // namespace imgconfirm
