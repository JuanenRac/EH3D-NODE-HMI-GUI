# Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
"""Tests of tools/gen_server_errors.py: the header it writes is well formed and, when the server and the phone app are next to this repository, it is the one on disk."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen_server_errors as gen  # noqa: E402

EIGHT = ["en", "es", "it", "de", "fr", "pt", "zh", "ja"]


class HeaderTests(unittest.TestCase):
    def test_rows_are_sorted_and_have_eight_texts(self):
        text = gen.header({"zeta": EIGHT, "alpha": EIGHT})
        self.assertLess(text.index('"alpha"'), text.index('"zeta"'))
        self.assertIn("kServerErrorCount = 2;", text)
        self.assertEqual(text.count('{"alpha", {'), 1)

    def test_quotes_and_backslashes_are_escaped(self):
        text = gen.header({"a": ['say "x"', "back\\slash"] + EIGHT[2:]})
        self.assertIn('"say \\"x\\""', text)
        self.assertIn('"back\\\\slash"', text)

    def test_a_row_without_eight_texts_is_refused(self):
        with self.assertRaises(ValueError):
            gen.header({"a": EIGHT[:7]})
        with self.assertRaises(ValueError):
            gen.header({"a": EIGHT[:7] + [" "]})

    def test_a_bad_code_is_refused(self):
        for code in ("", "Bad", "has space", "a-b"):
            with self.assertRaises(ValueError):
                gen.header({code: EIGHT})


class SourcesTests(unittest.TestCase):
    @unittest.skipUnless(gen.CODES.is_file() and gen.TABLE.is_file(), "EH3D-SERVER or EH3D-ANDROID-CONTROL is not next to this repository")
    def test_the_file_on_disk_is_the_one_the_sources_give(self):
        text, missing = gen.build()
        self.assertEqual(gen.OUT.read_bytes().decode("utf-8").replace("\r\n", "\n"), text)
        self.assertIsInstance(missing, list)

    @unittest.skipUnless(gen.CODES.is_file() and gen.TABLE.is_file(), "EH3D-SERVER or EH3D-ANDROID-CONTROL is not next to this repository")
    def test_every_code_of_the_table_is_a_code_of_the_server(self):
        import json
        codes = {item["code"] for item in json.loads(gen.CODES.read_text(encoding="utf-8"))}
        table = json.loads(gen.TABLE.read_text(encoding="utf-8"))
        self.assertEqual(set(table) - codes, set())


if __name__ == "__main__":
    unittest.main()
