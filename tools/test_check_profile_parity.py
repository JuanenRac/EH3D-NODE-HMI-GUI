# Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
"""Tests of tools/check_profile_parity.py with two small repositories made on the spot, and (when the Bungard screen is next to this repository) the real comparison."""
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_profile_parity as parity  # noqa: E402


class Pair:
    """Two repositories in a temporary folder, wired into the module."""

    def __init__(self):
        self.tmp = tempfile.TemporaryDirectory()
        base = Path(self.tmp.name)
        self.generic, self.other = base / "EH3D-NODE-HMI-GUI", base / "EH3D-NODE-BUNGARD-HMI"
        for repo in (self.generic, self.other):
            (repo / "core").mkdir(parents=True)
            (repo / "tools").mkdir()
        (self.generic / "profiles").mkdir()
        self.saved = (parity.ROOT, parity.OTHER, parity.BASELINE, parity.PROFILE)
        parity.ROOT, parity.OTHER = self.generic, self.other
        parity.BASELINE, parity.PROFILE = self.generic / "tools" / "baseline.json", self.generic / "profiles" / "bungard.cfg"

    def write(self, repo, rel, text):
        path = repo / rel
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(text.encode("utf-8"))

    def close(self):
        parity.ROOT, parity.OTHER, parity.BASELINE, parity.PROFILE = self.saved
        self.tmp.cleanup()


class CompareTests(unittest.TestCase):
    def setUp(self):
        self.pair = Pair()
        self.addCleanup(self.pair.close)

    def test_the_names_of_the_projects_do_not_count(self):
        self.pair.write(self.pair.generic, "core/a.hpp", "// EH3D-NODE-HMI-GUI - core/a.hpp\r\nint eh3d_node_hmi_gui;\n")
        self.pair.write(self.pair.other, "core/a.hpp", "// EH3D-NODE-BUNGARD-HMI - core/a.hpp\nint eh3d_node_bungard_hmi;\n")
        self.assertEqual(parity.compare(), {"different": [], "only_generic": [], "only_bungard": []})

    def test_a_difference_and_a_lonely_file_are_found(self):
        self.pair.write(self.pair.generic, "core/a.hpp", "int a;\n")
        self.pair.write(self.pair.other, "core/a.hpp", "int b;\n")
        self.pair.write(self.pair.generic, "core/only_here.hpp", "x\n")
        self.pair.write(self.pair.other, "core/only_there.hpp", "x\n")
        found = parity.compare()
        self.assertEqual(found["different"], ["core/a.hpp"])
        self.assertEqual(found["only_generic"], ["core/only_here.hpp"])
        self.assertEqual(found["only_bungard"], ["core/only_there.hpp"])

    def test_the_screens_themselves_are_not_compared(self):
        self.pair.write(self.pair.generic, "main/ui.cpp", "one\n")
        self.pair.write(self.pair.other, "main/ui.cpp", "two\n")
        self.assertEqual(parity.compare()["different"], [])

    def test_a_new_difference_fails_and_a_closed_one_is_stale(self):
        self.pair.write(self.pair.generic, "core/a.hpp", "int a;\n")
        self.pair.write(self.pair.other, "core/a.hpp", "int b;\n")
        self.pair.write(self.pair.generic, "hmi.cfg.example", "language = en\n")
        self.pair.write(self.pair.other, "hmi.cfg.example", "language = en\n")
        for repo in (self.pair.generic, self.pair.other):
            self.pair.write(repo, "core/hmi_config.hpp", 'key == "language"')
        self.pair.write(self.pair.generic, "profiles/bungard.cfg", "# nothing\n")
        new, stale, cfg = parity.problems()
        self.assertEqual((new, stale, cfg), (["different: core/a.hpp"], [], []))
        parity.BASELINE.write_text(json.dumps({"different": ["core/a.hpp", "core/gone.hpp"]}), encoding="utf-8")
        new, stale, cfg = parity.problems()
        self.assertEqual((new, stale, cfg), ([], ["different: core/gone.hpp"], []))


class ProfileTests(unittest.TestCase):
    def setUp(self):
        self.pair = Pair()
        self.addCleanup(self.pair.close)
        self.pair.write(self.pair.generic, "core/hmi_config.hpp", 'key == "language" key == "jog_step" key == "machine_kind"')

    def test_the_profile_makes_the_generic_settings_the_bungard_ones(self):
        self.pair.write(self.pair.generic, "hmi.cfg.example", "language = en\nmachine_kind = cnc\n")
        self.pair.write(self.pair.other, "hmi.cfg.example", "language = en\njog_step = 1\n")
        self.pair.write(self.pair.generic, "profiles/bungard.cfg", "machine_kind = cnc\njog_step = 1\n")
        self.assertEqual(parity.profile_problems(), [])

    def test_a_value_that_differs_is_reported(self):
        self.pair.write(self.pair.generic, "hmi.cfg.example", "language = en\n")
        self.pair.write(self.pair.other, "hmi.cfg.example", "language = es\n")
        self.pair.write(self.pair.generic, "profiles/bungard.cfg", "# nothing\n")
        self.assertTrue(any("language" in p for p in parity.profile_problems()))

    def test_a_missing_profile_and_an_unknown_key_are_reported(self):
        self.pair.write(self.pair.generic, "hmi.cfg.example", "language = en\n")
        self.pair.write(self.pair.other, "hmi.cfg.example", "language = en\n")
        self.assertTrue(any("missing" in p for p in parity.profile_problems()))
        self.pair.write(self.pair.generic, "profiles/bungard.cfg", "no_such_key = 1\n")
        self.pair.write(self.pair.other, "hmi.cfg.example", "language = en\nno_such_key = 1\n")
        self.assertTrue(any("does not read" in p for p in parity.profile_problems()))

    def test_a_setting_waiting_for_the_sync_is_left_out_until_it_arrives(self):
        self.pair.write(self.pair.generic, "hmi.cfg.example", "language = en\njog_step = 1\n")
        self.pair.write(self.pair.other, "hmi.cfg.example", "language = en\n")
        self.pair.write(self.pair.generic, "profiles/bungard.cfg", "# nothing\n")
        parity.BASELINE.write_text(json.dumps({"settings_not_yet_in_bungard": ["jog_step"]}), encoding="utf-8")
        self.assertEqual(parity.profile_problems(), [])
        self.pair.write(self.pair.other, "hmi.cfg.example", "language = en\njog_step = 1\n")
        self.assertTrue(any("take it out" in p for p in parity.profile_problems()))


class RealPairTests(unittest.TestCase):
    @unittest.skipUnless(parity.OTHER.is_dir(), "EH3D-NODE-BUNGARD-HMI is not next to this repository")
    def test_the_two_screens_differ_only_where_the_baseline_says(self):
        new, stale, cfg = parity.problems()
        self.assertEqual((new, stale, cfg), ([], [], []))


if __name__ == "__main__":
    unittest.main()
