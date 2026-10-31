# =============================================================================
# <REPO> - tools/_tracked_names.py
# Copyright (C) 2026 JuanenRac (Electro Hobby 3D) <electrohobby3d@gmail.com>
# GPL-3.0-or-later - see LICENSE
# =============================================================================
# The same copy in every <REPO> repository (tools/check_repo_tools.py of <REPO> compares them).
"""Refuses a repository that versions files whose names come from a Windows path that was not translated.

A Windows path such as C:\\Users\\x handed to a Linux program (WSL) is taken for a relative path whose first folder is called "C:"
(shown as "C" followed by U+F03A when the file system has to store it on Windows): whatever the program writes there ends up
inside the repository, and a careless `git add` versions it. No file of these repositories has a name with a colon, and none
starts with a folder named like a drive or like a user profile.
"""
from __future__ import annotations

import re
import subprocess
from pathlib import Path

_MANGLED = re.compile(r"(^|/)([A-Za-z][:\uf03a])(/|$)|^(Users|Documents and Settings)/", re.IGNORECASE)


def bad_names(names: list[str]) -> list[str]:
    """The names that look like a Windows path or contain a character a path of this project never has."""
    return [name for name in names if ":" in name or "\uf03a" in name or _MANGLED.search(name) or name.startswith("C/")]


def check_tracked_names(root: Path) -> str | None:
    """An error message when a versioned file has such a name, or None. `root` must be a git working tree."""
    result = subprocess.run(
        ("git", "ls-files", "-z"), cwd=root, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, check=False
    )
    if result.returncode != 0:
        return "could not list the versioned files"
    names = [item for item in result.stdout.decode("utf-8", errors="replace").split("\0") if item]
    found = bad_names(names)
    if found:
        shown = ", ".join(repr(name) for name in found[:3])
        return f"versioned files must not have the name of a Windows path ({len(found)}: {shown})"
    return None
