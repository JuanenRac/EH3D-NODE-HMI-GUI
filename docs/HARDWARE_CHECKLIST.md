# Hardware checklist - EH3D-NODE-HMI-GUI

The screen has no API of its own: it reads the EH3D server (port 18200). The automatic part needs no screen and no server.

## Automatic part

```
python tools/selftest.py
python tools/selftest.py --cfg hmi.cfg --server SERVER:18200 --token TOKEN --machine ID
```

It checks every line of the settings file with the rules of the firmware, starts a toy server on this computer and checks that its answers are what the screen expects. With `--server` it asks a real server the same questions using GET only: nothing is started, paused or cancelled. The token can also come from `EH3D_TOKEN` and is never printed. The report `selftest_hmi-gui_<time>.txt` lands in the current folder.

A value in `hmi.cfg` must stand alone on its line: a `#` comment after a value is read as part of the value and the line is rejected. Comments go on their own lines.

## By hand (with the screen and a server running)

| Step | What to do | What must happen | Result |
|---|---|---|---|
| 1 | Power the screen with `hmi.cfg` and `server_host` set | The home page shows the machine and its state within a few seconds | |
| 2 | Unplug the server | The screen says the link is lost and offers no movement; it recovers when the server returns | |
| 3 | Touch Start on a prepared job | A confirmation appears; nothing moves until you confirm | |
| 4 | Touch Pause, then Resume | The job pauses at a safe point and resumes from the same hole | |
| 5 | Touch Stop during a job | The machine stops at once, whatever the screen was showing | |
| 6 | Make the job reach a tool change | The notice sounds (if switched on) and the screen asks for the tool | |
| 7 | Cause a message (a refused move, a job error) | The message is shown in the language of `hmi.cfg` and can be dismissed | |
| 8 | Leave the screen idle | It dims after `idle_dim_s` and wakes on touch | |
| 9 | Turn the unit to portrait | The layout follows (with `orientation = auto`) | |
| 10 | Set `machine_kind = laser` (or `pnp`) and restart | The spindle button and the visual check of the board disappear; the screen never starts a spindle | |
