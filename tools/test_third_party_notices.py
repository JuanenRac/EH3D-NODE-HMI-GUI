"""Tests of tools/third_party_notices.py with a made-up build (no ESP-IDF, no network)."""
from __future__ import annotations

import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import third_party_notices as notices  # noqa: E402


def put(path: Path, text: str) -> Path:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")
    return path


class Notices(unittest.TestCase):
    def setUp(self) -> None:
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        self.root = Path(folder.name).resolve()
        idf = self.root / "esp-idf"
        put(idf / "LICENSE", "Apache License\nof ESP-IDF")
        put(idf / "components" / "net" / "net" / "COPYING", "Copyright of the net stack")
        put(idf / "components" / "net" / "test" / "LICENSE", "only of a test")
        thing = self.root / "managed_components" / "maker__thing"
        put(thing / "LICENSE", "Permission is hereby granted, free of charge, by Maker")
        put(thing / "idf_component.yml", "version: \"1.2.0\"\n")
        put(self.root / "main" / "idf_component.yml", "dependencies:\n  idf: \">=5.5\"\n  maker/thing: \"^1\"\n")
        self.description = put(self.root / "build" / "project_description.json", json.dumps({
            "idf_path": str(idf), "git_revision": "v5.5.5", "build_components": ["net", "maker__thing"],
            "build_component_info": {"net": {"dir": str(idf / "components" / "net")}, "maker__thing": {"dir": str(thing)}}}))

    def test_the_file_carries_the_texts_of_the_build_and_passes_its_check(self) -> None:
        self.assertEqual(notices.main([str(self.description), "--root", str(self.root)]), 0)
        written = (self.root / notices.OUTPUT).read_text(encoding="utf-8")
        self.assertIn("maker/thing | 1.2.0 | MIT", written)
        self.assertIn("Copyright of the net stack", written)
        self.assertNotIn("only of a test", written)
        self.assertEqual(notices.main(["--root", str(self.root), "--check"]), 0)

    def test_a_declared_dependency_that_is_not_listed_fails_the_check(self) -> None:
        notices.main([str(self.description), "--root", str(self.root)])
        put(self.root / "dependencies.lock", "dependencies:\n  maker/other:\n    version: 1.0.0\n  idf:\n    version: 5.5.5\ntarget: esp32s3\n")
        self.assertEqual(notices.declared(self.root), ["idf", "maker/other", "maker/thing"])
        self.assertEqual(notices.main(["--root", str(self.root), "--check"]), 1)

    def test_a_missing_file_fails_the_check(self) -> None:
        self.assertEqual(notices.main(["--root", str(self.root), "--check"]), 1)


class TheRepository(unittest.TestCase):
    def test_the_notices_of_the_repository_cover_what_it_declares(self) -> None:
        written = (notices.ROOT / notices.OUTPUT).read_text(encoding="utf-8")
        self.assertEqual(notices.problems(written, notices.declared(notices.ROOT)), [])


if __name__ == "__main__":
    unittest.main()
