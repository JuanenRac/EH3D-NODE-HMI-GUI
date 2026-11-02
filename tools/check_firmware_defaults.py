#!/usr/bin/env python3
"""Checks the robustness settings of the firmware without building it: sdkconfig.defaults keeps the task watchdog on (with a reset, not a hang), a main task stack above the default,
the stack overflow canary and the stack data of FreeRTOS, and main/*.cpp creates every task with a literal stack that is not below what tools/task_needs.py says it needs.

    python -I tools/check_firmware_defaults.py        prints FIRMWARE_DEFAULTS=PASS or the problems
Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
"""
from __future__ import annotations

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from task_needs import MAIN_TASK_STACK_MIN, NEEDS, WDT_TIMEOUT_MAX_S  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
MIN_STACK = 4096                       # no task of the screen has less
TASK_CALL = re.compile(r"\bxTaskCreate\w*\s*\(\s*(?:&)?(\w+)\s*,\s*\"([^\"]+)\"\s*,\s*([^,]+),")
REQUIRED_ON = ["CONFIG_ESP_TASK_WDT_EN", "CONFIG_ESP_TASK_WDT_PANIC", "CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU0", "CONFIG_ESP_TASK_WDT_CHECK_IDLE_TASK_CPU1",
               "CONFIG_FREERTOS_CHECK_STACKOVERFLOW_CANARY", "CONFIG_FREERTOS_USE_TRACE_FACILITY"]


def read_defaults(text: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for line in text.splitlines():
        line = line.strip()
        if line and not line.startswith("#") and "=" in line:
            key, value = line.split("=", 1)
            out[key.strip()] = value.strip().strip('"')
    return out


def strip_comments(text: str) -> str:
    return re.sub(r"//[^\n]*", "", re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL))


def problems_of(defaults_text: str, sources: dict[str, str]) -> list[str]:
    bad: list[str] = []
    cfg = read_defaults(defaults_text)
    for key in REQUIRED_ON:
        if cfg.get(key) != "y":
            bad.append(f"sdkconfig.defaults: {key}=y is missing (the screen must reset itself when it hangs and must catch a stack overflow)")
    timeout = cfg.get("CONFIG_ESP_TASK_WDT_TIMEOUT_S", "")
    if not timeout.isdigit() or not 1 <= int(timeout) <= WDT_TIMEOUT_MAX_S:
        bad.append(f"sdkconfig.defaults: CONFIG_ESP_TASK_WDT_TIMEOUT_S must be a number from 1 to {WDT_TIMEOUT_MAX_S}")
    main_stack = cfg.get("CONFIG_ESP_MAIN_TASK_STACK_SIZE", "")
    if not main_stack.isdigit() or int(main_stack) < MAIN_TASK_STACK_MIN:
        bad.append(f"sdkconfig.defaults: CONFIG_ESP_MAIN_TASK_STACK_SIZE must be at least {MAIN_TASK_STACK_MIN} (the default is 3584)")
    tasks = 0
    for name, text in sources.items():
        text = strip_comments(text)
        for m in TASK_CALL.finditer(text):
            tasks += 1
            task, size_text = m.group(2), m.group(3).strip().replace("'", "")
            where = f"{name}: task '{task}'"
            if not size_text.isdigit():
                bad.append(f"{where}: its stack is not a literal number ({size_text!r}), so it cannot be checked")
                continue
            size = int(size_text)
            need, why = NEEDS.get(task, (MIN_STACK, "default minimum"))
            if task not in NEEDS:
                bad.append(f"{where}: not in tools/task_needs.py: say what stack its job needs")
            if size < max(need, MIN_STACK):
                bad.append(f"{where} has a {size}-byte stack, below the {max(need, MIN_STACK)} it needs ({why})")
    if tasks == 0:
        bad.append("no task found in main/*.cpp: the scan is broken")
    for task in NEEDS:
        if not any(re.search(r'"' + re.escape(task) + r'"', s) for s in sources.values()):
            bad.append(f"tools/task_needs.py names task '{task}', which main/*.cpp does not create")
    return bad


def problems(root: Path = ROOT) -> list[str]:
    sources = {p.name: p.read_text(encoding="utf-8", errors="replace") for p in sorted((root / "main").glob("*.cpp"))}
    return problems_of((root / "sdkconfig.defaults").read_text(encoding="utf-8"), sources)


def main() -> int:
    bad = problems()
    for b in bad:
        print("FIRMWARE_DEFAULTS=FAIL " + b)
    if not bad:
        print("FIRMWARE_DEFAULTS=PASS")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
