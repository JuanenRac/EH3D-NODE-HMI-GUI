# The settings of the server on the screen

EH3D-SERVER publishes every key of its `server.cfg` as a contract (`GET /api/v1/settings/contract`, and `docs/settings-contract.json` in the server's repository): the group, type, range,
default and unit of each key, whether a change applies at once or after a restart, the route that writes it, the role and scope it needs, whether it is a secret, its risk, and its label,
help and warning in eight languages. Studio, the Android app and the iPhone app draw their pages of settings from it. This screen does too: it is one more client, and a new key of the
server reaches it without a change of the code.

The screen is a touch screen next to a machine, so it is stricter than the other clients about what it **changes**. It reads everything; it changes nothing unless three lines of `hmi.cfg`
say so and the login of the screen is an administrator.

## What the screen shows

The page of the settings has a block "Server settings": one line telling where the values come from, one line saying why, when the copy is shown, any warning about what the server and
`hmi.cfg` disagree on, and then the 17 groups of the contract with every setting (105 today) and its value in force. A value that is not the default of the contract is drawn in amber.
A switch is On or Off, a number has its unit (`12 h`, `8 MB`), a list is its items, a secret is `Hidden` (the screen never asks for it and never shows what the server might send).

* **Live contract.** The link asks `GET /api/v1/settings/contract` after the first reading of the fleet and then every `settings_poll_s` seconds (600 by default). The answer is about
  250 KB, so the sockets keep up to 1 MiB for it (any other answer keeps 256 KiB). The server gives it to an administrator with the scope `admin:policy` only.
* **The copy.** When the live contract cannot be read (the login of the screen is an operator, nobody answers, the answer is too long or is not a contract this screen understands, a
  newer schema), the page shows the copy the screen carries (`core/settings_contract_gen.hpp`): every label, group and warning in eight languages, no values, **read only**, and a line
  with the reason in the language of the operator.
* **A server newer than the screen.** A key the live contract has and the copy does not is drawn from the live contract; a key that has a value and no entry is drawn as a read-only line
  with its name; the page counts them. A setting whose type the screen does not know is a read-only line. A schema of a newer version is not drawn at all: the copy is shown.
* **What the server and `hmi.cfg` say about each other.** If `rack_max_slots` of the server is below the highest place of `rack_places` the page warns; the core also has
  `body_warning`, the sentence for a file larger than `max_body_mb`.

## The light view for a login that is not an administrator

The full contract needs an administrator with the scope `admin:policy`, so the `hmi` login of an operator could never read it and showed only the copy with no value. EH3D-SERVER also
gives `GET /api/v1/settings/values` (document `eh3d.settings.values/1`, `docs/CLIENT_SYNC.md` of the server): read only, for any login, a few kilobytes and never more than 16 KiB. For each key it gives
`v` (the value in force), `origin` (`file` or `override`) and `editable`; a key marked `secret` has `v` = `***`, a key whose format is a path of the computer has `hidden` and no `v`, and a long
text or list is cut (`truncated`). It has no text, so the page keeps the labels of its own copy and shows the values next to them.

* **Who reads what.** An administrator asks for the full contract first (it has the texts and the values). Anybody else, and an administrator whose full contract is refused, asks for the
  light view when `settings_values` is on. With `settings_values = off` a login that is not an administrator still tries the full contract and shows the copy when it is refused.
* **`ETag`.** The answer has an `ETag`; the link sends it back in `If-None-Match` and the server answers `304` with no body while nothing changed. The tag is forgotten when the login
  changes or a read fails, and only a quoted run of printable characters of up to 100 is ever sent or kept. The page is asked again every `settings_values_poll_s` seconds (30).
* **The hash.** The generator writes the SHA-256 of `docs/settings-contract.json` (`kContractCopyHash`). The page compares it with the `contract_hash` of the answer (the full contract has it
  too) and warns when they differ: the texts and keys of the copy may be out of date. A value that is not a SHA-256 says nothing.
* **What is never kept.** A secret (or a value that is the mark `***`) is Hidden and no value is kept; a hidden key shows that the server does not show it; a key the copy does not know is
  drawn read only. An answer with a key that is not a name, an entry that is not an object, a value that is an object, a text over 1024 bytes, a list over 64 items, more than 256 keys or a
  `count` that is not the number of keys is not drawn at all; the copy is shown with the reason.
* **Read only.** Nothing is written from the light view, whatever `hmi.cfg` says: a change needs the live full contract.

## What the screen may change

Of the 105 settings, 86 are set in `server.cfg` and read when the server starts: the screen shows them and says where they are set, and never writes them. The other 19 are the keys of the
security page, written through `PUT /api/v1/auth/policy`. The screen changes one of those only when **all** of this holds:

1. The contract is the live one (the copy never writes).
2. The route of the key is `PUT /api/v1/auth/policy` and no other (a contract that names another route for a key makes it read only here: the token of the screen goes nowhere else).
3. The login of the screen is an administrator (the role the server gave at the login; a screen that uses the static `token` never learns its role and so never changes a setting).
4. `settings_write = on` (it is `off`).
5. The key is in `settings_write_keys` (it is empty).
6. No job runs or waits (`settings_block_running`, `on`).

A change is asked with the words of the setting ("Change Slicing enabled to On?"), a risky one (all 19 are, by their risk of `security` or `exposure`) shows its warning and asks a second
time, and a value the server calls looser than recommended (`409 weak_confirm_required`) is asked about a third time and then sent with `confirm_weak`. Each question has its own time
(`confirm_timeout_ms`) and gives up by itself. A spoken order never changes a setting. A secret is never shown, asked for or written. The answer is read by code: saved (and whether it
is read after a restart), refused (the text of the code in the eight languages), or nothing was sent. An answer of 200 that is not the JSON of the server is not a saved change.

### Which keys it makes sense to list

| Group of the contract | Keys | Written through the policy | On this screen |
|---|---:|---:|---|
| Network | 7 | 2 | `network_open`, `mdns_announce`: do not list (they open the server to the outside) |
| Storage and limits of a request | 5 | 0 | read only (set in `server.cfg`) |
| Logins and passwords | 14 | 9 | `url_token`, `persist_sessions`, `login_ip_limit`, `auth_*`, `password_*`: do not list (a wrong value locks people out or weakens the logins) |
| Audit and logs | 4 | 1 | `audit_denials`: do not list |
| Updates | 3 | 2 | `update_allow_unsigned`, `update_keep_logins`: do not list |
| Machines and G-code jobs | 7 | 0 | read only; the values that matter to an operator are shown (timeouts, places of the bar) |
| Moonraker and OctoPrint routes | 2 | 2 | `compat_api`, `compat_write`: do not list |
| Slicing, Bambu Lab printers, Design program | 6 + 3 + 5 | 1 + 1 + 1 | `slice_enabled`, `bambu_lan_enabled`, `design_run` are the switches of a feature; list one only for a commissioning visit with an administrator at the screen |
| Computing tasks, Artifacts, Project store, Shared catalogs, Events and queues | 33 | 0 | read only |

The default is that the screen changes none of the 105: that is the recommended setup for a screen left next to a machine. The right place to change the security of the server is Studio, in
the office, with a person logged in. `settings_write_keys` exists for the visit where an administrator stands at the screen.

## The lines of `hmi.cfg`

| Key | Default | Meaning |
|---|---|---|
| `settings_contract` | `on` | read the contract now and then and show the settings (reading changes nothing) |
| `settings_poll_s` | `600` | how often it is read again, 30 to 86400 seconds; after a failure it is tried again after this time but never later than five minutes |
| `settings_values` | `on` | a login that is not an administrator reads the light view of the values in force instead of showing only the copy |
| `settings_values_poll_s` | `30` | how often the light view is asked for, 5 to 3600 seconds |
| `settings_write` | `off` | SAFETY: the screen may change a setting of the server |
| `settings_write_keys` | empty | SAFETY: the keys it may change, like `slice_enabled, design_run` (up to 32 names) |
| `settings_block_running` | `on` | SAFETY: no change while a job runs or waits |

A line that cannot be used is reported and changes nothing, so a typing mistake never opens the writing.

## Kept in step with the server

* `tools/gen_settings_contract.py` writes `core/settings_contract_gen.hpp` from `docs/settings-contract.json` of the server (the eight texts in the order of the tables of the screen) and
  refuses a document the screen would not understand. With the server next to this repository `--check` fails when the file is not the one the server gives; without it the check is skipped.
* `tests/test_settings.cpp` is the guard that does not need the server: the copy has 105 settings in 17 groups, 19 through the policy and 86 in the file, every one with a label, a help and a
  warning in the eight languages and a control the screen knows; the copy written as the server writes it and read again by the code that reads the live answer is the same;
  the rules of who may change what, the steps of a change, the answers by code, the texts and a seeded storm of failures against a simulated server.
* A key the server adds is shown the next time the contract is read; the file is regenerated with the next copy of the server's contract.

## What has not run

The core (`core/settings_contract.hpp`, `core/settings_values.hpp`, `core/settings_text.hpp`, `core/settings_page.hpp`) is tested on a computer, including with AddressSanitizer and UBSan. The firmware that reads the
contract and draws the block is written but has not been built or run on the module: the controls that change a setting (a switch, a number pad) are not drawn yet, so today the
page is read only on the screen even with every line of `hmi.cfg` open. A text with a character that the fonts of the screen do not hold is shown in English; the fonts hold the characters of the eight languages of the copy the screen carries, and a text of a newer server with another character is shown in English until the fonts are built again. The new fonts have not been seen on the module. The parse of the answer (some 250 KB) has not been measured on the module. The reading of the `ETag` header in the sockets (`main/server_link.cpp`) and the light view on the module have not been built or run against a real server; the light view and its `304` are tested against a simulated server.
