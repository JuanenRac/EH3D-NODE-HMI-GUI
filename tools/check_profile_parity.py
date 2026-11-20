#!/usr/bin/env python3
# Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
"""Checks that the Bungard touch screen is this screen plus a profile, and says what is still different.

EH3D-NODE-HMI-GUI is the generic screen. EH3D-NODE-BUNGARD-HMI is the same screen for one machine: it must differ only by a settings profile (`profiles/bungard.cfg`) and by the parts of
the screen that are still written only there. This tool compares the two repositories when they sit side by side (it is skipped when the other one is not next to this one):

* the files the two share (the core, the tests of the core, the sockets of the link, the board, the texts) must be the same text once the names of the two projects are made equal;
  `tools/profile_parity_baseline.json` lists the ones that are known to differ (`different`) and the ones that exist in one repository only (`only_generic`, `only_bungard`), so a
  new difference fails and a difference that was closed has to be taken out of the list;
* the settings file of the Bungard screen (`hmi.cfg.example`) must be the one of this screen with `profiles/bungard.cfg` added, and every key of it must be one that this screen reads.

    py -3 -I tools/check_profile_parity.py            report and fail on a difference that is not in the baseline
    py -3 -I tools/check_profile_parity.py --list     print every difference, the known ones too
"""
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OTHER = ROOT.parent / "EH3D-NODE-BUNGARD-HMI"
BASELINE = ROOT / "tools" / "profile_parity_baseline.json"
PROFILE = ROOT / "profiles" / "bungard.cfg"
NAMES = [("EH3D-NODE-BUNGARD-HMI", "@NAME@"), ("EH3D-NODE-HMI-GUI", "@NAME@"), ("eh3d-node-bungard-hmi", "@name@"), ("eh3d-node-hmi-gui", "@name@"),
         ("eh3d_node_bungard_hmi", "@name_@"), ("eh3d_node_hmi_gui", "@name_@"), ("bungard-hmi", "@short@"), ("hmi-gui", "@short@")]
# What is compared: the files that make the two screens one (the screens themselves, app_main.cpp and ui.cpp, are compared by hand: see docs/PROFILES.md).
SHARED = [re.compile(p) for p in (r"^core/.*", r"^tests/test_.*\.cpp$", r"^main/(server_link\.(cpp|hpp)|board.*|image_confirm\.hpp)$", r"^sdkconfig.*", r"^locales/.*\.tsv$", r"^tools/(make_locales|make_fonts)\.py$")]


def normal(text: str) -> str:
    text = text.replace("\r\n", "\n")
    for old, new in NAMES:
        text = text.replace(old, new)
    return text


def tracked(repo: Path) -> dict:
    out = {}
    # only the folders that can hold a shared file are walked (a build folder next to them can be huge)
    candidates = [p for folder in ("core", "tests", "main", "locales", "tools") if (repo / folder).is_dir() for p in (repo / folder).rglob("*")] + list(repo.glob("sdkconfig*"))
    for path in sorted(candidates):
        rel = path.relative_to(repo).as_posix()
        if path.is_file() and "__pycache__" not in rel and any(p.match(rel) for p in SHARED):
            out[rel] = path
    return out


def compare() -> dict:
    mine, theirs = tracked(ROOT), tracked(OTHER)
    found = {"different": [], "only_generic": [], "only_bungard": []}
    for rel in sorted(set(mine) | set(theirs)):
        if rel not in theirs:
            found["only_generic"].append(rel)
        elif rel not in mine:
            found["only_bungard"].append(rel)
        elif normal(mine[rel].read_text(encoding="utf-8", errors="replace")) != normal(theirs[rel].read_text(encoding="utf-8", errors="replace")):
            found["different"].append(rel)
    return found


def cfg_values(text: str) -> dict:
    out = {}
    for line in text.replace("\r\n", "\n").split("\n"):
        line = line.strip()
        if line and not line.startswith("#") and "=" in line:
            key, _, value = line.partition("=")
            out[key.strip()] = value.strip()
    return out


def read_keys(root: Path) -> set:
    """The keys that core/hmi_config.hpp of a repository reads."""
    return set(re.findall(r'key == "([a-z_0-9]+)"', (root / "core" / "hmi_config.hpp").read_text(encoding="utf-8")))


def profile_problems() -> list:
    bad = []
    base = json.loads(BASELINE.read_text(encoding="utf-8")) if BASELINE.is_file() else {}
    waiting = set(base.get("settings_not_yet_in_bungard", []))
    generic = cfg_values((ROOT / "hmi.cfg.example").read_text(encoding="utf-8"))
    bungard = cfg_values((OTHER / "hmi.cfg.example").read_text(encoding="utf-8"))
    profile = cfg_values(PROFILE.read_text(encoding="utf-8")) if PROFILE.is_file() else {}
    if not PROFILE.is_file():
        bad.append("profiles/bungard.cfg is missing")
    merged = dict(generic)
    merged.update(profile)
    # the Bungard screen is always a CNC machine: it has no machine_kind line
    if merged.get("machine_kind") == "cnc" and "machine_kind" not in bungard:
        merged.pop("machine_kind")
    for key in sorted(waiting - set(merged)):
        bad.append(f"settings_not_yet_in_bungard lists {key}, which the generic screen does not have (take it out of tools/profile_parity_baseline.json)")
    for key in sorted(waiting & set(bungard)):
        bad.append(f"{key} is in the Bungard screen now: take it out of settings_not_yet_in_bungard in tools/profile_parity_baseline.json")
    merged = {k: v for k, v in merged.items() if k not in waiting}
    if merged != bungard:
        for key in sorted(set(merged) | set(bungard)):
            if merged.get(key) != bungard.get(key):
                bad.append(f"the settings differ at {key}: generic + profile = {merged.get(key)!r}, Bungard screen = {bungard.get(key)!r}")
    unknown = sorted(set(profile) - read_keys(ROOT))
    if unknown:
        bad.append("profiles/bungard.cfg has keys that core/hmi_config.hpp does not read: " + ", ".join(unknown))
    return bad


def problems() -> tuple:
    """(new differences, baseline lines that no longer hold, differences of the settings)."""
    base = json.loads(BASELINE.read_text(encoding="utf-8")) if BASELINE.is_file() else {}
    found = compare()
    new, stale = [], []
    for kind, files in found.items():
        known = set(base.get(kind, []))
        new += [f"{kind}: {f}" for f in files if f not in known]
        stale += [f"{kind}: {f}" for f in sorted(known - set(files))]
    return new, stale, profile_problems()


def main() -> int:
    if not OTHER.is_dir():
        print("PROFILE_PARITY=SKIP (EH3D-NODE-BUNGARD-HMI is not next to this repository)")
        return 0
    if "--list" in sys.argv:
        for kind, files in compare().items():
            for f in files:
                print(f"{kind}: {f}")
    new, stale, cfg = problems()
    for line in new:
        print("  NEW   ", line)
    for line in stale:
        print("  STALE ", line, "(closed: take it out of tools/profile_parity_baseline.json)")
    for line in cfg:
        print("  CFG   ", line)
    if new or stale or cfg:
        print("PROFILE_PARITY=FAIL")
        return 1
    print("PROFILE_PARITY=PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
