#!/usr/bin/env python3
# GPL-3.0-or-later - see LICENSE
"""Tests of tools/gsl3680_table.py with made-up tables."""
import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gsl3680_table as tool  # noqa: E402


class TableTests(unittest.TestCase):
    def test_records_in_order_least_significant_first(self):
        text = "static const struct fw_data T[] = {\n {0xf0,0x2},\n {0x7c, 0x11223344}, // {0x00,0x1} a comment\n /* {0x04,0x1} */\n};"
        self.assertEqual(tool.table_bytes(text), bytes([0xF0, 2, 0, 0, 0, 0x7C, 0x44, 0x33, 0x22, 0x11]))

    def test_refuses_what_is_not_such_a_table(self):
        for text in ("", "{0x80,0x1}", "{0x02,0x1}", "{0x00,0x100000000}"):
            with self.subTest(text=text), self.assertRaises(ValueError):
                tool.table_bytes(text)

    def test_refuses_an_oversized_table(self):
        with self.assertRaises(ValueError):
            tool.table_bytes("{0x00,0x1}," * (tool.MAX_BYTES // 5 + 1))

    def test_command_writes_the_file_and_fails_without_a_table(self):
        with tempfile.TemporaryDirectory() as folder:
            source, target = Path(folder) / "t.h", Path(folder) / "gsl3680.fw"
            source.write_text("{0x00,0x5}", encoding="utf-8")
            with contextlib.redirect_stdout(io.StringIO()):
                self.assertEqual(tool.main([str(source), "-o", str(target)]), 0)
            self.assertEqual(target.read_bytes(), bytes([0, 5, 0, 0, 0]))
            source.write_text("nothing", encoding="utf-8")
            with contextlib.redirect_stderr(io.StringIO()):
                self.assertEqual(tool.main([str(source), "-o", str(target)]), 2)


if __name__ == "__main__":
    unittest.main()
