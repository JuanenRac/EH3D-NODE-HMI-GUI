"""Tests of tools/make_locales.py: the names of the eight languages, and the check that fails when a language of the past comes back."""
import unittest

import make_locales as ml


class LocalesTest(unittest.TestCase):
    def test_check_passes_on_the_repository(self):
        self.assertEqual(ml.problems(), [])

    def test_the_eight_languages_are_the_ones_of_the_project(self):
        self.assertEqual([c for c, _ in ml.languages()], ["en", "es", "it", "de", "fr", "pt", "zh", "ja"])

    def test_a_language_that_is_not_one_of_the_eight_fails_the_check(self):
        original = ml.languages

        def with_russian():
            return list(original()) + [("ru", "Русский")]

        ml.languages = with_russian
        try:
            bad = ml.problems()
        finally:
            ml.languages = original
        self.assertTrue(any("names.tsv" in b for b in bad), bad)


if __name__ == "__main__":
    unittest.main()
