// EH3D-NODE-HMI-GUI - main/module_p4.h: the start of the parts of the module with their own drivers (C, because the
// configuration macros of these drivers do not compile as C++). GPL-3.0-or-later.
#pragma once

#include <stdbool.h>

#include "esp_codec_dev.h"

#ifdef __cplusplus
extern "C" {
#endif

/// Starts LVGL, the backlight, the JD9365 panel (800 x 1280, MIPI-DSI) and the touch; false when the panel does not start.
/// A module whose touch controller is not a GT911 starts without touch and says so in the log.
bool module_display_start(void);
/// Duty of the backlight, 0 to 100.
void module_set_brightness(int percent);
/// Starts the ES8311 codec for playing; NULL when it does not answer. Nothing records yet, so the microphone is not started.
esp_codec_dev_handle_t module_speaker_start(void);
/// Mounts the microSD card of SDMMC slot 0 at `mount_point`; false when there is no card or it cannot be read.
bool module_sd_mount(const char *mount_point);

#ifdef __cplusplus
}
#endif
