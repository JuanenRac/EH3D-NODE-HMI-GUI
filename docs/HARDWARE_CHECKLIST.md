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
| 11 | Set `server_user` and `server_password` of a real login (and no `token`), restart | The screen logs in as the application `hmi` (the server's audit shows `login` with app `hmi`) and the fleet is read with that login | |
| 12 | Wait until the login is in the last quarter of its life (set a short `session_hours` on the server for the test) | The server's audit shows `session_refresh`; the screen stays connected without a new login | |
| 13 | End the login from the server (`POST /auth/logout` with that token, or close it from another client) | The next request is refused with 401, the screen forgets the token and logs in again on its own | |
| 14 | Give a wrong password | The screen says the refusal in its language (the text of `bad_credentials`); the next try waits 30 s, then 60 s | |
| 15 | Set `server_timeout_s = 1`, then switch the server off | The screen reports no answer within about a second per try, with `server_retries` more tries; a job request is never sent twice | |
| 16 | Make a machine of a plugin (for example a demo or a laser) on the server and latch it | The fleet page shows it with its detail, in red with the warning mark | |
| 17 | With the login of an administrator (`server_user`) open the settings page | The block "Server settings" says the values are read from the server and lists 107 settings in 17 groups with their values; reading the answer (some 250 KB) does not reset the screen or stall the touch | |
| 18 | Use the login of an operator (or only the `token`) and restart | The block shows the copy the screen carries, read only, without values, and says that the server gives its settings to an administrator only | |
| 19 | Set the language to Japanese, then to Chinese | The labels are drawn in that language without boxes | |
| 20 | Put a proxy page or another service on the address of the server | The block shows the copy, nothing is changed and no token goes to the other service | |
| 21 | Set `settings_write = on` and a key in `settings_write_keys`, with an administrator login (waits for the controls: they are not drawn yet) | A switch asks, a risky one asks twice, the server's `weak_confirm_required` asks a third time; nothing is sent before the last touch; a job running blocks it | |
