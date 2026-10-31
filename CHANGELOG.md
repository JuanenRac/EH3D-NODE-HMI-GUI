# Changelog

All notable changes to this project are documented here.

## [Unreleased]

## [0.0.1] - A touch screen for CNC machines, lasers and pick and place machines, on the hardware of the Bungard screen

- **The same screen for three kinds of machine:** the firmware and the rules of EH3D-NODE-BUNGARD-HMI (the 10.1-inch ESP32-P4 touch screen, the settings file, the gate with the confirmation and the hardware-stop latch, the spoken alerts and orders, the keyboards, the server link and the screens in eight languages) as a general purpose controller; a copy of that base that goes its own way from here.
- **The kind of machine:** `machine_kind` in the settings file (`cnc`, `laser` or `pnp`, `cnc` by default; a wrong value is reported and the default stays) decides what the screen may order. Only a CNC machine gets the spindle button; the gate refuses a spindle order on a laser or a pick and place machine whatever the other settings say, with a sentence in each of the eight languages.
- **Tests:** a fifth host test program for the kinds of machine (52 checks); the five programs make 1018 checks.
- **Built and run:** the rules are tested on a computer; the firmware has not been flashed to a module and nothing has been connected to a machine.
