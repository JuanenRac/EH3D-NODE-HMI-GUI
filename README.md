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
  <img src="https://img.shields.io/badge/Version-0.0.7-0f6cbd.svg" alt="Version">
  <img src="https://img.shields.io/badge/Maturity-scaffolding-FFB020.svg" alt="Maturity">
</p>

---

**Honesty check - what actually runs today:** the rules of the screen are written in C++17 and tested on a computer: the settings file with its safe defaults, the gate that every move and every spindle order must pass (a confirmation on the screen, the hardware-stop latch, the armed link), the spoken orders and alerts in eight languages, the texts of the screens, the standing and lying layouts, the files of the microSD card, and the client of EH3D-SERVER (the requests, the reading of the fleet and the flow of a job from a file of the card to its end). The ESP-IDF firmware with the LVGL screens is written for the Guition JC8012P4A1C module but **has not been built, flashed or run on a module**; it has never talked to a real server, and the moves of the machine itself (jog, home) wait for the link to the node, which is not written yet.

---

> **Safety and security status:** this is not a safety system. The stop that protects people and the machine is a hardware button that cuts the drive by itself, wired apart from this screen; the screen only reads it, shows it and refuses to move anything while it is pressed or has not been acknowledged. A move or a spindle order asks for a confirmation on the screen, a spoken order always does, and spoken orders are off until the operator turns them on. Do not use this screen to run a machine until the whole chain has been tried on a real one. The project is independent: it is not made, endorsed or supported by Bungard Elektronik or Guition, and product names appear only to say which machines and modules it is meant to work with.

## 📋 At a glance

| | |
|---|---|
| Version | `0.0.7` |
| Maturity | Scaffolding |
| Tests | 10 host test programs (211934 checks counted) |
| Stack | C++17, ESP-IDF 5.5, LVGL 9, ESP32-P4 |
| On real hardware | No: the firmware has not been built or flashed, and nothing has been connected to a machine |

**Jump to:** [Overview](#-overview) · [Architecture](#-architecture) · [Quick start](#-quick-start) · [Testing](#-testing) · [Structure](#-repository-structure) · [Related](#-related-projects)

## 🚦 Project status

Each line is one part of this repository and the light says how far it is today. It is written from what the repository can show (tests, builds, runs), not from what is planned.

🟢 Done and verified · 🟡 Works, with known limits · 🟠 Partly done or not verified yet · 🔴 Not done or not working

| Area | State | What it means here |
|---|---|---|
| Software | 🟠 | The ESP-IDF project (ESP32-P4, LVGL 9, the settings file in its own partition) is written; it has not been built or flashed. |
| Tests | 🟡 | 10 host test programs pass (`ctest`): 211934 checks in the settings file, the gate, the spoken orders and alerts, the keyboards, the recorded notices, the server client and the texts of the screens in eight languages. |
| Safety rules | 🟡 | Every move and spindle order goes through the gate: confirmation on the screen, hardware-stop latch that only an acknowledgement on the screen clears once the button is up, no move while the link is not armed. Covered by tests; never run with a real stop button. |
| Voice | 🟡 | Spoken orders and alerts are recognised and phrased in eight languages and tested as text; no microphone or speaker has been used. |
| Screens | 🟠 | The machine, job (microSD), machines-and-stations and settings screens are written, lying down and standing up, in the look of the desktop program; the viewer tab is still an empty card. Only a drawing of the layout exists, not a capture of a module. |
| Real hardware | 🔴 | No module has run this firmware, the link to the server does not exist yet, and nothing has been connected to a machine. |
| Documentation | 🟡 | README in eight languages, the settings reference and the hardware notes. |
| Server and card | 🟡 | The files of the microSD card (listing, safe names, kinds), the requests to EH3D-SERVER, the reading of the fleet and the flow of a job (send it ready, start it through the gate, follow it, answer the tool change) are tested on a computer with answers of the server written down; the sockets and the card driver have never run on a module. |

## 🎯 Overview

* The **gate**: every move and every spindle order, from a touch or from the voice, passes one place that decides to run it, to ask for a confirmation or to refuse it with a reason; a stop always goes.
* The **settings file** `hmi.cfg`: every tuned value (language, brightness, the server, the safety switches, the voice) is a line of one file in its own partition, never a number in the code; a value out of range is reported and the safe default stays.
* The **voice**: spoken alerts (job finished, tool change, error, link lost, hardware stop) and a short fixed list of spoken orders in eight languages; the orders that only halt are understood inside a sentence, the ones that move or spin must be said whole and always ask for a confirmation.
* The **screens**: the machine (jog, home, spindle, start, stop), the job, the viewer, the machines and stations of the server and the settings, in the order and with the names of the desktop program; the stop button is always on top.
* The **board against the table** and the **end of a job**: when a drilling or routing file is chosen on the card, the size of its box is compared with the table of the machine (the course the server reports less the offset, or `table_width_mm` and `table_depth_mm` of `hmi.cfg`); a board that fits neither lying nor turned is warned about on the job page with its size and by how much it goes over, and the job is still allowed (the server refuses what lies outside the table). A job that was cancelled or failed is shown as such with the reason of the server, is never taken for a finished one (no finished notice is played for it) and is kept to go on from the holes already done; what the server notes about a job that asks more than the spindle gives is shown when the job is made.
* The **module**: the Guition JC8012P4A1C (ESP32-P4 with 32 MB of external RAM, a 10.1-inch 800x1280 MIPI-DSI touch display, an ESP32-C6 for Wi-Fi, the ES8311 codec with a speaker amplifier and the ES7210 microphone input). The panel of the unit is the new one.
* **Standing up or lying down:** the setting `orientation` (`auto`, `landscape`, `portrait`) turns the picture of the panel: lying down (1280 x 800) the job, the plan view and the jog pad sit in three columns above the bar of the position, standing up (800 x 1280) they stack; a touch lands where the screen is touched in both.
* **The microSD card and the server, as one flow:** the card lists the drilling and routing files; the screen sends the chosen file to EH3D-SERVER as a job that waits ready, the operator starts it through the gate, and the progress, the stops for a tool change and the end come back from the fleet in real time, the same as on the desktop programs and the node. The answer of the server for each job is saved on the card.
* **The tools of the file, and the spindle:** when a file is chosen the job page shows what it holds (the tools, their diameter and how many holes or cuts each one has) and a chip for each tool: a tap switches it off or on, and the job is sent to the server without it (`skip_tools`); with every tool off nothing is sent. It also shows the spindle the machine carries (`original` or `vfd`) with its turns, as the server says. Written and tested on a computer (`core/job_tools.hpp`); not seen on a module.
* **The gate keeps to the spindle:** a spindle order carries the turns it asks for (`spindle_rpm`, what the spindle button asks), and the gate never lets it pass the lower of `spindle_max_rpm` and the top speed of the spindle the machine carries, as the server says it. With `spindle_over = clip` (the default) an order for more is cut to that speed and the screen says so; with `refuse` it is refused with a reason. The confirmation shows the turns the spindle will really get. Written and tested on a computer; not seen on a module.
* **A stopped job goes on:** when a job leaves the machine the screen asks the server how it ended; a job that was cancelled or failed is shown as stopped, with the holes (cuts) it did, and a button sends the same file again with `skip` set to those already done, so only the rest is drilled or routed. The tools switched off stay off and the start goes through the gate like any other. Written and tested on a computer; not seen on a module. When the server answers `resume-from` (it keeps the request of every job), the screen asks it for the new job instead of sending the file again, so a job goes on even after the screen was reset and has no file; the new job is only made ready, the page names the job it goes on from, and the operator starts it through the gate. An older server (no such route) is answered with the file the screen keeps, as before.
* **Drilling and then routing:** a drilling file and a routing file chosen one after the other are two programs of one job: when the server says the drilling is done, the routing is made and waits for the operator to start it, after the change of tool, and the job page names the program that comes next. Written and tested on a computer; not seen on a module.

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
* **Pause, resume and the tool change:** the pause button holds the job on the server (it never cancels it) and a button lets it go on; at a tool change a button answers it once the tool is changed. The server holds only G-code jobs, so for a drilling or routing job it answers with its reason, which the screen shows. The stop is sent by a task of its own, to the job the server shows on the machine (also one started from the desktop), with three tries and a short wait, and pressing the hardware stop sends it too; if it cannot be sent, the screen says so. Written and tested on a computer; the firmware has never run on a module.
* **G-code programs from the card:** a `.nc`, `.gcode` or `.tap` file is sent to EH3D-SERVER as a G-code job of its own (never paired with a drilling or routing file) that waits ready; the operator starts it through the gate, can pause and resume it (the server holds G-code jobs), and a stopped one goes on through the server from the lines already done. The server checks the program against the machine and refuses it with its reason on a machine that does not take G-code. Written and tested on a computer; the firmware has never run on a module.
* **The bar of the tool changer (rules and checks):** on a machine whose model says ATC, every tool of the job gets a place in the bar (1 to 14, the small drills first); the start is refused with a sentence while a tool has no place, a place is not defined or two tools share one (`rack_check`); the request of the job carries the places and the chaining of the drilling and the routing (`request_version` 2, `rack_chain`; a server that turns it down gets the job again without them); and a drilling that ends with a routing still to come is a stage of its own, with its notice (`sound_stage`) and a line that says the routing comes next (`stage_window`), not the end of the job. The window to choose the places by hand is not on this screen yet.
* **Confirmation before motion:** a move or a spindle order shows what it will do and waits for a touch, with a time limit; a new request or a lost link drops it.
* **Eight languages, also by voice:** English, Spanish, Italian, German, French, European Portuguese, Chinese and Japanese on the screens, in the spoken alerts and in the spoken orders.
* **Safe by default:** every safety switch has a switch in the settings and starts in its conservative position: spoken orders off, spoken confirmation off, confirmation on, hardware stop required.
* **The right keyboard for each setting:** a number is typed on a numeric pad that knows the range and the decimals of that setting (the arrows move it by one unit, a value outside the range is not accepted); a text, such as the address of the server or the token, on a full keyboard with the keys of the language in use (the umlauts, the accents, the tilde, the cedilla), with a one-letter shift and a layer of symbols; switches and lists need no keyboard.
* **Buttons with a symbol:** the jog pad, home, the spindle, pause, start, stop and the answers of a question use symbols and the letter of the axis instead of words, in the colours of the desktop program (green to start, blue to pause, red to stop).
* **Recorded notices:** the end of a job and the request for a tool can be said by a recorded voice, in the language and the gender that are set (`sound_finish = voice:es:f`), from the folder `voices` of the card; the recordings are those of EH3D-BUNGARD-GUI. The speaker is not wired in the firmware yet.
* **The job is followed again after a reset:** from the moment the operator starts a job, the screen keeps in its flash which job it is (its id, the machine, its name); after a power cut or a reset it follows the same job again, the next reading of the server says whether it still runs, waits at a tool change or is over, and the cancel button leaves it. The job itself lives in the server. Written and tested on a computer; the firmware has never run on a module.
* **The kind of machine:** `machine_kind` in the settings file (`cnc`, `laser` or `pnp`) decides what the screen may order. Only a CNC machine gets the spindle button, and the gate refuses a spindle order on a laser or a pick and place machine whatever the other settings say: the screen never fires a laser and never starts a pick and place head by itself.
* **Thirty-seven languages on the screen:** besides the eight of the tables, the texts of the screens (the tabs, the buttons, the refusals of the gate and, on a CNC machine, the whole quality check) are written in 29 more languages (Bulgarian, Croatian, Czech, Danish, Dutch, Estonian, Finnish, Greek, Hungarian, Irish, Latvian, Lithuanian, Maltese, Polish, Romanian, Slovak, Slovenian, Swedish, Norwegian, Icelandic, Ukrainian, Russian, Serbian, Albanian, Catalan, Galician, Basque, Brazilian Portuguese and Canadian French), one file each in `locales/`, with fonts made only for the characters the screen can draw; the language is chosen in a scrolling list in the settings. The spoken alerts and orders stay in the first eight. A text a language lacks is shown in English. These texts and fonts have been tested on a computer only, not on the real screen.
* **The screen says what is wrong and what is old:** a line of `hmi.cfg` that cannot be used is shown under the top bar (which line, which key, why) instead of being ignored in silence, and the server is checked at the start (a clear note when it does not answer on its port, `18200` by default). The top bar says connected, no connection or other server, and marks what the screen shows as old when the server has not updated it for `stale_after_s` seconds. The confirmation counts down to its abort (`confirm_timeout_ms`) and names the order in the language of the operator; the notes of the application are texts in the 37 languages, no longer English literals. The version of the firmware is on the title bar and in the settings, and every request to the server carries it in the headers `User-Agent` and `X-EH3D-Client`. `sdkconfig.defaults` sets the task watchdog, the stack of the main task and the stack overflow check, the firmware logs the stack each task has left, and `tools/check_firmware_defaults.py` guards these settings. Written and tested on a computer; not built or run on a module.
* **Quality check and notices, on a CNC machine only:** `machine_kind = cnc` adds a sixth page with the verdict of the check of the work through the camera of the drill head (OK, NOT OK or cannot judge), what was found, the ring of LEDs, the profile and the calibration, all through EH3D-SERVER, and the screen says the result aloud when `sound_check_ok` and `sound_check_bad` are set (a beep or a recorded voice). A laser or a pick and place machine has neither the page nor those notices, and the notices of the end of a job and of a tool change work on every kind of machine. Written and tested on a computer; not seen on a real screen.

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
| `spindle_rpm` | 12000 | 0 - 65535 | the turns the spindle button asks for (0: the speed the job says); the gate cuts it to what the spindle gives |
| `spindle_over` | clip | clip / refuse | an order for more turns than the spindle gives: cut to its top speed and say so, or refuse it |
| `hw_stop_required` | on | on / off | control the machine only while the hardware stop input is wired and released |
| `voice_orders` | off | on / off | accept spoken orders |
| `voice_confirm` | off | on / off | accept a spoken yes as the confirmation of a move |
| `token` | none | 8 - 64 characters | the access token of the server; write-only, never shown or logged |
| `orientation` | `auto` | `auto landscape portrait` | standing up or lying down; `auto` follows the way the module is mounted |
| `poll_s` | 3 | 1 - 60 | how often the screen asks the server for the fleet |
| `stale_after_s` | 15 | 2 - 3600 | the data shown are marked old when the server has not updated them for this many seconds |
| `table_width_mm` | 0 | 0 - 5000 | the width of the table of the machine for the warning of a board that does not fit; 0 = the one the server says (the course less the offset) |
| `table_depth_mm` | 0 | 0 - 5000 | the depth of the table; 0 = the one the server says |

## 🚀 Quick start

You need a C++17 compiler and CMake for the host tests, and ESP-IDF 5.5.5 for the firmware (the module's ESP32-P4 target).

```bash
build.bat            # the host tests (Windows)  -  ./build.sh on Linux
python -I tools/ci_validate.py
```

The firmware has not been built yet: when it is, `idf.py set-target esp32p4 && idf.py build` makes the image, and a partition named `config` holds `hmi.cfg`.

## 🧪 Testing

The host tests (`build.bat` or `./build.sh`, then `ctest`) are ten programs and all of it passes: `test_hmi` (1718 checks) on the settings file, the gate (stop always goes, hardware-stop latch, confirmation, timeout and clock wrap-around, voice rules, switches), the spoken orders and alerts and the screen texts in eight languages; and `test_workflow` (365 checks) on the orientation and the touch, the files of the card (kinds, safe names, sorting), the JSON reader, the requests and answers of the server, and the flow of a job from the card to its end. Two more programs cover the keyboards (`test_keyboard`, 235 checks: every setting has its keyboard, the ranges of the pad are the ones of the settings file, the decimals, the sign, the arrows, the full keyboard of each language, the shift, the symbols and the length) and the recorded notices (`test_notices`, 266 checks: the setting, the file of each language and gender). Three more programs: `test_machine` (55 checks: the kinds of machine, the spindle refusal and the sentence in every language), `test_quality` (911 checks: the requests, the reading of the report, every finding) and `test_locales` (205250 checks: every key has a text in each of the 29 other languages, nothing is empty or too long, the script of each language is the right one, and every character that any text can draw is in the range of the generated fonts). Another program, `test_status` (1151 checks: the report of `hmi.cfg`, the state of the link and the age of the data, the notes, the orders with their countdown and the identification of the client). The program `test_rack` (119 checks) covers the bar of the tool changer: the places, the chaining of the drilling and the routing, the request of the second version, a server that does not chain, the wait for the places before a start, the end of a stage and the settings of the bar. The program `test_more` covers the small rules of the screen: the languages and their fall-backs, the sound files that an operator puts on the card, the notices, the memory of the job that is running, the turn of the panel and the files of the card.

Self-test for the day the screen meets the hardware: `python tools/selftest.py` checks `hmi.cfg.example` (or `--cfg FILE`) with the rules of the firmware and starts a toy server on port 18200 to check that its answers are what the screen expects; with `--server HOST:PORT` it asks a real server the same questions using GET only. It sends nothing to hardware. Its tests run in the CI (`python -I -m unittest discover -s tools -p "test_*.py"`). The steps to do by hand on the screen are in [docs/HARDWARE_CHECKLIST.md](docs/HARDWARE_CHECKLIST.md).

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
│   ├── job_tools.hpp                                 the tools of a file and the ones switched off
│   ├── rack.hpp stage_notice.hpp                     the bar of the tool changer and the stage notice
│   ├── quality.hpp                                   the quality check and the ring of LEDs
│   ├── ui_text.hpp keyboard.hpp notices.hpp          texts, keyboards and notices
│   ├── locales.hpp locales_gen.hpp font_ranges.hpp   37 languages and the font ranges
│   └── wav.hpp                                       the recordings and the beeps of the notices
├── main/
│   ├── app_main.cpp                                  start, settings, the loop of the gate
│   ├── ui.cpp ui.hpp                                 the LVGL screens and the stop button
│   ├── fonts/                                        the fonts: Latin to Japanese
│   ├── board_p4.cpp board.hpp                        the module: display, sound, stop input
│   ├── server_link.cpp server_link.hpp               the sockets of the client of the server
│   └── idf_component.yml Kconfig.projbuild           component list and menuconfig pins
├── tests/                                            host tests of the settings, the gate, the voice and the screen texts
├── locales/                                          the texts of the 29 other languages, one file each
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
