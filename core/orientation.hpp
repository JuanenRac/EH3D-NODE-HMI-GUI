// EH3D-NODE-HMI-GUI - core/orientation.hpp
// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
// The screen works standing up (800 x 1280) or lying down (1280 x 800). The setting `orientation` says which one, or `auto` to follow the
// way the module is mounted as told by the application (a tilt sensor, a switch, the last choice). The layout of the screens is a function of
// the result: lying down the job, the plan view and the jog pad sit side by side above the bar of the position; standing up they stack.
#pragma once

#include <cstdint>
#include <string>

namespace hmi {

enum class Orientation { Auto, Landscape, Portrait };

inline bool orientation_from_text(const std::string& v, Orientation& out) {
    if (v == "auto") { out = Orientation::Auto; return true; }
    if (v == "landscape") { out = Orientation::Landscape; return true; }
    if (v == "portrait") { out = Orientation::Portrait; return true; }
    return false;
}

inline const char* orientation_text(Orientation o) {
    switch (o) {
        case Orientation::Landscape: return "landscape";
        case Orientation::Portrait: return "portrait";
        default: return "auto";
    }
}

/// The native size of the panel of the module: it is a portrait panel.
constexpr int kPanelWidth = 800;
constexpr int kPanelHeight = 1280;

struct Layout {
    bool portrait = false;
    int width = 1280;
    int height = 800;
    /// How many degrees the picture is turned to reach this layout from the native panel (0 or 90).
    int rotation = 90;
    /// The screen is divided in columns (lying down: three, standing up: one) and the bar of the position sits under them.
    int columns = 3;
    int bar_height = 216;
    int top_height = 66;
};

/// The layout for a setting and for the way the module is mounted (`mounted_portrait`, only read when the setting is `auto`).
inline Layout layout_for(Orientation setting, bool mounted_portrait) {
    const bool portrait = setting == Orientation::Portrait || (setting == Orientation::Auto && mounted_portrait);
    Layout l;
    l.portrait = portrait;
    if (portrait) {
        l.width = kPanelWidth;
        l.height = kPanelHeight;
        l.rotation = 0;
        l.columns = 1;
        l.bar_height = 260;
    } else {
        l.width = kPanelHeight;
        l.height = kPanelWidth;
        l.rotation = 90;
        l.columns = 3;
        l.bar_height = 216;
    }
    return l;
}

/// The width of a column of the screen, with `gap` pixels between the columns and at the sides.
inline int column_width(const Layout& l, int gap) { return (l.width - gap * (l.columns + 1)) / l.columns; }

/// Where a touch of the panel (native coordinates) falls on the screen of the layout, so a rotated screen answers where it is touched.
inline void touch_to_screen(const Layout& l, int panel_x, int panel_y, int& x, int& y) {
    if (l.rotation == 0) {
        x = panel_x;
        y = panel_y;
    } else {
        // turned 90 degrees clockwise: the left edge of the panel is the top of the screen
        x = panel_y;
        y = kPanelWidth - 1 - panel_x;
    }
}

}  // namespace hmi
