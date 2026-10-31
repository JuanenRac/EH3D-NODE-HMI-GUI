#!/usr/bin/env python3
"""Builds the fonts of the screen (main/fonts/eh3d_font_16.c and eh3d_font_24.c) and core/font_ranges.hpp.

The fonts hold only the characters the screen can draw (see used_chars() in make_locales.py), so they stay small although the screen speaks thirty-seven languages:
Latin, Latin Extended and Cyrillic come from Montserrat (the font of LVGL), Greek and what Montserrat lacks from DejaVu Sans, and Chinese and Japanese from Source Han Sans.
The three fonts are the ones LVGL ships in scripts/built_in_font (they are in the folder managed_components after a first build; the variable EH3D_FONT_DIR can point to any other
folder with the same three files). The symbols of LVGL (arrows, play, pause...) stay in the built-in Montserrat, which each font names as its fallback.
Needs node with lv_font_conv (the variable LV_FONT_CONV names its program; else `npx lv_font_conv`) and the Python package fontTools.

    python -I tools/make_fonts.py          builds the fonts and the file of ranges
    python -I tools/make_fonts.py --size   only prints how many glyphs each font gets and from which font they come
Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
"""
from __future__ import annotations

import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import make_locales  # noqa: E402

ROOT = make_locales.ROOT
SIZES = (16, 24)
FONTS = ["Montserrat-Medium.ttf", "DejaVuSans.ttf", "SourceHanSansSC-Normal.otf"]


def font_dir() -> Path:
    env = os.environ.get("EH3D_FONT_DIR")
    return Path(env) if env else ROOT / "managed_components" / "lvgl__lvgl" / "scripts" / "built_in_font"


def assignment(chars: set[str]) -> list[set[int]]:
    """For each font, the code points it supplies (the first font that has the character)."""
    from fontTools.ttLib import TTFont  # type: ignore[import-not-found]

    maps = [set(TTFont(str(font_dir() / f)).getBestCmap()) for f in FONTS]
    out: list[set[int]] = [set() for _ in FONTS]
    missing = []
    for c in sorted(chars):
        cp = ord(c)
        for i, m in enumerate(maps):
            if cp in m:
                out[i].add(cp)
                break
        else:
            missing.append(c)
    if missing:
        raise SystemExit("no font has: " + "".join(missing))
    return out


def as_ranges(cps: set[int]) -> list[tuple[int, int]]:
    out: list[tuple[int, int]] = []
    for cp in sorted(cps):
        if out and out[-1][1] + 1 == cp:
            out[-1] = (out[-1][0], cp)
        else:
            out.append((cp, cp))
    return out


def converter() -> list[str]:
    env = os.environ.get("LV_FONT_CONV")
    if env:
        return [env]
    npx = shutil.which("npx") or shutil.which("npx.cmd")
    if not npx:
        raise SystemExit("lv_font_conv is not available: install node and run `npm install lv_font_conv`, or set LV_FONT_CONV")
    return [npx, "--yes", "lv_font_conv@1.5.3"]


def build(size: int, parts: list[set[int]]) -> Path:
    out = ROOT / "main" / "fonts" / f"eh3d_font_{size}.c"
    out.parent.mkdir(parents=True, exist_ok=True)
    cmd = converter() + ["--bpp", "4", "--size", str(size), "--format", "lvgl", "--lv-include", "lvgl.h", "--no-compress", "--no-kerning", "--lv-font-name", f"eh3d_font_{size}", "-o", str(out)]
    for font, cps in zip(FONTS, parts):
        if cps:
            cmd += ["--font", str(font_dir() / font), "-r", ",".join(f"0x{a:X}-0x{b:X}" if a != b else f"0x{a:X}" for a, b in as_ranges(cps))]
    subprocess.run(cmd, check=True)
    text = out.read_text(encoding="utf-8")
    fallback = f"lv_font_montserrat_{size}"
    if ".fallback = NULL" not in text:
        raise SystemExit("the generated font has no `.fallback = NULL` to point at the built-in Montserrat")
    text = text.replace(".fallback = NULL", f".fallback = &{fallback}")
    text = re.sub(r"(?m)^ \* Opts: .*$", f" * Built by tools/make_fonts.py (lv_font_conv, 4 bpp, size {size})", text, count=1)
    marker = f"#ifndef EH3D_FONT_{size}\n"
    if marker not in text:
        raise SystemExit("the generated font has no guard to place the declaration of the fallback before")
    text = text.replace(marker, f"/* the symbols of LVGL (arrows, play, pause) stay in the built-in font */\nLV_FONT_DECLARE({fallback})\n\n" + marker, 1)
    out.write_text(text, encoding="utf-8", newline="\n")
    return out


def main() -> int:
    chars = make_locales.used_chars()
    parts = assignment(chars)
    print(f"{len(chars)} characters: " + ", ".join(f"{len(p)} from {f}" for p, f in zip(parts, FONTS)))
    if "--size" in sys.argv:
        return 0
    for size in SIZES:
        path = build(size, parts)
        print(f"{path.relative_to(ROOT)}: {path.stat().st_size // 1024} KiB of source")
    ranges = as_ranges({ord(c) for c in chars})
    lines = ["// EH3D-NODE-HMI-GUI - core/font_ranges.hpp",
             "// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.",
             "// Generated by tools/make_fonts.py: the code points the fonts of the screen (main/fonts) hold. Do not edit by hand.",
             "#pragma once", "", "#include <cstddef>", "", "namespace hmi {", "", "struct CodeRange {", "    unsigned lo;", "    unsigned hi;", "};", "",
             f"inline constexpr CodeRange kFontRanges[{len(ranges)}] = {{"]
    lines += [f"    {{0x{a:X}, 0x{b:X}}}," for a, b in ranges]
    lines += ["};", "", f"inline constexpr std::size_t kFontRangeCount = {len(ranges)};", "", "}  // namespace hmi", ""]
    (ROOT / "core" / "font_ranges.hpp").write_text("\n".join(lines), encoding="utf-8", newline="\n")
    print("wrote core/font_ranges.hpp")
    return 0


if __name__ == "__main__":
    sys.exit(main())
