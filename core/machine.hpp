// EH3D-NODE-HMI-GUI - core/machine.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The kind of machine the screen controls. The same screen serves a CNC machine, a laser and a pick and place machine; what changes is what the screen is
// allowed to order. Today that is one thing: a spindle. The screen never fires a laser and never starts the vacuum or the nozzles of a pick and place head
// by itself; those are left to the controller of the machine until a safer way to ask for them is designed and tested.
#pragma once

#include <string>

namespace hmi {

enum class MachineKind { Cnc, Laser, PickAndPlace };

struct Controls {
    /// The screen may ask the machine to turn a spindle.
    bool spindle = false;
};

/// What the screen may order on a machine of this kind.
inline Controls controls_for(MachineKind k) {
    Controls c;
    c.spindle = (k == MachineKind::Cnc);
    return c;
}

inline const char* machine_kind_key(MachineKind k) {
    switch (k) {
        case MachineKind::Cnc: return "cnc";
        case MachineKind::Laser: return "laser";
        case MachineKind::PickAndPlace: return "pnp";
    }
    return "cnc";
}

/// `cnc`, `laser` and `pnp` (or `pick-and-place`), in any case.
inline bool machine_kind_from_text(const std::string& text, MachineKind& out) {
    std::string t;
    for (char c : text) t += (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
    if (t == "cnc") { out = MachineKind::Cnc; return true; }
    if (t == "laser") { out = MachineKind::Laser; return true; }
    if (t == "pnp" || t == "pick-and-place") { out = MachineKind::PickAndPlace; return true; }
    return false;
}

}  // namespace hmi
