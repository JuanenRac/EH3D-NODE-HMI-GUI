# Security policy

EH3D-NODE-HMI-GUI is firmware for a touch screen that is meant to control a machine: it can ask for a move and for the spindle to turn, and it listens to the operator's voice. It is not a safety system. The stop that protects people and the machine is a hardware button that cuts the drive by itself; the screen only reads it.

What the firmware does, and what is tested on a computer:

* Every move and spindle order passes the gate: a confirmation on the screen with a time limit, no move while the hardware stop is pressed or not acknowledged, none while the link is not armed, none while the stop input is not wired (unless the bench setting says so). A stop always goes.
* A spoken order is taken only from a short fixed list for each language, always asks for a confirmation, and is off until the operator turns it on. A spoken yes confirms only when the setting `voice_confirm` is on.
* A value out of range in `hmi.cfg` or an unknown key is reported and the safe default stays. The access token is a setting that is never shown or logged, and nothing secret is compiled in.

What it does **not** do: the link to the server or the machine does not exist yet, the real microphone path (noise, accents, echo) has not been tried, the settings file can be changed by anyone who holds the module, and it has had no independent review. Nothing has run on a module or on a machine yet.

## Reporting a problem

Please report a vulnerability privately to **electrohobby3d@gmail.com**, with the steps that show it. Do not open a public issue for it. You will get an answer as soon as possible; this is a one-person project.
