# Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
"""Tests of tools/gen_settings_contract.py: the header it writes is well formed, refuses a document the screen would not understand and, when the server is next to this repository,
it is the one on disk and says what the server says (89 settings in 15 groups today)."""
import copy
import json
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import gen_settings_contract as gen  # noqa: E402

def texts(word):
    return {code: f"{word} {code}" for code in gen.CONTRACT_LANGS}


def setting(key, type_="boolean", via="policy", **extra):
    entry = {"key": key, "group": "network", "type": type_, "default": False, "apply": "now", "role": "admin", "scope": "admin:policy", "confirm_when_looser": True, "secret": False,
             "risk": "exposure", "label": texts("L"), "help": texts("H"), "warning": texts("W")}
    entry["write"] = {"via": via, "method": "PUT", "path": "/api/v1/auth/policy"} if via == "policy" else {"via": "file", "method": None, "path": None, "file": "server.cfg"}
    entry.update(extra)
    return entry


def document(*settings):
    return {"schema": gen.SCHEMA, "languages": list(gen.CONTRACT_LANGS), "groups": [{"id": "network", "label": texts("Network")}], "settings": list(settings)}


class HeaderTests(unittest.TestCase):
    def test_a_row_per_setting_with_the_texts_in_the_order_of_the_screen(self):
        text = gen.header(document(setting("a"), setting("b", "integer", via="file", min=1, max=9, unit="seconds", default=3)), "0" * 64)
        self.assertIn("kContractCopySettingCount = 2;", text)
        self.assertIn("kContractCopyGroupCount = 1;", text)
        self.assertIn('{"a", "network", "boolean", "false", "now", "policy", "PUT", "/api/v1/auth/policy", nullptr, "admin", "admin:policy", true, false, "exposure", false, 0LL, false, 0LL,', text)
        self.assertIn('"integer", "3", "now", "file", nullptr, nullptr, "server.cfg"', text)
        self.assertIn("true, 1LL, true, 9LL", text)
        # en, es, it, de, fr, pt, zh, ja: the order of lang.hpp, not the one of the contract
        self.assertIn('{"L en", "L es", "L it", "L de", "L fr", "L pt", "L zh", "L ja"}', text)
        self.assertIn("sha256 of the document: " + "0" * 64, text)

    def test_quotes_and_backslashes_are_escaped(self):
        entry = setting("a", warning={**texts("W"), "en": 'say "x" \\ y'})
        self.assertIn('"say \\"x\\" \\\\ y"', gen.header(document(entry), "0" * 64))

    def test_the_default_of_a_list_and_a_text_is_json_text(self):
        text = gen.header(document(setting("l", "list", via="file", default=["a", "b"]), setting("t", "string", via="file", default="127.0.0.1:1", max_length=63)), "0" * 64)
        self.assertIn('"[\\"a\\",\\"b\\"]"', text)
        self.assertIn('"\\"127.0.0.1:1\\""', text)
        self.assertIn(", 63, ", text)

    def test_a_setting_with_no_default_has_a_null_one(self):
        entry = setting("a")
        del entry["default"]
        self.assertIn('{"a", "network", "boolean", nullptr, "now"', gen.header(document(entry), "0" * 64))

    def test_a_document_the_screen_would_not_understand_is_refused(self):
        good = document(setting("a"))
        cases = {
            "schema": lambda d: d.update(schema="eh3d.settings.contract/2"),
            "languages": lambda d: d.update(languages=["en"]),
            "no settings": lambda d: d.update(settings=[]),
            "no groups": lambda d: d.update(groups=[]),
            "key twice": lambda d: d["settings"].append(setting("a")),
            "group twice": lambda d: d["groups"].append(d["groups"][0]),
            "unknown group": lambda d: d["settings"][0].update(group="ghost"),
            "unknown type": lambda d: d["settings"][0].update(type="float"),
            "unknown apply": lambda d: d["settings"][0].update(apply="later"),
            "unknown risk": lambda d: d["settings"][0].update(risk="scary"),
            "no write": lambda d: d["settings"][0].pop("write"),
            "another route": lambda d: d["settings"][0]["write"].update(path="/api/v1/machines/m1/stop"),
            "another method": lambda d: d["settings"][0]["write"].update(method="POST"),
            "blank text": lambda d: d["settings"][0]["label"].update(fr=" "),
            "missing text": lambda d: d["settings"][0]["help"].pop("ja"),
            "fraction": lambda d: d["settings"][0].update(min=1.5),
            "group label": lambda d: d["groups"][0]["label"].pop("zh"),
        }
        for name, change in cases.items():
            with self.subTest(name):
                bad = copy.deepcopy(good)
                change(bad)
                with self.assertRaises(ValueError):
                    gen.header(bad, "0" * 64)

    def test_a_file_key_may_name_any_route_because_it_is_never_written(self):
        entry = setting("f", via="file")
        entry["write"]["path"] = "/anything"
        gen.header(document(entry), "0" * 64)


@unittest.skipUnless(gen.SOURCE.is_file(), "EH3D-SERVER is not next to this repository")
class SourceTests(unittest.TestCase):
    def test_the_file_on_disk_is_the_one_the_server_gives(self):
        self.assertEqual(gen.OUT.read_bytes().decode("utf-8").replace("\r\n", "\n"), gen.build())

    def test_the_server_says_what_the_screen_expects(self):
        doc = json.loads(gen.SOURCE.read_text(encoding="utf-8"))
        gen.check_document(doc)
        self.assertEqual(len(doc["settings"]), 89)
        self.assertEqual(len(doc["groups"]), 15)
        self.assertEqual(sum(1 for s in doc["settings"] if s["write"]["via"] == "policy"), 19)
        self.assertEqual(sum(1 for s in doc["settings"] if s["write"]["via"] == "file"), 70)

    def test_every_key_of_the_server_is_a_row_of_the_table(self):
        doc = json.loads(gen.SOURCE.read_text(encoding="utf-8"))
        text = gen.OUT.read_text(encoding="utf-8")
        for s in doc["settings"]:
            self.assertIn('    {"' + s["key"] + '", "' + s["group"] + '", "', text)


if __name__ == "__main__":
    unittest.main()
