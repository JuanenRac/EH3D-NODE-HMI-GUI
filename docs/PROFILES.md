# Profiles: the Bungard screen is this screen plus settings

EH3D-NODE-HMI-GUI is the generic touch screen of the EH3D node family. EH3D-NODE-BUNGARD-HMI is the same screen for one machine, the Bungard CCD drill and router. The aim is one
screen and one profile per machine, so a fix is made once. This page says where the two stand, what a profile is and what is still written only in the Bungard repository.

## What a profile is

A profile is a settings file. `profiles/bungard.cfg` holds the lines that the Bungard screen carries on top of the generic `hmi.cfg.example`: the kind of machine (`machine_kind = cnc`),
the tool bar of the machine page (`tool_bar`), the panel that moves the machine by hand (`jog_*`) and the clock of the end of a job (`clock_utc_offset_min`). Append it to `hmi.cfg`.
Every key is read by `core/hmi_config.hpp`, which is the same file in both repositories, so a generic screen with the profile reads the same settings as the Bungard screen.
Every value is a line of the file, never a constant of the code, and each safety switch starts in its conservative position.

`py -3 -I tools/check_profile_parity.py` checks it when the two repositories are side by side: the generic `hmi.cfg.example` plus the profile must be the `hmi.cfg.example` of the
Bungard screen, and every key of the profile must be one that the generic screen reads.

## What is shared today

The files of the core (`core/`), the host tests of the core (`tests/`), the sockets of the link to the server (`main/server_link.*`), the board (`main/board*`), the image confirmation,
the `sdkconfig` files and the texts of the 37 languages (`locales/`) are the same text in both repositories once the names of the projects are made equal. The firmware base of
EH3D-COMMON copies them. `tools/check_profile_parity.py` compares them and fails on a difference that `tools/profile_parity_baseline.json` does not list, and on a listed difference
that has been closed.

## What is still different

| Difference | Where | What is needed to close it |
|---|---|---|
| The panel that moves the machine by hand, its page and its buttons | `core/jog_panel.hpp`, the pages in `main/ui.cpp` and `main/app_main.cpp` of the Bungard screen | Write the page in the generic screen behind `jog_*` and `machine_kind = cnc` (its settings are already read by the shared core), then the Bungard screen needs no code of its own for it |
| The card under the progress bar (tool, times, position, counters) | `core/job_card.hpp` and the machine page of the Bungard screen | The same: write it in the generic screen, shown for the CNC kind with `tool_bar` on |
| The parity tests of the desktop program | `tests/test_parity.cpp` | Move with the two parts above |
| The tests of the kind of machine and of the bar of the tool changer | `tests/test_machine.cpp`, `tests/test_rack.cpp` exist only here | Copy to the Bungard screen with the next copy of the firmware base |
| The login of the application `hmi`, the table of refusals by code, the connection settings and the machines of the plugins | `core/session.hpp`, `core/server_errors.hpp`, `core/server_errors_gen.hpp`, `main/server_link.*`, `core/server_link.hpp`, `core/hmi_config.hpp` and `tests/test_session.cpp` exist here first | Add the new files to the list of the firmware base and copy; until then the baseline lists them |
| The screens themselves | `main/app_main.cpp`, `main/ui.cpp` | They are not compared; after the two panels above are in the generic screen the Bungard ones are the same file |

When every line above is closed, EH3D-NODE-BUNGARD-HMI holds only its manifest, its profile and its documentation, and the firmware of the Bungard machine is built from the generic
screen with `profiles/bungard.cfg`. Nothing of the Bungard repository is deleted before that.
