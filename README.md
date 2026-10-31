<p align="center">
  <img src="images/EH3D_BANNER.svg" alt="EH3D-NODE-HMI-GUI banner" width="100%">
</p>

# 🖥️ EH3D-NODE-HMI-GUI

<p align="center">🇺🇸 <b>English</b> | <a href="README_spa.md">🇪🇸 Español</a> | <a href="README_fra.md">🇫🇷 Français</a> | <a href="README_ita.md">🇮🇹 Italiano</a> | <a href="README_deu.md">🇩🇪 Deutsch</a> | <a href="README_zho.md">🇨🇳 中文</a> | <a href="README_jpn.md">🇯🇵 日本語</a> | <a href="README_por.md">🇵🇹 Português</a></p>

### A 10.1-inch touch screen with an ESP32-P4 that controls a CNC machine, a laser or a pick and place machine on its own: the screens and the flow of the desktop programs, in eight languages, spoken alerts and orders, and a hardware stop that does not go through the screen.

<p align="center">
  <img src="https://img.shields.io/badge/License-GPL%203.0--or--later-blue.svg" alt="GPL 3.0 or later">
  <img src="https://img.shields.io/badge/Stack-C%2B%2B17%20%2B%20ESP--IDF-00599C.svg" alt="C++17 and ESP-IDF">
  <img src="https://img.shields.io/badge/Languages-8-00E5FF.svg" alt="Languages">
  <img src="https://img.shields.io/badge/Version-0.0.1-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-scaffolding-FFB020.svg" alt="Maturity">
</p>

---

**Honesty check - what actually runs today:** the rules of the screen are written in C++17 and tested on a computer: the settings file with its safe defaults, the gate that every move and every spindle order must pass (a confirmation on the screen, the hardware-stop latch, the armed link), the spoken orders and alerts in eight languages, the texts of the screens, the standing and lying layouts, the files of the microSD card, and the client of EH3D-SERVER (the requests, the reading of the fleet and the flow of a job from a file of the card to its end). The ESP-IDF firmware with the LVGL screens is written for the Guition JC8012P4A1C module but **has not been built, flashed or run on a module**; it has never talked to a real server, and the moves of the machine itself (jog, home) wait for the link to the node, which is not written yet.

---

> **Safety and security status:** this is not a safety system. The stop that protects people and the machine is a hardware button that cuts the drive by itself, wired apart from this screen; the screen only reads it, shows it and refuses to move anything while it is pressed or has not been acknowledged. A move or a spindle order asks for a confirmation on the screen, a spoken order always does, and spoken orders are off until the operator turns them on. Do not use this screen to run a machine until the whole chain has been tried on a real one. The project is independent: it is not made, endorsed or supported by Bungard Elektronik or Guition, and product names appear only to say which machines and modules it is meant to work with.

## 📋 At a glance

| | |
|---|---|
| Version | `0.0.1` |
| Maturity | Scaffolding |
| Tests | 5 host test programs (1018 checks counted) |
| Stack | C++17, ESP-IDF 5.5, LVGL 9, ESP32-P4 |
| On real hardware | No: the firmware has not been built or flashed, and nothing has been connected to a machine |

**Jump to:** [Overview](#-overview) · [Architecture](#-architecture) · [Quick start](#-quick-start) · [Testing](#-testing) · [Structure](#-repository-structure) · [Related](#-related-projects)

## 🚦 Project status

Each line is one part of this repository and the light says how far it is today. It is written from what the repository can show (tests, builds, runs), not from what is planned.

🟢 Done and verified · 🟡 Works, with known limits · 🟠 Partly done or not verified yet · 🔴 Not done or not working

| Area | State | What it means here |
|---|---|---|
| Software | 🟠 | The ESP-IDF project (ESP32-P4, LVGL 9, the settings file in its own partition) is written; it has not been built or flashed. |
| Tests | 🟢 | 5 host test programs pass (`ctest`): 1018 checks in the settings file, the gate, the spoken orders and alerts, the keyboards, the recorded notices, the server client and the texts of the screens in eight languages. |
| Safety rules | 🟡 | Every move and spindle order goes through the gate: confirmation on the screen, hardware-stop latch that only an acknowledgement on the screen clears once the button is up, no move while the link is not armed. Covered by tests; never run with a real stop button. |
| Voice | 🟡 | Spoken orders and alerts are recognised and phrased in eight languages and tested as text; no microphone or speaker has been used. |
| Screens | 🟠 | The machine, job (microSD), machines-and-stations and settings screens are written, lying down and standing up, in the look of the desktop program; the viewer tab is still an empty card. Only a drawing of the layout exists, not a capture of a module. |
| Real hardware | 🔴 | No module has run this firmware, the link to the server does not exist yet, and nothing has been connected to a machine. |
| Documentation | 🟢 | README in eight languages, the settings reference and the hardware notes. |
| Server and card | 🟡 | The files of the microSD card (listing, safe names, kinds), the requests to EH3D-SERVER, the reading of the fleet and the flow of a job (send it ready, start it through the gate, follow it, answer the tool change) are tested on a computer with answers of the server written down; the sockets and the card driver have never run on a module. |

## 🎯 Overview

* The **gate**: every move and every spindle order, from a touch or from the voice, passes one place that decides to run it, to ask for a confirmation or to refuse it with a reason; a stop always goes.
* The **settings file** `hmi.cfg`: every tuned value (language, brightness, the server, the safety switches, the voice) is a line of one file in its own partition, never a number in the code; a value out of range is reported and the safe default stays.
* The **voice**: spoken alerts (job finished, tool change, error, link lost, hardware stop) and a short fixed list of spoken orders in eight languages; the orders that only halt are understood inside a sentence, the ones that move or spin must be said whole and always ask for a confirmation.
* The **screens**: the machine (jog, home, spindle, start, stop), the job, the viewer, the machines and stations of the server and the settings, in the order and with the names of the desktop program; the stop button is always on top.
* The **module**: the Guition JC8012P4A1C (ESP32-P4 with 32 MB of external RAM, a 10.1-inch 800x1280 MIPI-DSI touch display, an ESP32-C6 for Wi-Fi, the ES8311 codec with a speaker amplifier and the ES7210 microphone input). The panel of the unit is the new one.
* **Standing up or lying down:** the setting `orientation` (`auto`, `landscape`, `portrait`) turns the picture of the panel: lying down (1280 x 800) the job, the plan view and the jog pad sit in three columns above the bar of the position, standing up (800 x 1280) they stack; a touch lands where the screen is touched in both.
* **The microSD card and the server, as one flow:** the card lists the drilling and routing files; the screen sends the chosen file to EH3D-SERVER as a job that waits ready, the operator starts it through the gate, and the progress, the stops for a tool change and the end come back from the fleet in real time, the same as on the desktop programs and the node. The answer of the server for each job is saved on the card.

## 🧩 Role in EH3D

EH3D-NODE-HMI-GUI is the operator's own screen for a CNC machine, a laser or a pick and place machine (the same hardware as EH3D-NODE-BUNGARD-HMI): it takes the place of the PC next to the machine, and it is one more client of EH3D-SERVER, like EH3D-DRILLPRO, EH3D-ROUTEPRO and EH3D-BUNGARD-GUI. A job that the screen sends from the microSD card is planned by the server and is at the same moment on the desktop programs; a job started from a desktop program shows its progress here; the server runs it on the machine through the node of the machine. The direct moves of the screen (jog, home) will use the same node, and that link is not written yet.

## 🏛️ Architecture

<p align="center">
  <img src="images/architecture.svg" alt="EH3D-NODE-HMI-GUI architecture: who uses it, what it holds and what it depends on" width="100%">
</p>

How the parts of this repository fit together: who talks to it, what it holds and what it depends on. The picture is drawn from `docs/architecture.json` and checked against the code.

## 🖼️ Screenshots

<p align="center">
  <img src="images/screenshots/screen-machine.png" alt="machine screen lying down" width="100%">
</p>

<p align="center"><em>The machine screen lying down, in English: the job, the plan view and the jog pad in three columns, the position and the stop button on the bar. A drawing of the layout from the texts of the firmware, not a capture of a module.</em></p>

<p align="center">
  <img src="images/screenshots/screen-portrait.png" alt="machine screen standing up" width="40%">
</p>

<p align="center"><em>The same screen standing up, in German: the cards stack and the bar of the position stays at the bottom.</em></p>

<p align="center">
  <img src="images/screenshots/screen-card.png" alt="files of the microSD card" width="100%">
</p>

<p align="center"><em>The job page (here in Italian): the drilling and routing files of the microSD card, the chosen one, and the button that sends it to the server as a job that waits ready.</em></p>

<p align="center">
  <img src="images/screenshots/screen-fleet.png" alt="machines and stations" width="100%">
</p>

<p align="center"><em>Machines and stations (here in Portuguese): every machine of the server by station, with its state and the progress of its job, as the desktop programs see it.</em></p>

<p align="center">
  <img src="images/screenshots/screen-confirm.png" alt="confirmation of a move" width="100%">
</p>

<p align="center"><em>A move asks for a confirmation before anything happens (here in Spanish).</em></p>

<p align="center">
  <img src="images/screenshots/screen-stop.png" alt="hardware stop pressed" width="100%">
</p>

<p align="center"><em>The hardware stop is pressed: the screen says so, dims the buttons that move and refuses to move anything until the release has been acknowledged (here in Japanese).</em></p>

## ✨ Features

* **A stop that cannot be forgotten:** the stop button is always on top and always works; the hardware stop latches, and only an acknowledgement on the screen, with the button up, clears it.
* **Confirmation before motion:** a move or a spindle order shows what it will do and waits for a touch, with a time limit; a new request or a lost link drops it.
* **Eight languages, also by voice:** English, Spanish, Italian, German, French, European Portuguese, Chinese and Japanese on the screens, in the spoken alerts and in the spoken orders.
* **Safe by default:** every safety switch has a switch in the settings and starts in its conservative position: spoken orders off, spoken confirmation off, confirmation on, hardware stop required.
* **The right keyboard for each setting:** a number is typed on a numeric pad that knows the range and the decimals of that setting (the arrows move it by one unit, a value outside the range is not accepted); a text, such as the address of the server or the token, on a full keyboard with the keys of the language in use (the umlauts, the accents, the tilde, the cedilla), with a one-letter shift and a layer of symbols; switches and lists need no keyboard.
* **Buttons with a symbol:** the jog pad, home, the spindle, pause, start, stop and the answers of a question use symbols and the letter of the axis instead of words, in the colours of the desktop program (green to start, blue to pause, red to stop).
* **Recorded notices:** the end of a job and the request for a tool can be said by a recorded voice, in the language and the gender that are set (`sound_finish = voice:es:f`), from the folder `voices` of the card; the recordings are those of EH3D-BUNGARD-GUI. The speaker is not wired in the firmware yet.
* **The job is followed again after a reset:** from the moment the operator starts a job, the screen keeps in its flash which job it is (its id, the machine, its name); after a power cut or a reset it follows the same job again, the next reading of the server says whether it still runs, waits at a tool change or is over, and the cancel button leaves it. The job itself lives in the server. Written and tested on a computer; the firmware has never run on a module.
* **The kind of machine:** `machine_kind` in the settings file (`cnc`, `laser` or `pnp`) decides what the screen may order. Only a CNC machine gets the spindle button, and the gate refuses a spindle order on a laser or a pick and place machine whatever the other settings say: the screen never fires a laser and never starts a pick and place head by itself.

## 🔒 Security & Safety

| Threat | Defense |
|---|---|
| A touch or a spoken order that moves the machine by accident | a move asks for a confirmation on the screen with a time limit; a spoken order always asks; a spoken yes confirms only when the setting `voice_confirm` is on (off by default) |
| The screen starts moving while the hardware stop is pressed | the hardware stop is a latch: pressed, or released but not acknowledged on the screen, every move and spindle order is refused and what waited is dropped; a stop pressed while a confirmation waits cancels it |
| A machine that is not ready | nothing moves while the link is not armed, while the hardware stop input is not wired (`hw_stop_required`, on by default) or when the spindle limit is 0 |
| A spoken word taken for an order | orders come from a short fixed list per language, never free text; a word that only contains an order (`stopwatch`) is no order; spoken orders are off until the operator turns them on |
| A typing mistake in the settings | a value out of range or an unknown key is reported and the safe default stays; no switch opens by a mistake |
| A secret in the firmware or in the repository | nothing secret is compiled in; the token is a setting in its partition and is never shown or logged |

**What is not defended:** the screen is not a safety device and has no way to cut the machine's drive; the hardware stop must do that on its own. There is no link to the server yet, so the screen cannot move a machine at all today. Speech recognition runs as text in the tests; the real microphone path (echo cancellation, noise, accents) has not been tried. Someone with the module in their hands can change the settings file.

## 🔌 Interfaces

| Interface | What it is | Where |
|---|---|---|
| Settings file | `hmi.cfg` in the `config` partition: `key = value` lines; see `hmi.cfg.example` and the table above | `core/hmi_config.hpp` |
| Hardware stop input | one GPIO (`HMI_HW_STOP_GPIO` in menuconfig, active low by default, with a pull-up so an open wire reads as pressed) | `main/board_p4.cpp` |
| Touch display | 800x1280 MIPI-DSI through LVGL 9 | `main/ui.cpp` |
| Sound | ES8311 codec with speaker amplifier and the ES7210 microphone input | `main/board_p4.cpp` |
| microSD | the root of the card at `/sdcard`: drilling, routing, Gerber, DXF and ZIP files are listed and read, reports are written; only plain file names are accepted | `core/sdfiles.hpp`, `main/board_p4.cpp` |
| EH3D-SERVER | HTTP with the access token: `GET /api/v1/fleet` every `poll_s` seconds, `POST /api/v1/machines/:id/jobs`, `POST /api/v1/jobs/:id/start`, `confirm` and `cancel` | `core/server_link.hpp`, `main/server_link.cpp` |

## ⚙️ Configuration

Everything tuned is a line of `hmi.cfg` (copy `hmi.cfg.example` to the `config` partition); a value out of range is reported in the log and the default stays. The values below are the defaults; every safety default is the conservative one.

| Setting | Default | Range | What it does |
|---|---|---|---|
| `language` | `en` | `en es it de fr pt zh ja` | the language of the screens and of the voice |
| `confirm_moves` | on | on / off | the touch alone confirms a move: turn off only on the bench |
| `confirm_timeout_ms` | 8000 | 1000 - 60000 | how long a confirmation waits |
| `jog_max_mm` | 20 | 1 - 200 | the longest single jog the screen sends |
| `spindle_max_rpm` | 24000 | 0 - 65535 | the highest spindle speed it asks for (0: it never starts the spindle) |
| `hw_stop_required` | on | on / off | control the machine only while the hardware stop input is wired and released |
| `voice_orders` | off | on / off | accept spoken orders |
| `voice_confirm` | off | on / off | accept a spoken yes as the confirmation of a move |
| `token` | none | 8 - 64 characters | the access token of the server; write-only, never shown or logged |
| `orientation` | `auto` | `auto landscape portrait` | standing up or lying down; `auto` follows the way the module is mounted |
| `poll_s` | 3 | 1 - 60 | how often the screen asks the server for the fleet |

## 🚀 Quick start

You need a C++17 compiler and CMake for the host tests, and ESP-IDF 5.5.5 for the firmware (the module's ESP32-P4 target).

```bash
build.bat            # the host tests (Windows)  -  ./build.sh on Linux
python -I tools/ci_validate.py
```

The firmware has not been built yet: when it is, `idf.py set-target esp32p4 && idf.py build` makes the image, and a partition named `config` holds `hmi.cfg`.

## 🧪 Testing

The host tests (`build.bat` or `./build.sh`, then `ctest`) are two programs and all of it passes: `test_hmi` (638 checks) on the settings file, the gate (stop always goes, hardware-stop latch, confirmation, timeout and clock wrap-around, voice rules, switches), the spoken orders and alerts and the screen texts in eight languages; and `test_workflow` (115 checks) on the orientation and the touch, the files of the card (kinds, safe names, sorting), the JSON reader, the requests and answers of the server, and the flow of a job from the card to its end. The other two programs cover the keyboards (`test_keyboard`, 137 checks: every setting has its keyboard, the ranges of the pad are the ones of the settings file, the decimals, the sign, the arrows, the full keyboard of each language, the shift, the symbols and the length) and the recorded notices (`test_notices`, 66 checks: the setting, the file of each language and gender).

## 🏗️ Build & Release

The version lives in `eh3d.project.json`, and each version has its entry in [CHANGELOG.md](CHANGELOG.md). `build.sh` / `build.bat` build and run the host tests and `test.sh` / `test.bat` run the repository checks. The firmware is built outside this repository's scripts, in a visible window, one build at a time.

## 🩺 Troubleshooting

| Symptom | What to do |
|---|---|
| Nothing moves and the screen says the hardware stop is pressed or not acknowledged | Release the button, then press the acknowledge button on the screen: a stop that was released is not cleared by itself. |
| The screen says the machine is not armed | There is no link to the machine yet in this version; the gate refuses every move until a link arms it. |
| The screen says the hardware stop input is not wired | Wire the stop button to the pin set in menuconfig (`HMI_HW_STOP_GPIO`), or turn `hw_stop_required` off on the bench only. |
| A spoken order does nothing | Spoken orders are off by default: set `voice_orders = on`. Orders that move or spin ask for a touch on the screen unless `voice_confirm` is on. |

## 📂 Repository Structure

<p align="center">
  <img src="images/repository-structure.svg" alt="Repository structure of EH3D-NODE-HMI-GUI" width="100%">
</p>

<details>
<summary>Text version of the tree</summary>

```text
EH3D-NODE-HMI-GUI/
├── core/
│   ├── hmi_config.hpp lang.hpp                       the settings file and its safe defaults
│   ├── gate.hpp                                      the gate: confirmation, hardware-stop latch
│   ├── machine.hpp                                   the kind of machine
│   ├── voice.hpp                                     spoken orders and alerts in eight languages
│   ├── orientation.hpp sdfiles.hpp                   standing and lying layouts, files of the card
│   ├── server_link.hpp workflow.hpp job_memory.hpp   client of the server and the flow of a job
│   └── ui_text.hpp keyboard.hpp notices.hpp          texts, keyboards and notices
├── main/
│   ├── app_main.cpp                                  start, settings, the loop of the gate
│   ├── ui.cpp ui.hpp                                 the LVGL screens and the stop button
│   ├── board_p4.cpp board.hpp                        the module: display, sound, stop input
│   ├── server_link.cpp server_link.hpp               the sockets of the client of the server
│   └── idf_component.yml Kconfig.projbuild           component list and menuconfig pins
├── tests/                                            host tests of the settings, the gate, the voice and the screen texts
├── docs/                                             the hardware notes and the two pictures' sources
├── tools/                                            CI check, README check and the picture tools
├── CMakeLists.txt                                    the ESP-IDF project
├── partitions.csv                                    the flash layout with two slots
├── hmi.cfg.example                                   every setting with its default and range
├── sdkconfig.defaults                                ESP-IDF settings for the ESP32-P4 module
├── sdkconfig.board.p4                                settings of the board
├── dependencies.lock                                 the exact versions of the components
├── images/                                           banner and the Architecture and Repository Structure pictures
├── .github/                                          CI and release workflows, issue and pull-request templates
├── build.sh build.bat                                build and run the host tests in a live panel
├── eh3d.project.json                                 manifest: version, role, maturity and build command
├── CHANGELOG.md                                      what changed in each version
├── SECURITY.md SUPPORT.md                            how to report a vulnerability and where to ask
├── CONTRIBUTING.md CODE_OF_CONDUCT.md LICENSE        how to contribute, the code of conduct and the licence
└── bump_manifest_version.py                          version utility of the manifest
```

</details>

## 🗺️ Roadmap & Known limits

**Known limits today**

* Nothing has run on a module: the firmware has not been built, flashed or tried, and the board support package it asks for may need changes for this module.
* The screen reaches the machine only through a job of EH3D-SERVER; its own moves (jog, home, spindle) ask the gate and then wait for the link to the node, which is not written yet. The client of the server and the card have never run on a module.
* The pin of the hardware stop and the touch controller are not fixed yet: they must be checked against the schematic of the unit.
* The speech recogniser and the speaker are not in the firmware yet; the orders and alerts are tested as text.
* The logic of the desktop programs (formats, controller, fleet) is not reused: the screens follow the desktop program, but the rules are rewritten in C++.

**Next**

* Build the firmware and start the module: display, touch and sound.
* Write the link to EH3D-SERVER and to the node, and fill the other tabs of the screens.
* Add the speech recogniser and the speaker, and try them with echo cancellation.
* Wire a real stop button and try the latch on a machine.

## 🌍 Languages

README and texts exist in English, Spanish, French, Italian, German, Chinese, Japanese and European Portuguese.

## 🔗 Related Projects

* [EH3D-DRILLPRO](https://github.com/JuanenRac/EH3D-DRILLPRO): the desktop program the screens follow
* [EH3D-ROUTEPRO](https://github.com/JuanenRac/EH3D-ROUTEPRO): the routing program; shares the machine file and the protocol library
* [EH3D-SERVER](https://github.com/JuanenRac/EH3D-SERVER): the center of the family: it runs the jobs of every client
* [EH3D-NODE-BUNGARD](https://github.com/JuanenRac/EH3D-NODE-BUNGARD): the node on the serial port of the control unit of the machine
* [EH3D-COMMON](https://github.com/JuanenRac/EH3D-COMMON): the common parts of the family (formats, controller, fleet, themes)

## 📚 Documentation

* The settings: `hmi.cfg.example` lists every key with its default and range.
* The hardware: the module's pins and the notes that come with the demo of the maker, in `docs/HARDWARE.md`.
* [Changelog](CHANGELOG.md)

## 🤝 Contributing & Community

* Before a pull request run the host tests (`./build.sh` or `build.bat`) and `python -I tools/ci_validate.py`: all must pass. See [CONTRIBUTING.md](CONTRIBUTING.md) and the [code of conduct](CODE_OF_CONDUCT.md).
* The README exists in eight languages: a change to the text is made in all eight files, and the two pictures are drawn from `docs/architecture.json` and `docs/structure.json` by `tools/make_architecture.py` and `tools/make_structure.py`.
* Nothing that lets the screen move a machine changes without a test of the gate that covers it and a note of what was observed on hardware.
* Report a vulnerability privately: [SECURITY.md](SECURITY.md). Questions, ideas and reports: electrohobby3d@gmail.com ([SUPPORT.md](SUPPORT.md)).

## 👤 AUTHOR

**JuanenRac (Electro Hobby 3D)** · electrohobby3d@gmail.com

## 📜 LICENSE

Software: GPL-3.0-or-later. See [LICENSE](LICENSE). Hardware designs of the suite use CERN OHL-S.
