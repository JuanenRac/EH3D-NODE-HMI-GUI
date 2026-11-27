#!/usr/bin/env python3
# =============================================================================
# EH3D firmware - tools/third_party_notices.py
# Copyright (C) 2026 JuanenRac (Electro Hobby 3D) <electrohobby3d@gmail.com>
# GPL-3.0-or-later - see LICENSE
# =============================================================================
"""The third-party notices of the firmware, made from what the build really holds and checked against the declared dependencies.

    idf.py -B build/<board> ... set-target <chip>                      (one build folder per board; this fetches the components)
    python3 tools/third_party_notices.py build/*/project_description.json   write docs/THIRD_PARTY_NOTICES
    python3 tools/third_party_notices.py --check                        fail when a declared dependency is not in the file

The components are the ones ESP-IDF says are in the build of any board; the licence texts are copied from their folders.
`--check` needs no ESP-IDF: every dependency of dependencies.lock and of main/idf_component.yml must be listed (names,
not versions).
"""
from __future__ import annotations

import argparse
import json
import re
import sys
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUTPUT = "docs/THIRD_PARTY_NOTICES"
MARKER = "Made by tools/third_party_notices.py from the components of the ESP-IDF build; do not edit by hand."
IDF = "idf"
NOTICE_FILE = re.compile(r"^((un)?licen[cs]e|copying|copyright|notice)([-_.].*)?$", re.IGNORECASE)
CODE_SUFFIXES = {".c", ".h", ".cpp", ".hpp", ".py", ".sh", ".json", ".yml", ".yaml", ".cmake", ".png", ".s"}
# fonts whose glyphs are inside the firmware of a screen: the three tools/make_fonts.py builds from, and the symbols of LVGL
FONTS = {
    "Montserrat-Medium.ttf": ("OFL-1.1", "https://raw.githubusercontent.com/JulietaUla/Montserrat/master/OFL.txt"),
    "DejaVuSans.ttf": ("Bitstream Vera and public domain", "https://raw.githubusercontent.com/dejavu-fonts/dejavu-fonts/master/LICENSE"),
    "SourceHanSansSC-Normal.otf": ("OFL-1.1", "https://raw.githubusercontent.com/adobe-fonts/source-han-sans/release/LICENSE.txt"),
}
SYMBOLS = {"FontAwesome": ("OFL-1.1 (fonts) and CC-BY-4.0 (icons)", "https://raw.githubusercontent.com/FortAwesome/Font-Awesome/5.x/LICENSE.txt")}
SKIPPED_FOLDERS = {".git", "test", "tests", "test_apps", "host_test", "examples", "docs", "demos"}


def clean(text: str) -> str:
    return "\n".join(line.rstrip() for line in text.replace("﻿", "").replace("\r\n", "\n").replace("\r", "\n").replace("\f", "").split("\n")).strip("\n")


def declared(root: Path) -> list[str]:
    """The names of dependencies.lock and of main/idf_component.yml, plus ESP-IDF itself."""
    names = {IDF}
    lock = root / "dependencies.lock"
    if lock.is_file():
        section = ""
        for line in lock.read_text(encoding="utf-8").splitlines():
            if line and not line.startswith(" "):
                section = line.split(":", 1)[0]
            elif section == "dependencies":
                found = re.match(r"^  ([A-Za-z0-9_./~-]+):\s*$", line)
                if found:
                    names.add(found[1])
    manifest = root / "main" / "idf_component.yml"
    if manifest.is_file():
        inside = False
        for line in manifest.read_text(encoding="utf-8").splitlines():
            if line.startswith("dependencies:"):
                inside = True
            elif line and not line.startswith((" ", "#")):
                inside = False
            elif inside:
                found = re.match(r"^  ([A-Za-z0-9_./~-]+):", line)
                if found:
                    names.add(found[1])
    return sorted(names) + fonts(root)


def fonts(root: Path) -> list[str]:
    """The fonts tools/make_fonts.py names, and the symbols of LVGL; nothing for a firmware without a screen."""
    tool = root / "tools" / "make_fonts.py"
    if not tool.is_file():
        return []
    found = re.search(r"^FONTS = \[(.*?)\]", tool.read_text(encoding="utf-8"), re.MULTILINE | re.DOTALL)
    if not found:
        raise SystemExit("tools/make_fonts.py: the list FONTS was not found")
    return re.findall(r"[\"']([^\"']+)[\"']", found[1]) + list(SYMBOLS)


def fetch(url: str) -> str:
    with urllib.request.urlopen(url, timeout=60) as answer:  # noqa: S310 - fixed https addresses
        return answer.read().decode("utf-8")


def font_texts(names: list[str]) -> list[tuple[str, str, str]]:
    """(name, licence, text) of every font, from the repository of the font."""
    known = {**FONTS, **SYMBOLS}
    unknown = [name for name in names if name not in known]
    if unknown:
        raise SystemExit(f"no licence is known for the fonts {unknown}: add them to this tool")
    return [(name, known[name][0], clean(fetch(known[name][1]))) for name in names]


def licence_files(folder: Path) -> list[Path]:
    found = []
    for path in folder.rglob("*"):
        if not NOTICE_FILE.match(path.name) or path.suffix.lower() in CODE_SUFFIXES or not path.is_file() or path.stat().st_size > 200_000:
            continue
        if any(part in SKIPPED_FOLDERS for part in path.relative_to(folder).parts[:-1]):
            continue
        found.append(path)
    return sorted(found, key=lambda p: p.as_posix())


def kind(texts: list[str]) -> str:
    whole = " ".join(" ".join(text.split()) for text in texts)
    kinds = [name for name, mark in (("Apache-2.0", "Apache License"), ("MIT", "Permission is hereby granted, free of charge"), ("BSD", "Redistribution and use in source and binary forms")) if mark in whole]
    return " and ".join(kinds) or "see its text below"


def stated(folder: Path, key: str) -> str:
    manifest = folder / "idf_component.yml"
    if not manifest.is_file():
        return ""
    found = re.search(rf"^{key}:\s*[\"']?([^\"'\r\n]+)", manifest.read_text(encoding="utf-8", errors="replace"), re.MULTILINE)
    return found[1].strip() if found else ""


def build_components(descriptions: list[Path]) -> tuple[str, Path, dict[str, Path], set[str]]:
    """(version of ESP-IDF, its folder, managed components by registry name, IDF components) of every board."""
    version, idf_path, managed, own = "", Path(), {}, set()
    for description in descriptions:
        data = json.loads(description.read_text(encoding="utf-8"))
        idf_path = Path(data["idf_path"]).resolve()
        version = str(data.get("git_revision") or "?").lstrip("v")
        for name in data["build_components"]:
            info = data["build_component_info"].get(name)
            if not name or not info:
                continue
            folder = Path(info["dir"]).resolve()
            if folder.parent.name == "managed_components":
                managed[folder.name.replace("__", "/", 1)] = folder
            elif folder.is_relative_to(idf_path / "components"):
                own.add(folder.name)
        project = data.get("project_path")
        if project and (Path(project) / "managed_components").is_dir():
            # what was fetched but is not built for this chip is listed too: it costs a few lines and nothing can be forgotten
            for folder in (Path(project) / "managed_components").iterdir():
                if folder.is_dir():
                    managed.setdefault(folder.name.replace("__", "/", 1), folder.resolve())
    return version, idf_path, managed, own


def render(version: str, idf_path: Path, managed: dict[str, Path], own: set[str], screen_fonts: list[tuple[str, str, str]], adapted: list[tuple[str, str]] = []) -> str:
    rows = []
    used: dict[str, tuple[str, list[str]]] = {}

    def add(label: str, path: Path) -> str:
        text = clean(path.read_text(encoding="utf-8", errors="replace"))
        if text:
            used.setdefault(" ".join(text.split()), (text, []))[1].append(label)
        return text

    root_licence = idf_path / "LICENSE"
    add("ESP-IDF", root_licence)
    rows.append(f"{IDF} | {version} | Apache-2.0")
    for name in sorted(own):
        folder = idf_path / "components" / name
        for path in licence_files(folder):
            add(f"ESP-IDF components/{name}/{path.relative_to(folder).as_posix()}", path)
    for name, folder in sorted(managed.items()):
        texts = [add(f"{name}: {path.relative_to(folder).as_posix()}", path) for path in licence_files(folder)]
        licence = stated(folder, "license") or kind(texts)
        if not any(texts):
            if licence != "Apache-2.0":
                raise SystemExit(f"{name} ships no licence text and states '{licence}': look at it before it is shipped")
            used.setdefault(" ".join(clean(root_licence.read_text(encoding="utf-8")).split()), ("", []))[1].append(f"{name} (Apache License, as stated by the component)")
        rows.append(f"{name} | {stated(folder, 'version') or '?'} | {licence}")
    for name, licence, text in screen_fonts:
        used.setdefault(" ".join(text.split()), (text, []))[1].append(f"font {name}")
        rows.append(f"{name} | font | {licence}")
    for name, text in adapted:
        used.setdefault(" ".join(text.split()), (text, []))[1].append(f"code adapted from {name}")
        rows.append(f"{name} | adapted code | {kind([text])}")
    lines = ["Third-party notices", "===================", "", MARKER, "",
             "The firmware is built with ESP-IDF (Espressif Systems) and the components below. ESP-IDF holds third-party code of",
             "its own (FreeRTOS, lwIP, Mbed TLS, newlib and others); the licence and copyright files of every ESP-IDF component",
             "that is in the build follow, after those of ESP-IDF itself. The program also holds the run-time libraries of the",
             "GCC toolchain (GPL-3.0 with the GCC Runtime Library Exception). Source: https://github.com/espressif/esp-idf and",
             "https://components.espressif.com.", "",
             "Components", "----------", ""]
    lines += rows
    lines += ["", "ESP-IDF components in the build: " + ", ".join(sorted(own))]
    lines += ["", "Copyright notices and licence texts", "-----------------------------------"]
    for text, users in used.values():
        lines += ["", "=" * 78, "Used by: " + ", ".join(dict.fromkeys(users)), "=" * 78, "", text]
    return "\n".join(lines) + "\n"


def problems(written: str, names: list[str]) -> list[str]:
    if MARKER not in written:
        return ["the file was not made by this tool"]
    section = written.split("\nComponents\n", 1)[-1].split("\nCopyright notices and licence texts\n", 1)[0]
    listed = {line.split(" | ")[0].strip() for line in section.splitlines() if line.count(" | ") == 2}
    return [f"not in the notices: {name}" for name in names if name not in listed]


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("descriptions", type=Path, nargs="*", help="project_description.json of the build of each board")
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--check", action="store_true", help="write nothing; fail when a declared dependency is not listed")
    args = parser.parse_args(argv)
    root = args.root.resolve()
    output = root / OUTPUT
    names = declared(root)
    if args.check:
        found = problems(output.read_text(encoding="utf-8"), names) if output.is_file() else [f"{OUTPUT} is missing"]
        for problem in found:
            print(f"- {problem}")
        print(f"THIRD_PARTY_NOTICES={'FAIL' if found else 'PASS'} ({len(names)} declared dependencies)")
        if found:
            print("Make the file again from the build (see tools/third_party_notices.py) and commit it.")
        return 1 if found else 0
    if not args.descriptions:
        parser.error("give the project_description.json of every board")
    adapted = [(path.stem, clean(path.read_text(encoding="utf-8"))) for path in sorted((root / "docs" / "licenses").glob("*.txt"))]
    written = render(*build_components(args.descriptions), font_texts(fonts(root)), adapted)
    missing = problems(written, names)
    if missing:
        raise SystemExit("the build does not hold what is declared: " + "; ".join(missing))
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_bytes(written.encode("utf-8"))
    print(f"wrote {output} ({output.stat().st_size} bytes)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
