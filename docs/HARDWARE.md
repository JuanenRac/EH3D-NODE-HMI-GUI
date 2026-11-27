# The module

The screen is built on the Guition JC8012P4A1C module:

| Part | What it is |
|---|---|
| Processor | ESP32-P4 with 16 MB of flash and 32 MB of external RAM |
| Wireless | ESP32-C6 for Wi-Fi and Bluetooth, reached through ESP-Hosted |
| Display | 10.1 inch, 800 x 1280, JD9365 controller on MIPI-DSI, capacitive touch |
| Sound | ES8311 codec, NS4150 speaker amplifier, ES7210 microphone input with echo cancellation |
| Storage | a card slot and a battery connector |

The unit this firmware is meant for has the **new** panel variant (the maker ships two firmware demos, one for each).

## Pins the maker's demo uses

| Function | Pins |
|---|---|
| I2C (codec, touch) | SDA 7, SCL 8 |
| I2S (codec) | MCLK 13, SCLK 12, LRCK 10, DOUT 9, DIN 11 |
| Display reset | 27 |
| Touch reset and interrupt | 22 and 21 (new panel) |

These come from the board support code of the maker's demo and are to be checked against the schematic of the unit before anything is connected. The backlight (GPIO 23 by default, `HMI_BACKLIGHT_GPIO`) is taken from the demos of the maker for this family of modules; the pin that enables the amplifier is not known (`HMI_AMP_GPIO`, `-1`: not driven). The stop input is not one of them: `HMI_HW_STOP_GPIO` in menuconfig names the pin, and `-1` means that no stop is wired, which the screen treats as "do not control the machine" while `hw_stop_required` is on.

## The drivers

`main/module_p4.c` starts each part with its own driver from the component registry: JD9365 (display, with the default timing and initialisation commands of the driver), GT911 (touch, if one answers), ES8311 through `esp_codec_dev` (sound) and the SD card of SDMMC slot 0. No board package is used: the one for Espressif's own board drives other panels and brings camera code whose licence allows Espressif chips only. The module is reported to carry a GSL3680 touch controller instead of a GT911; this is to be checked on the unit.

## The table of a GSL3680

A GSL3680 keeps no program: it is given a table at each start. That table comes from the maker of the panel and has no licence that lets this project pass it on, so it is **not in this repository nor in its releases**: you provide it.

1. Get `gsl3680_firmware.h` from the maker of your module (for the JC8012P4A1C, Guition's component `guition/esp_lcd_touch_gsl3680` of the ESP-IDF component registry carries it).
2. `python tools/gsl3680_table.py gsl3680_firmware.h -o gsl3680.fw`
3. Put `gsl3680.fw` in the root of the microSD card, or in the settings partition next to `hmi.cfg` (looked at first). `HMI_TOUCH_TABLE_FROM_CARD` switches the card off.

Without the file the screen starts without touch and says so in its log. A table made for another panel gives wrong positions: use the one of your module.

## The stop

The emergency stop of a machine is a hardware button wired to cut the drive on its own. This screen is not part of that chain: it reads one input to show the state and to refuse moves, and nothing it does can clear a stop. See the README for the rules of the gate.

## Standing up and lying down

The panel is a portrait panel (800 x 1280). Lying down the picture is turned 90 degrees by the display driver (`board::set_rotation`) and the touch is turned with it (`touch_to_screen`, tested). `orientation = auto` follows the way the module is mounted as the application tells it; there is no tilt sensor read yet, so `auto` means lying down until one is.

## The microSD card

The card is mounted at `/sdcard` by `main/module_p4.c` and the screen never leaves it. It lists the drilling, routing, Gerber, DXF and ZIP files, reads the chosen one (at most 512 KB) and the tool table of the same name when there is one, and writes the answer of the server for each job as a report that does not replace an older one.

## Not done yet

The link to the node of the machine for the direct moves of the screen, the speech recogniser and the speaker path, the viewer tab, and a tilt sensor for `orientation = auto`.
