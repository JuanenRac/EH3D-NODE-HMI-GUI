#!/usr/bin/env python3
# =============================================================================
# EH3D firmware - tools/gsl3680_table.py
# Copyright (C) 2026 JuanenRac (Electro Hobby 3D) <electrohobby3d@gmail.com>
# GPL-3.0-or-later - see LICENSE
# =============================================================================
"""Makes `gsl3680.fw`, the file the screen loads into its GSL3680 touch controller, from a table YOU provide.

    python tools/gsl3680_table.py path/to/gsl3680_firmware.h -o gsl3680.fw

The table is a C array of `{register, value}` pairs, as the maker of the panel hands it out. It is not part of this
project (it carries no licence): get it from the maker, make the file for your own screen and put it in the root of the
card, or in the settings partition. Do not add either file to a repository or a release.
"""
from __future__ import annotations

import argparse
import re
import struct
import sys
from pathlib import Path

PAGE_REGISTER = 0xF0
DATA_REGISTER = 0x80
MAX_BYTES = 64 * 1024
PAIR = re.compile(r"\{\s*(0[xX][0-9a-fA-F]+|\d+)\s*,\s*(0[xX][0-9a-fA-F]+|\d+)\s*\}")
COMMENT = re.compile(r"/\*.*?\*/|//[^\n]*", re.DOTALL)


def table_bytes(text: str) -> bytes:
    """The records of the file: the register (1 byte) and the value (4 bytes, least significant first)."""
    out = bytearray()
    for number, (register_text, value_text) in enumerate(PAIR.findall(COMMENT.sub(" ", text)), 1):
        register, value = int(register_text, 0), int(value_text, 0)
        if register != PAGE_REGISTER and (register >= DATA_REGISTER or register % 4):
            raise ValueError(f"pair {number}: register {register:#x} is not one of the table of this controller")
        if not 0 <= value <= 0xFFFFFFFF:
            raise ValueError(f"pair {number}: the value does not fit in 32 bits")
        out += struct.pack("<BI", register, value)
    if not out:
        raise ValueError("no {register, value} pairs found")
    if len(out) > MAX_BYTES:
        raise ValueError("the table is bigger than the screen accepts")
    return bytes(out)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("source", type=Path, help="the C header or source that holds the table")
    parser.add_argument("-o", "--output", type=Path, default=Path("gsl3680.fw"))
    args = parser.parse_args(argv)
    try:
        data = table_bytes(args.source.read_text(encoding="utf-8", errors="replace"))
    except (OSError, ValueError) as error:
        print(f"GSL3680_TABLE=FAIL {error}", file=sys.stderr)
        return 2
    args.output.write_bytes(data)
    print(f"GSL3680_TABLE=WRITTEN {args.output} ({len(data) // 5} records)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
