#!/usr/bin/env python3
# Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
"""Writes core/settings_contract_gen.hpp: the copy of the settings contract of EH3D-SERVER that the screen carries.

The server lists every key of its `server.cfg` in `docs/settings-contract.json` (document `eh3d.settings.contract/1`): group, type, range, default, unit, whether a change applies at
once or after a restart, the route that writes it, the role it needs, whether it is a secret, the risk, and the label, the help and the warning in eight languages. The screen
keeps that document as a table of constants (no parsing at the start, no heap) for the day the live contract (`GET /api/v1/settings/contract`) cannot be read; the page then shows
it read-only. The texts are written in the order of lang.hpp (en, es, it, de, fr, pt, zh, ja), not in the order of the contract (en, es, de, fr, it, ja, zh, pt). No text is invented.

    py -3 -I tools/gen_settings_contract.py            write the file
    py -3 -I tools/gen_settings_contract.py --check    fail when the file is not the one the server gives (skipped when EH3D-SERVER is not next to this repository)
"""
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SOURCE = ROOT.parent / "EH3D-SERVER" / "docs" / "settings-contract.json"
OUT = ROOT / "core" / "settings_contract_gen.hpp"

SCHEMA = "eh3d.settings.contract/1"
CONTRACT_LANGS = ["en", "es", "de", "fr", "it", "ja", "zh", "pt"]
SCREEN_LANGS = ["en", "es", "it", "de", "fr", "pt", "zh", "ja"]  # the order of core/lang.hpp
TYPES = {"boolean", "integer", "string", "enum", "list"}
APPLY = {"now", "restart"}
VIA = {"policy", "file"}
RISKS = {"none", "security", "exposure", "capacity", "machine", "data"}


def c_string(text: str) -> str:
    """A C++ string literal with the text as UTF-8 (the same way the other generated headers write theirs)."""
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n") + '"'


def c_or_null(value) -> str:
    return "nullptr" if value is None else c_string(str(value))


def texts(entry: dict, field: str, where: str) -> str:
    """The eight texts of a field as a C++ initializer in the order of the screen; a missing or blank text is an error (no text is ever invented)."""
    table = entry.get(field)
    if not isinstance(table, dict):
        raise ValueError(f"{where}: '{field}' is not a table of texts")
    out = []
    for lang in SCREEN_LANGS:
        text = str(table.get(lang, "")).strip()
        if not text:
            raise ValueError(f"{where}: '{field}' has no text in {lang}")
        out.append(c_string(text))
    return "{" + ", ".join(out) + "}"


def whole(entry: dict, name: str, where: str):
    """A whole number of the contract (or None). A number that is not whole is an error: the screen compares integers only."""
    if name not in entry or entry[name] is None:
        return None
    value = entry[name]
    if isinstance(value, bool) or not isinstance(value, int):
        raise ValueError(f"{where}: '{name}' is not a whole number")
    return value


def check_document(doc: object) -> None:
    """Refuses a document the screen would not understand (another schema, other languages, a type or a risk it does not know, a key twice, a group that is not listed)."""
    if not isinstance(doc, dict) or doc.get("schema") != SCHEMA:
        raise ValueError(f"the schema is not {SCHEMA}")
    if doc.get("languages") != CONTRACT_LANGS:
        raise ValueError(f"the languages are not {CONTRACT_LANGS}")
    groups = doc.get("groups")
    settings = doc.get("settings")
    if not isinstance(groups, list) or not groups or not isinstance(settings, list) or not settings:
        raise ValueError("groups or settings are empty")
    ids = [g.get("id") for g in groups]
    if len(set(ids)) != len(ids) or not all(isinstance(i, str) and i for i in ids):
        raise ValueError("the groups are not unique")
    keys = [s.get("key") for s in settings]
    if len(set(keys)) != len(keys) or not all(isinstance(k, str) and k for k in keys):
        raise ValueError("the keys are not unique")
    for s in settings:
        where = f"setting {s.get('key')}"
        if s.get("group") not in ids:
            raise ValueError(f"{where}: the group {s.get('group')!r} is not listed")
        if s.get("type") not in TYPES:
            raise ValueError(f"{where}: the type {s.get('type')!r} is not one the screen draws")
        if s.get("apply") not in APPLY:
            raise ValueError(f"{where}: apply {s.get('apply')!r}")
        if s.get("risk") not in RISKS:
            raise ValueError(f"{where}: risk {s.get('risk')!r}")
        write = s.get("write")
        if not isinstance(write, dict) or write.get("via") not in VIA:
            raise ValueError(f"{where}: no write")
        if write["via"] == "policy" and not (write.get("method") == "PUT" and write.get("path") == "/api/v1/auth/policy"):
            raise ValueError(f"{where}: a policy key is written by PUT /api/v1/auth/policy only")


def header(doc: dict, digest: str) -> str:
    """The text of the header for a contract document."""
    check_document(doc)
    out = ["// EH3D-NODE-HMI-GUI - core/settings_contract_gen.hpp",
           "// Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.",
           "// Generated by tools/gen_settings_contract.py from docs/settings-contract.json of EH3D-SERVER: do not edit by hand.",
           "// The copy of the contract that the screen carries for when the live one cannot be read. Texts in the order of lang.hpp: en, es, it, de, fr, pt, zh, ja.",
           f"// sha256 of the document: {digest}",
           "#pragma once", "", "#include <cstddef>", "", "#include \"lang.hpp\"", "", "namespace hmi {", "",
           "struct ContractGroupRow {", "    const char* id;", "    Row label;", "};", "",
           "struct ContractRow {",
           "    const char* key;", "    const char* group;", "    const char* type;", "    const char* default_json;  // the default as JSON text; null when the contract has none",
           "    const char* apply;", "    const char* via;", "    const char* method;", "    const char* path;", "    const char* file;", "    const char* role;", "    const char* scope;",
           "    bool confirm_when_looser;", "    bool secret;", "    const char* risk;",
           "    bool has_min;", "    long long min;", "    bool has_max;", "    long long max;", "    const char* unit;", "    int max_length;  // 0: none", "    const char* format;",
           "    const char* values_json;  // the choices of an enum as a JSON array; null when none",
           "    Row label;", "    Row help;", "    Row warning;", "};", "",
           f"inline constexpr const char* kContractCopySchema = {c_string(doc['schema'])};",
           f"inline constexpr std::size_t kContractCopyGroupCount = {len(doc['groups'])};",
           f"inline constexpr std::size_t kContractCopySettingCount = {len(doc['settings'])};", "",
           "inline const ContractGroupRow kContractCopyGroups[kContractCopyGroupCount] = {"]
    for g in doc["groups"]:
        out.append(f"    {{{c_string(g['id'])}, {texts(g, 'label', 'group ' + g['id'])}}},")
    out += ["};", "", "inline const ContractRow kContractCopySettings[kContractCopySettingCount] = {"]
    for s in doc["settings"]:
        where = f"setting {s['key']}"
        w = s["write"]
        lo, hi = whole(s, "min", where), whole(s, "max", where)
        maxlen = whole(s, "max_length", where) or 0
        default = "nullptr" if "default" not in s else c_string(json.dumps(s["default"], ensure_ascii=False, separators=(",", ":")))
        values = "nullptr" if not s.get("values") else c_string(json.dumps(s["values"], ensure_ascii=False, separators=(",", ":")))
        out.append("    {" + ", ".join([
            c_string(s["key"]), c_string(s["group"]), c_string(s["type"]), default, c_string(s["apply"]),
            c_string(w["via"]), c_or_null(w.get("method")), c_or_null(w.get("path")), c_or_null(w.get("file")), c_or_null(s.get("role")), c_or_null(s.get("scope")),
            "true" if s.get("confirm_when_looser") else "false", "true" if s.get("secret") else "false", c_string(s["risk"]),
            "true" if lo is not None else "false", str(lo or 0) + "LL", "true" if hi is not None else "false", str(hi or 0) + "LL",
            c_or_null(s.get("unit")), str(maxlen), c_or_null(s.get("format")), values,
            texts(s, "label", where), texts(s, "help", where), texts(s, "warning", where)]) + "},")
    out += ["};", "", "}  // namespace hmi", ""]
    return "\n".join(out)


def build() -> str:
    raw = SOURCE.read_bytes()
    return header(json.loads(raw.decode("utf-8")), hashlib.sha256(raw.replace(b"\r\n", b"\n")).hexdigest())


def main() -> int:
    if not SOURCE.is_file():
        print("SETTINGS_CONTRACT=SKIP (EH3D-SERVER is not next to this repository)")
        return 0
    text = build()
    if "--check" in sys.argv:
        current = OUT.read_bytes().decode("utf-8").replace("\r\n", "\n") if OUT.is_file() else ""
        if current != text:
            print("SETTINGS_CONTRACT=FAIL (core/settings_contract_gen.hpp is not the one the server gives: run py -3 -I tools/gen_settings_contract.py)")
            return 1
        print(f"SETTINGS_CONTRACT=PASS ({text.count(chr(10) + '    {' + chr(34))} lines of the table)")
        return 0
    OUT.write_bytes(text.encode("utf-8"))
    print(f"wrote {OUT.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
