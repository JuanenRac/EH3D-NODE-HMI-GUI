#!/usr/bin/env python3
# =============================================================================
# EH3D-NODE-HMI-GUI - tools/render_preview.py
# Copyright (C) 2026 JuanenRac (Electro Hobby 3D) <electrohobby3d@gmail.com>
# GPL-3.0-or-later - see LICENSE
# =============================================================================
"""Draws a preview of the screens of the touch display from the texts of core/ui_text.hpp, in the look of the desktop program (the Forge theme).

    python tools/render_preview.py            # writes images/screenshots/screen-*.svg
    python tools/render_preview.py --png      # also renders them to PNG with Edge (needs Microsoft Edge)

It is a drawing of the layout, NOT a capture of a module: the firmware has not been run on one. The texts are the ones the firmware shows,
read from the header, so a change of a text shows up here.
"""
from __future__ import annotations

import re
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / "images" / "screenshots"
LANGS = ["en", "es", "it", "de", "fr", "pt", "zh", "ja"]
W, H = 1280, 800
FONT = "'Ubuntu','Segoe UI','Noto Sans','Noto Sans CJK SC','Noto Sans CJK JP','Yu Gothic',sans-serif"

BG, CARD, LINE, AMBER, TEXT, DIM = "#14110d", "#181410", "#3a2f1e", "#ffb300", "#f0e6d0", "#a89878"
NAMES = ["Viewer", "Job", "Machine", "Settings", "Fleet", "Start", "Pause", "Stop", "Home", "Confirm", "Cancel", "Armed", "NotArmed",
         "StopPressed", "AckRelease", "Language", "Voice", "Brightness", "Volume", "JogX", "JogY", "JogZ", "Spindle", "AskMove",
         "AskSpindle", "WhyHardwareStop", "WhyNotArmed", "WhyVoiceOff", "WhyNoStop", "WhySpindle", "Feed", "Progress", "Tool", "Jog"]


def texts() -> dict[str, list[str]]:
    src = (ROOT / "core" / "ui_text.hpp").read_text(encoding="utf-8")
    rows = re.findall(r'^\s*\{("(?:[^"\\]|\\.)*"(?:,\s*"(?:[^"\\]|\\.)*"){7})\},?$', src, re.M)
    out = {}
    for name, row in zip(NAMES, rows):
        out[name] = [s.replace('\\"', '"') for s in re.findall(r'"((?:[^"\\]|\\.)*)"', row)]
    if len(out) != len(NAMES):
        raise SystemExit(f"read {len(out)} rows of text, expected {len(NAMES)}")
    return out


def esc(s: str) -> str:
    return s.replace("&", "&amp;").replace("<", "&lt;").replace(">", "&gt;")


def card(x, y, w, h, title=None, stroke=AMBER, cut=16):
    """A card with the two opposite corners cut, as the cards of the desktop program."""
    pts = f"{x + cut},{y} {x + w},{y} {x + w},{y + h - cut} {x + w - cut},{y + h} {x},{y + h} {x},{y + cut}"
    s = f'<polygon points="{pts}" fill="{CARD}" stroke="{stroke}" stroke-width="1.5"/>'
    if title:
        s += f'<text x="{x + 16}" y="{y + 28}" font-size="13" fill="{AMBER}" letter-spacing="1">{esc(title.upper())}</text>'
    return s


def chip(x, y, w, h, label, active=False, size=15):
    fill = "#3a2a00" if active else "none"
    return (f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="9" fill="{fill}" stroke="{AMBER}" stroke-width="1.3"/>'
            f'<text x="{x + w / 2}" y="{y + h / 2 + size / 3}" font-size="{size}" fill="{TEXT}" text-anchor="middle">{esc(label)}</text>')


def cut_button(x, y, w, h, label, fill, color="#ffffff", size=20, dim=False, cut=12):
    pts = f"{x},{y} {x + w - cut},{y} {x + w},{y + cut} {x + w},{y + h} {x + cut},{y + h} {x},{y + h - cut}"
    op = ' opacity="0.35"' if dim else ""
    return (f'<polygon points="{pts}" fill="{fill}"{op}/>'
            f'<text x="{x + w / 2}" y="{y + h / 2 + size / 3}" font-size="{size}" fill="{color}" text-anchor="middle"{op}>{esc(label)}</text>')



GO, HOLD, HALT = "#28e65a", "#2896ff", "#ff2828"


def glyph(cx, cy, r, kind, color="#ffffff"):
    """A symbol of the buttons (the same shapes as the symbols of LVGL that the firmware uses), drawn with lines and polygons."""
    f = f'fill="{color}"'
    k = r * 0.62
    if kind == "play":
        return f'<polygon points="{cx - k * 0.7},{cy - k} {cx + k},{cy} {cx - k * 0.7},{cy + k}" {f}/>'
    if kind == "pause":
        return f'<rect x="{cx - k}" y="{cy - k}" width="{k * 0.8}" height="{k * 2}" {f}/><rect x="{cx + k * 0.2}" y="{cy - k}" width="{k * 0.8}" height="{k * 2}" {f}/>'
    if kind == "stop":
        return f'<rect x="{cx - k}" y="{cy - k}" width="{k * 2}" height="{k * 2}" {f}/>'
    if kind in ("left", "right", "up", "down"):
        pts = {"left": [(-k, 0), (k * 0.5, -k), (k * 0.5, k)], "right": [(k, 0), (-k * 0.5, -k), (-k * 0.5, k)],
               "up": [(0, -k), (-k, k * 0.5), (k, k * 0.5)], "down": [(0, k), (-k, -k * 0.5), (k, -k * 0.5)]}[kind]
        return f'<polygon points="{" ".join(f"{cx + a},{cy + b}" for a, b in pts)}" {f}/>'
    if kind == "home":
        return (f'<polygon points="{cx - k * 1.2},{cy} {cx},{cy - k * 1.1} {cx + k * 1.2},{cy}" {f}/>'
                f'<rect x="{cx - k * 0.8}" y="{cy}" width="{k * 1.6}" height="{k * 1.0}" {f}/>')
    if kind == "spindle":
        return (f'<path d="M {cx - k},{cy} A {k},{k} 0 1 1 {cx + k * 0.7},{cy + k * 0.7}" fill="none" stroke="{color}" stroke-width="{r * 0.16}"/>'
                f'<polygon points="{cx + k * 0.2},{cy + k * 1.2} {cx + k * 1.3},{cy + k * 0.8} {cx + k * 0.8},{cy - k * 0.1}" {f}/>')
    if kind == "ok":
        return f'<polyline points="{cx - k},{cy} {cx - k * 0.3},{cy + k * 0.7} {cx + k},{cy - k * 0.7}" fill="none" stroke="{color}" stroke-width="{r * 0.22}" stroke-linecap="round"/>'
    if kind == "close":
        return (f'<line x1="{cx - k}" y1="{cy - k}" x2="{cx + k}" y2="{cy + k}" stroke="{color}" stroke-width="{r * 0.22}" stroke-linecap="round"/>'
                f'<line x1="{cx + k}" y1="{cy - k}" x2="{cx - k}" y2="{cy + k}" stroke="{color}" stroke-width="{r * 0.22}" stroke-linecap="round"/>')
    if kind == "upload":
        return (f'<polygon points="{cx},{cy - k} {cx - k},{cy} {cx + k},{cy}" {f}/><rect x="{cx - k * 0.35}" y="{cy}" width="{k * 0.7}" height="{k}" {f}/>'
                f'<rect x="{cx - k}" y="{cy + k * 1.15}" width="{k * 2}" height="{k * 0.25}" {f}/>')
    if kind == "warning":
        return (f'<polygon points="{cx},{cy - k * 1.1} {cx - k * 1.2},{cy + k} {cx + k * 1.2},{cy + k}" {f}/>'
                f'<rect x="{cx - k * 0.1}" y="{cy - k * 0.4}" width="{k * 0.2}" height="{k * 0.7}" fill="#c62828"/><circle cx="{cx}" cy="{cy + k * 0.62}" r="{k * 0.13}" fill="#c62828"/>')
    if kind == "sd":
        return (f'<polygon points="{cx - k * 0.8},{cy - k} {cx + k * 0.2},{cy - k} {cx + k * 0.8},{cy - k * 0.4} {cx + k * 0.8},{cy + k} {cx - k * 0.8},{cy + k}" {f}/>')
    return ""


def icon_button(x, y, w, h, kind, fill, axis="", dim=False, cut=12):
    """A button with a symbol (and the letter of an axis) in place of a word."""
    pts = f"{x},{y} {x + w - cut},{y} {x + w},{y + cut} {x + w},{y + h} {x + cut},{y + h} {x},{y + h - cut}"
    op = ' opacity="0.35"' if dim else ""
    r = min(w, h) * 0.34
    cx, cy = x + w / 2, y + h / 2
    inner = ""
    if axis:
        inner += f'<text x="{cx - r * 0.55}" y="{cy + r * 0.45}" font-size="{r * 1.3}" fill="#ffffff" text-anchor="middle">{axis}</text>'
        inner += glyph(cx + r * 0.75, cy, r * 0.8, kind)
    else:
        inner += glyph(cx, cy, r, kind)
    return f'<polygon points="{pts}" fill="{fill}"{op}/><g{op}>{inner}</g>'



def board_2d(x, y, w, h):
    """The plan view: a grid, a table outline, and the boards with their holes."""
    g = [f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="#100d09" stroke="{LINE}"/>']
    for i in range(1, 12):
        g.append(f'<line x1="{x + i * w / 12}" y1="{y}" x2="{x + i * w / 12}" y2="{y + h}" stroke="#241c10"/>')
    for i in range(1, 8):
        g.append(f'<line x1="{x}" y1="{y + i * h / 8}" x2="{x + w}" y2="{y + i * h / 8}" stroke="#241c10"/>')
    bw, bh = (w - 70) / 4, (h - 60) / 3
    for r in range(3):
        for c in range(4):
            bx, by = x + 22 + c * (bw + 8), y + 18 + r * (bh + 8)
            g.append(f'<rect x="{bx:.1f}" y="{by:.1f}" width="{bw:.1f}" height="{bh:.1f}" fill="none" stroke="#ff5a1f" stroke-dasharray="5 3" stroke-width="1.5"/>')
            for k in range(5):
                g.append(f'<circle cx="{bx + 12 + k * 11:.1f}" cy="{by + 12:.1f}" r="2.6" fill="none" stroke="#9be564"/>')
            for k in range(3):
                g.append(f'<rect x="{bx + 14 + k * 20:.1f}" y="{by + bh / 2 - 6:.1f}" width="8" height="26" rx="3" fill="none" stroke="#9be564"/>')
    g.append(f'<circle cx="{x + 40}" cy="{y + h - 58}" r="12" fill="none" stroke="#9be564"/><line x1="{x + 22}" y1="{y + h - 58}" x2="{x + 58}" y2="{y + h - 58}" stroke="#9be564"/>'
             f'<line x1="{x + 40}" y1="{y + h - 76}" x2="{x + 40}" y2="{y + h - 40}" stroke="#9be564"/>')
    return "".join(g)


def screen(lang: str, state: str, portrait: bool = False, page: str = "machine") -> str:
    """One screen: `state` is idle, confirm or stop; `page` is machine, card or fleet; `portrait` stands the screen up (800 x 1280)."""
    t = {k: v[LANGS.index(lang)] for k, v in texts().items()}
    stopped = state == "stop"
    w, h = (800, 1280) if portrait else (1280, 800)
    g = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{w}" height="{h}" viewBox="0 0 {w} {h}" font-family="{FONT}">',
         f'<rect width="{w}" height="{h}" fill="{BG}"/>']
    # the top bar: the title, the five windows as chips, and the state of the link
    g.append(f'<text x="30" y="46" font-size="24" fill="{AMBER}">&lt;</text>')
    g.append(f'<text x="64" y="46" font-size="28" fill="{TEXT}">EH3D</text><text x="142" y="46" font-size="28" fill="{AMBER}">BUNGARD</text>')
    tabs = [(t["Machine"], "machine", 128), (t["Job"], "card", 110), (t["Viewer"], "viewer", 110), (t["Fleet"], "fleet", 230), (t["Settings"], "set", 128)]
    if portrait:
        x, y = 20, 70
        for label, key, tw in tabs[:3]:
            g.append(chip(x, y, 150, 40, label, key == page, 14))
            x += 160
        x = 20
        for label, key, tw in tabs[3:]:
            g.append(chip(x, y + 50, 230, 40, label, key == page, 14))
            x += 240
        top_end = 176
    else:
        x = 330
        for label, key, tw in tabs:
            g.append(chip(x, 16, tw, 38, label, key == page))
            x += tw + 12
        top_end = 66
    g.append(f'<text x="{w - 28}" y="32" font-size="13" fill="{TEXT}" text-anchor="end">{esc(t["NotArmed"] if stopped else t["Armed"])}   server</text>')
    g.append(f'<text x="{w - 28}" y="52" font-size="13" fill="{DIM}" text-anchor="end">{lang.upper()}</text>')
    g.append(f'<line x1="0" y1="{top_end}" x2="{w}" y2="{top_end}" stroke="{LINE}"/>')

    # the middle: the three cards of the machine, or the files of the card, or the fleet
    bar_h = 216 if not portrait else 250
    bar_y = h - bar_h - 18
    mid_y, mid_h = top_end + 14, bar_y - top_end - 28
    tools = [("D T06 0.60 mm", "96", AMBER, True), ("D T08 0.80 mm", "480", "#9be564", False), ("D T10 1.00 mm", "120", "#cfd8dc", False),
             ("D T32 3.20 mm", "48", "#ff5a1f", False), ("R T01 Routing 1", "", AMBER, False), ("R T02 Routing 2", "", "#9be564", False)]
    if page == "machine":
        if portrait:
            cw = w - 36
            g.append(card(18, mid_y, cw, 200, t["Job"]))
            g.append(f'<text x="34" y="{mid_y + 62}" font-size="18" fill="{TEXT}">05_SMALL_BOARD.NCD</text>')
            for i, (name, holes, dot, sel) in enumerate(tools[:4]):
                cx, cy = 34 + (i % 2) * 380, mid_y + 78 + (i // 2) * 56
                g.append(f'<rect x="{cx}" y="{cy}" width="350" height="46" rx="5" fill="{"#3a2a00" if sel else "#1d1812"}" stroke="{AMBER if sel else "none"}"/>')
                g.append(f'<circle cx="{cx + 16}" cy="{cy + 23}" r="6" fill="{dot}"/><text x="{cx + 34}" y="{cy + 21}" font-size="14" fill="{TEXT}">{esc(name)}</text>')
                g.append(f'<text x="{cx + 34}" y="{cy + 38}" font-size="11" fill="{DIM}">{holes} holes</text>')
            g.append(card(18, mid_y + 214, cw, 370, "2D"))
            g.append(board_2d(34, mid_y + 246, cw - 32, 322))
            jy = mid_y + 598
            g.append(card(18, jy, cw, bar_y - jy - 14, t["Jog"]))
            jog = [("X", "left", 0, 0), ("X", "right", 1, 0), ("Y", "down", 2, 0), ("Y", "up", 3, 0),
                   ("Z", "down", 0, 1), ("Z", "up", 1, 1), ("", "home", 2, 1), ("", "spindle", 3, 1)]
            for axis, kind, cx, cy in jog:
                g.append(icon_button(34 + cx * 190, jy + 42 + cy * 76, 178, 64, kind, "#37474f", axis, dim=stopped))
        else:
            g.append(card(18, 80, 240, 470, t["Job"]))
            g.append(f'<text x="34" y="128" font-size="20" fill="{TEXT}">05_SMALL_BOARD.NCD</text>')
            g.append(f'<text x="34" y="152" font-size="13" fill="{AMBER}">Drilling + Routing</text>')
            g.append(f'<text x="34" y="172" font-size="12" fill="{DIM}">Generic FR-4 - 1.60 mm</text>')
            for i, (name, holes, dot, sel) in enumerate(tools):
                y = 188 + i * 52
                g.append(f'<rect x="30" y="{y}" width="216" height="44" rx="5" fill="{"#3a2a00" if sel else "#1d1812"}" stroke="{AMBER if sel else "none"}"/>')
                g.append(f'<circle cx="46" cy="{y + 22}" r="6" fill="{dot}"/><text x="64" y="{y + 20}" font-size="15" fill="{TEXT}">{esc(name)}</text>')
                if holes:
                    g.append(f'<text x="64" y="{y + 36}" font-size="11" fill="{DIM}">{holes} holes</text>')
            g.append(card(272, 80, 560, 470, "2D"))
            g.append(board_2d(290, 112, 524, 390))
            g.append(f'<text x="290" y="532" font-size="11" fill="{DIM}">495 x 325 mm</text>')
            g.append(card(846, 80, 416, 470, t["Jog"]))
            jog = [("X", "left", 0, 0), ("Y", "up", 1, 0), ("X", "right", 2, 0), ("Z", "up", 3, 0),
                   ("Y", "down", 1, 1), ("Z", "down", 3, 1)]
            for axis, kind, cx, cy in jog:
                g.append(icon_button(862 + cx * 96, 120 + cy * 84, 88, 72, kind, "#37474f", axis, dim=stopped))
            g.append(icon_button(862, 300, 184, 72, "home", "#37474f", dim=stopped))
            g.append(icon_button(1058, 300, 184, 72, "spindle", "#37474f", dim=stopped))
            for i, (lab, val) in enumerate([(t["Feed"], "2400"), (t["Spindle"], "60000"), (t["Tool"], "T06")]):
                g.append(f'<text x="{862 + i * 148}" y="410" font-size="13" fill="{DIM}">{esc(lab.upper())}</text><text x="{862 + i * 148}" y="440" font-size="26" fill="{TEXT}">{val}</text>')
    elif page == "card":
        files = [("05_small_board.ncd", "12.4 KB", True), ("05_board_outline.plt", "3.1 KB", False), ("power-supply.ncd", "8.7 KB", False),
                 ("sensor-top.gbr", "41.0 KB", False), ("frame.dxf", "19.2 KB", False), ("panel-4x3.zip", "2.3 MB", False), ("hmi.cfg", "1.1 KB", False)]
        lw = w - 36 if portrait else 780
        g.append(card(18, mid_y, lw, mid_h, "microSD"))
        for i, (name, size, sel) in enumerate(files):
            y = mid_y + 48 + i * 62
            if y + 54 > mid_y + mid_h:
                break
            g.append(f'<rect x="32" y="{y}" width="{lw - 28}" height="54" rx="6" fill="{"#3a2a00" if sel else "#1d1812"}" stroke="{AMBER if sel else "none"}"/>')
            g.append(f'<text x="52" y="{y + 34}" font-size="20" fill="{TEXT}">{esc(name)}</text><text x="{lw - 10}" y="{y + 34}" font-size="16" fill="{DIM}" text-anchor="end">{size}</text>')
        if not portrait:
            g.append(card(814, mid_y, 448, mid_h, t["Job"]))
            g.append(f'<text x="834" y="{mid_y + 70}" font-size="20" fill="{TEXT}">05_small_board.ncd</text>')
            g.append(f'<text x="834" y="{mid_y + 98}" font-size="14" fill="{DIM}">machine: m1   station: bench</text>')
            g.append(icon_button(834, mid_y + 130, 200, 70, "sd", "#37474f"))
            g.append(icon_button(1046, mid_y + 130, 200, 70, "upload", GO, dim=stopped))
            g.append(f'<text x="834" y="{mid_y + 250}" font-size="13" fill="{AMBER}">ready: j42  (05_small_board-job.txt)</text>')
    elif page == "fleet":
        rows = [("BENCH", None), ("Bungard CCD 1", ("busy", "power-board.ncd", 59)), ("Bungard CCD 2", ("waiting", "sensor.ncd", 82)), ("Bungard CCD 3", ("idle", "", -1)),
                ("ROOM", None), ("Router 1", ("busy", "frame.plt", 34)), ("Router 2", ("idle", "", -1))]
        g.append(card(18, mid_y, w - 36, mid_h, t["Fleet"]))
        y = mid_y + 50
        for name, info in rows:
            if info is None:
                g.append(f'<text x="40" y="{y + 22}" font-size="14" fill="{AMBER}" letter-spacing="1">{esc(name)}</text>')
                y += 34
                continue
            state_, job, pct = info
            col = {"busy": AMBER, "waiting": "#ff9100", "idle": "#9be564"}[state_]
            g.append(f'<circle cx="52" cy="{y + 24}" r="8" fill="{col}"/><text x="76" y="{y + 32}" font-size="22" fill="{TEXT}">{esc(name)}</text>')
            sx = 360 if not portrait else 330
            g.append(f'<text x="{sx}" y="{y + 32}" font-size="18" fill="{TEXT}">{state_}</text>')
            if pct >= 0:
                bx = sx + 150
                g.append(f'<text x="{bx}" y="{y + 22}" font-size="14" fill="{TEXT}">{esc(job)}</text>')
                bw = 260 if not portrait else 130
                g.append(f'<rect x="{bx}" y="{y + 30}" width="{bw}" height="8" rx="4" fill="#241c10"/><rect x="{bx}" y="{y + 30}" width="{bw * pct / 100:.0f}" height="8" rx="4" fill="{col}"/>')
                g.append(f'<text x="{bx + bw + 12}" y="{y + 40}" font-size="15" fill="{TEXT}">{pct}%</text>')
            y += 66
    # the bar of the position
    g.append(card(18, bar_y, w - 36, bar_h, None))
    px = 44
    for lab, val, step in (("X", "19.5 mm", 150), ("Y", "77.5 mm", 150), ("Z", "-0.4 mm", 150), (t["Machine"], "CCD", 120)):
        g.append(f'<text x="{px}" y="{bar_y + 40}" font-size="12" fill="{DIM}">{esc(lab.upper())}</text><text x="{px}" y="{bar_y + 74}" font-size="28" fill="{TEXT}">{val}</text>')
        px += step if not portrait else 175
    pw = 560 if not portrait else w - 150
    g.append(f'<text x="44" y="{bar_y + 112}" font-size="12" fill="{DIM}">{esc(t["Progress"].upper())}</text>')
    g.append(f'<rect x="44" y="{bar_y + 124}" width="{pw}" height="14" rx="7" fill="#241c10"/><rect x="44" y="{bar_y + 124}" width="{0 if stopped else pw * 0.37:.0f}" height="14" rx="7" fill="{AMBER}"/>')
    g.append(f'<text x="{44 + pw + 14}" y="{bar_y + 138}" font-size="16" fill="{TEXT}">{0 if stopped else 37} %</text>')
    if portrait:
        sy = bar_y + 168
        g.append(f'<circle cx="86" cy="{sy + 34}" r="44" fill="#1d1812" stroke="{AMBER}" stroke-width="5"/><circle cx="86" cy="{sy + 34}" r="34" fill="#c62828"/>'
                 + glyph(86, sy + 34, 34, "warning"))
        g.append(icon_button(160, sy, 190, 68, "pause", HOLD, dim=stopped))
        g.append(icon_button(366, sy, 190, 68, "play", GO, dim=stopped))
        g.append(icon_button(572, sy, 190, 68, "stop", HALT))
    else:
        g.append(f'<circle cx="790" cy="{bar_y + 84}" r="52" fill="#1d1812" stroke="{AMBER}" stroke-width="5"/><circle cx="790" cy="{bar_y + 84}" r="40" fill="#c62828"/>'
                 + glyph(790, bar_y + 84, 40, "warning"))
        g.append(icon_button(870, bar_y + 52, 124, 64, "pause", HOLD, dim=stopped))
        g.append(icon_button(1006, bar_y + 52, 124, 64, "play", GO, dim=stopped))
        g.append(icon_button(1142, bar_y + 52, 104, 64, "stop", HALT))
    if stopped:
        bx, by, bw2 = (870, bar_y + 134, 376) if not portrait else (44, bar_y + 18 + 150, w - 88)
        g.append(f'<rect x="{bx}" y="{by}" width="{bw2}" height="62" rx="9" fill="#2a0e0e" stroke="#ff5252"/>')
        g.append(f'<text x="{bx + 18}" y="{by + 26}" font-size="14" fill="#ff5252">{esc(t["StopPressed"])}</text>')
        g.append(f'<text x="{bx + 18}" y="{by + 48}" font-size="12" fill="{DIM}">{esc(t["AckRelease"])}</text>')
        g.append(f'<text x="44" y="{bar_y - 8}" font-size="13" fill="{AMBER}">{esc(t["WhyHardwareStop"])}</text>')
    if state == "confirm":
        g.append(f'<rect width="{w}" height="{h}" fill="#000" opacity="0.6"/>')
        cx0, cy0 = (w - 520) // 2, (h - 290) // 2
        g.append(card(cx0, cy0, 520, 290, None, cut=20))
        g.append(f'<text x="{w / 2}" y="{cy0 + 66}" font-size="26" fill="{TEXT}" text-anchor="middle">{esc(t["AskMove"])}</text>')
        g.append(f'<text x="{w / 2}" y="{cy0 + 108}" font-size="20" fill="{AMBER}" text-anchor="middle">{esc(t["JogX"])} +</text>')
        g.append(icon_button(cx0 + 32, cy0 + 160, 210, 78, "ok", GO))
        g.append(icon_button(cx0 + 278, cy0 + 160, 210, 78, "close", "#546e7a"))
    g.append("</svg>")
    return "\n".join(g)


SHOTS = [("screen-machine", "en", "idle", False, "machine"), ("screen-portrait", "de", "idle", True, "machine"), ("screen-card", "it", "idle", False, "card"),
         ("screen-fleet", "pt", "idle", False, "fleet"), ("screen-confirm", "es", "confirm", False, "machine"), ("screen-stop", "ja", "stop", False, "machine")]


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    for name, lang, state, portrait, page in SHOTS:
        (OUT / f"{name}.svg").write_text(screen(lang, state, portrait, page), encoding="utf-8", newline="\n")
        print("written", name)
    if "--png" in sys.argv:
        edge = next((p for p in (r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe", r"C:\Program Files\Microsoft\Edge\Application\msedge.exe") if Path(p).exists()), None)
        if not edge:
            print("Microsoft Edge not found")
            return 1
        with tempfile.TemporaryDirectory() as tmp:
            for name, _, _, portrait, _ in SHOTS:
                size = "800,1280" if portrait else "1280,800"
                subprocess.run([edge, "--headless=new", f"--user-data-dir={tmp}", "--hide-scrollbars", f"--window-size={size}",
                                f"--screenshot={OUT / (name + '.png')}", (OUT / f"{name}.svg").as_uri()], capture_output=True, check=False)
                print("rendered", name)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
