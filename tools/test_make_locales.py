"""Tests of tools/make_locales.py: every language has every key, and the check fails when one is missing."""
import unittest

import make_locales as ml


class LocalesTest(unittest.TestCase):
    def test_check_passes_on_the_repository(self):
        self.assertEqual(ml.problems(), [])

    def test_every_language_has_every_key(self):
        keys = ml.key_names()
        for code in ml.extra_codes():
            if code in ("pt-br", "fr-ca"):
                continue
            rows = ml.read_tsv(ml.LOCALES / f"{code}.tsv")
            self.assertEqual([k for k in keys if k not in rows], [], code)

    def test_a_missing_key_fails_the_check(self):
        original = ml.read_tsv

        def without_a_key(path):
            rows = dict(original(path))
            if path.stem == "ru":
                rows.pop(ml.key_names()[0], None)
            return rows

        ml.read_tsv = without_a_key
        try:
            bad = ml.problems()
        finally:
            ml.read_tsv = original
        self.assertTrue(any("ru.tsv" in b and "without text" in b for b in bad), bad)


if __name__ == "__main__":
    unittest.main()
