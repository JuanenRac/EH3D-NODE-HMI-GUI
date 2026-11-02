#!/usr/bin/env python3
"""Hardware-day self-test of the EH3D-NODE-HMI-GUI screen firmware (standard library only, Windows and Linux).

The screen has no HTTP API of its own: it reads the EH3D server on port 18200. This tool needs no screen and no server, and sends nothing to hardware:

  1. it reads hmi.cfg.example (or --cfg FILE) with the same rules as the firmware (core/hmi_config.hpp) and reports every line the screen would reject;
  2. it starts a toy server on this computer with the routes the screen asks for (fleet, machine, spindle, job, light) and checks that each answer has what the
     screen's parsers need (core/server_link.hpp), that a wrong token is refused and that the default port is 18200;
  3. with --server HOST:PORT it asks a REAL server the same read-only questions (GET only; nothing is started, paused or cancelled).

    python tools/selftest.py
    python tools/selftest.py --cfg hmi.cfg --server 192.168.0.180:18200 --token TOKEN --machine bungard

The report selftest_hmi-gui_<time>.txt is written to the current folder: one line per step (OK, FALLO, OMITIDO) with the measured value.
The manual steps for the operator are in docs/HARDWARE_CHECKLIST.md.
"""
import argparse
import http.client
import http.server
import json
import os
import re
import socket
import ssl
import struct
import sys
import threading
import time

NODE = "hmi-gui"
HAS_MACHINE_KIND = True
DEFAULT_PORT = 18200
TOY_TOKEN = "toy-screen-token-12345678"

# ---- COMMON BEGIN (identical in every node repo; do not edit one copy alone) ----
OK = "OK"
FAIL = "FALLO"
SKIP = "OMITIDO"


class Fail(Exception):
    """A step that did not pass."""


class Skip(Exception):
    """A step that could not run (and why)."""


class Report:
    """One line per step. Every text that goes out is cleaned of the secrets the run knows."""

    def __init__(self, node, host):
        self.node, self.host = node, host
        self.secrets = []
        self.lines = []
        self.counts = {OK: 0, FAIL: 0, SKIP: 0}
        self.started = time.strftime("%Y-%m-%d %H:%M:%S")

    def hide(self, *values):
        for v in values:
            if isinstance(v, str) and len(v) >= 4 and v not in self.secrets:
                self.secrets.append(v)

    def clean(self, text):
        text = str(text)
        for s in sorted(self.secrets, key=len, reverse=True):
            text = text.replace(s, "***")
        return text

    def add(self, status, name, detail=""):
        line = "%-9s%s" % (status, name) + (" | " + detail if detail else "")
        line = self.clean(line)
        try:
            print(line, flush=True)
        except (OSError, UnicodeError):
            pass   # a closed pipe or a console that cannot show a character must not stop the run: the report file has the line
        self.lines.append(line)
        self.counts[status] += 1

    def step(self, name, fn):
        """Runs fn; it returns the measured value (text), or raises Fail / Skip."""
        try:
            self.add(OK, name, fn() or "")
        except Skip as e:
            self.add(SKIP, name, str(e))
        except Fail as e:
            self.add(FAIL, name, str(e))
        except Exception as e:  # a broken answer or a dead node is a failed step, never a crash
            self.add(FAIL, name, "%s: %s" % (type(e).__name__, e))

    def text(self):
        head = ["selftest %s" % self.node, "host: %s" % self.host, "started: %s" % self.started, ""]
        tail = ["", "OK=%d FALLO=%d OMITIDO=%d" % (self.counts[OK], self.counts[FAIL], self.counts[SKIP])]
        return self.clean("\n".join(head + self.lines + tail)) + "\n"

    def write(self, out_dir):
        path = os.path.join(out_dir, "%s_%s_%s.txt" % (getattr(self, "prefix", "selftest"), self.node, time.strftime("%Y%m%d_%H%M%S")))
        with open(path, "w", encoding="utf-8") as f:
            f.write(self.text())
        return path


def confirm(prompt, yes=False, ask=input):
    """The interactive gate of every active test: the person types SI. --yes (asked for expressly) skips the question; no answer is a no."""
    print(prompt, flush=True)
    if yes:
        print("(--yes: confirmed by whoever runs this)", flush=True)
        return True
    try:
        return ask("Escribe SI para seguir: ").strip() == "SI"
    except (EOFError, KeyboardInterrupt):
        return False


def secret_values(doc):
    """Every string kept under a key that names a password or a token, so that no report can print one."""
    found = []
    if isinstance(doc, dict):
        for k, v in doc.items():
            if isinstance(v, str) and v and any(w in k.lower() for w in ("password", "token", "secret", "psk", "key")):
                found.append(v)
            else:
                found.extend(secret_values(v))
    elif isinstance(doc, list):
        for v in doc:
            found.extend(secret_values(v))
    return found


class Http:
    """A small HTTP client. It writes nothing to the node unless allow_write is set (the login is the only exception: it opens a session)."""

    def __init__(self, host, port=80, scheme="http", insecure=False, timeout=5.0, bearer=""):
        self.host, self.port, self.scheme, self.insecure, self.timeout, self.bearer = host, int(port), scheme, insecure, timeout, bearer
        self.cookie = ""
        self.allow_write = False

    def request(self, method, path, body=None, headers=None, login=False):
        if method not in ("GET", "HEAD") and not self.allow_write and not login:
            raise RuntimeError("write refused: %s %s needs --active" % (method, path))
        if self.scheme == "https":
            ctx = ssl.create_default_context()
            if self.insecure:
                ctx.check_hostname = False
                ctx.verify_mode = ssl.CERT_NONE
            conn = http.client.HTTPSConnection(self.host, self.port, timeout=self.timeout, context=ctx)
        else:
            conn = http.client.HTTPConnection(self.host, self.port, timeout=self.timeout)
        hdr = {"X-Requested-With": "eh3d"}
        if self.bearer:
            hdr["Authorization"] = "Bearer " + self.bearer
        if self.cookie:
            hdr["Cookie"] = self.cookie
        if headers:
            hdr.update(headers)
        data = None
        if body is not None:
            data = body if isinstance(body, bytes) else json.dumps(body).encode()
            hdr.setdefault("Content-Type", "application/json")
        try:
            conn.request(method, path, body=data, headers=hdr)
            resp = conn.getresponse()
            payload = resp.read()
            for k, v in resp.getheaders():
                if k.lower() == "set-cookie" and "=" in v and "expires=thu, 01 jan 1970" not in v.lower():
                    self.cookie = v.split(";", 1)[0]
            return resp.status, payload
        finally:
            conn.close()

    def get_json(self, path):
        status, payload = self.request("GET", path)
        if status != 200:
            raise Fail("GET %s answered HTTP %d" % (path, status))
        try:
            doc = json.loads(payload.decode("utf-8"))
        except ValueError:
            raise Fail("GET %s did not answer valid JSON" % path)
        if not isinstance(doc, dict):
            raise Fail("GET %s did not answer a JSON object" % path)
        return doc

    def login(self, user, password):
        status, _ = self.request("POST", "/api/v1/login", {"user": user, "password": password}, login=True)
        if status != 200:
            raise Fail("login refused (HTTP %d)" % status)
        if not self.cookie:
            raise Fail("login gave no session")


def check_panel(rep, http, user, password):
    """The read-only battery of the panel API shared by the nodes: version, state, settings, export, backup networks, OTA. Returns the status document or None."""
    box = {}

    def session():
        d = http.get_json("/api/v1/session")
        box["session"] = d
        return "node_id=%s, keys=%d" % (d.get("node_id", "?"), len(d))

    rep.step("panel: session (no login)", session)
    if not user or not password:
        for n in ("panel: login", "panel: status", "panel: settings", "panel: settings export is repeatable", "panel: backup Wi-Fi networks", "panel: OTA state"):
            rep.add(SKIP, n, "give --user and --password (or EH3D_USER / EH3D_PASSWORD)")
        return None

    rep.step("panel: login", lambda: http.login(user, password) or "session opened")
    if not http.cookie:
        return None

    def status():
        d = http.get_json("/api/v1/status")
        box["status"] = d
        if not d.get("version"):
            raise Fail("the status has no version")
        return "version=%s uptime_s=%s ip=%s" % (d.get("version"), d.get("uptime_s", "?"), d.get("ip", "?"))

    rep.step("panel: status", status)

    def settings():
        d = http.get_json("/api/v1/config")
        return "%d sections: %s" % (len(d), ",".join(sorted(d)[:12]))

    rep.step("panel: settings", settings)

    def export():
        a = http.get_json("/api/v1/config/export")
        rep.hide(*secret_values(a))
        b = http.get_json("/api/v1/config/export")
        if a != b:
            raise Fail("two exports in a row differ")
        # what the import would send back: the same document, written and read again, must be the same (nothing is applied here)
        if json.loads(json.dumps(a, sort_keys=True)) != a:
            raise Fail("the export does not survive a write/read round trip")
        box["export"] = a
        return "identical twice, round trip equal, %d sections, nothing applied" % len(a)

    rep.step("panel: settings export is repeatable", export)

    def backup():
        doc = box.get("export")
        if doc is None:
            raise Skip("no export to read")
        sta = doc.get("sta")
        if not isinstance(sta, dict):
            raise Fail("the export has no sta section")
        nets = sta.get("backup", [])
        if not isinstance(nets, list) or any(not isinstance(n, dict) or not n.get("ssid") for n in nets):
            raise Fail("the backup network list is malformed")
        return "%d backup networks saved" % len(nets)

    rep.step("panel: backup Wi-Fi networks", backup)

    def ota():
        d = http.get_json("/api/v1/ota/progress")
        return "state=%s" % d.get("state", "?")

    rep.step("panel: OTA state", ota)
    return box.get("status")


def add_common_args(p, default_port):
    p.add_argument("--host", required=True, help="address of the node")
    p.add_argument("--port", type=int, default=default_port, help="HTTP port of the node (default %d)" % default_port)
    p.add_argument("--https", action="store_true", help="use HTTPS (add --insecure for the node's own certificate)")
    p.add_argument("--insecure", action="store_true", help="do not verify the certificate")
    p.add_argument("--token", default=os.environ.get("EH3D_TOKEN", ""), help="the node's access token (or the variable EH3D_TOKEN); never printed")
    p.add_argument("--user", default=os.environ.get("EH3D_USER", ""), help="panel user (or EH3D_USER)")
    p.add_argument("--password", default=os.environ.get("EH3D_PASSWORD", ""), help="panel password (or EH3D_PASSWORD); never printed")
    p.add_argument("--timeout", type=float, default=5.0, help="seconds to wait for one answer")
    p.add_argument("--out-dir", default=".", help="where the report goes (default: the current folder)")
    p.add_argument("--active", action="store_true", help="also run the tests that move things or start the spindle (asks for confirmation)")
    p.add_argument("--yes", action="store_true", help="answer the confirmation of --active yes (only when you expressly want it)")
# ---- COMMON END ----


LANGS = set("en es it de fr pt zh ja bg hr cs da nl et fi el hu ga lv lt mt pl ro sk sl sv no is uk ru sr sq ca gl eu pt-br fr-ca".split())
FLAGS = {"on", "true", "yes", "1", "off", "false", "no", "0"}
# key -> checker(value) returning an error text or "" (the rules of core/hmi_config.hpp)


def _num(lo, hi):
    def check(v):
        return "" if re.fullmatch(r"[0-9]{1,10}", v) and lo <= int(v) <= hi else "out of range or not a number (%d..%d)" % (lo, hi)
    return check


def _flag(v):
    return "" if v in FLAGS else "expected on or off"


def _notice(v):
    if v in ("off", "default"):
        return ""
    m = re.fullmatch(r"voice:([a-z\-]+):[fm]", v)
    return "" if m and m.group(1) in LANGS else "not off, default or voice:LANG:f|m"


RULES = {
    "language": lambda v: "" if v in LANGS else "not one of the language codes of the screen",
    "theme": lambda v: "",
    "orientation": lambda v: "" if v in ("auto", "landscape", "portrait") else "not auto, landscape or portrait",
    "poll_s": _num(1, 60), "table_width_mm": _num(0, 5000), "table_depth_mm": _num(0, 5000),
    "brightness_percent": _num(10, 100), "idle_dim_s": _num(0, 3600),
    "server_host": lambda v: "", "server_port": _num(1, 65535), "server_machine": lambda v: "",
    "token": lambda v: "" if v == "" or 8 <= len(v) <= 64 else "8 to 64 characters",
    "confirm_moves": _flag, "confirm_timeout_ms": _num(1000, 60000), "jog_max_mm": _num(1, 200),
    "spindle_max_rpm": _num(0, 65535), "spindle_rpm": _num(0, 65535),
    "spindle_over": lambda v: "" if v in ("clip", "refuse") else "not clip or refuse",
    "hw_stop_required": _flag, "link_idle_ms": _num(200, 60000), "stale_after_s": _num(2, 3600),
    "voice_alerts": _flag, "voice_orders": _flag, "voice_confirm": _flag,
    "volume_percent": _num(0, 100), "mic_gain_percent": _num(0, 100),
    "sound_finish": _notice, "sound_tool_change": _notice, "sound_check_ok": _notice, "sound_check_bad": _notice,
}
if HAS_MACHINE_KIND:
    RULES["machine_kind"] = lambda v: "" if v in ("cnc", "laser", "pnp") else "not cnc, laser or pnp"


def parse_cfg(text):
    """Like the firmware: returns (settings, problems). A line that is not '# ...' is a key = value; nothing after the value is a comment."""
    settings, problems = {}, []
    for no, raw in enumerate(text.splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        if "=" not in line:
            problems.append((no, line, "not a key = value line"))
            continue
        key, _, val = line.partition("=")
        key, val = key.strip(), val.strip()
        rule = RULES.get(key)
        if rule is None:
            problems.append((no, key, "unknown key"))
            continue
        why = rule(val)
        if why:
            problems.append((no, key, why + (" (a '#' comment after a value is part of the value)" if "#" in val else "")))
        else:
            settings[key] = val
    return settings, problems


DCF = ("0000001       * SERIAL\nEH3D.000000A  * VERSION\n.\\examples    * FOLDER\n*.NCD         * PATTERN\n1.0000 * SCALE X\n1.0000 * SCALE Y\n"
       "02.50 * OFFSET X\n02.50 * OFFSET Y\n495.00 * CLIPPING X\n325.00 * CLIPPING Y\n")


class ToyServer:
    """The EH3D server as the screen sees it. mode: ok, error (HTTP 500), broken (not JSON), empty (no machines)."""

    def __init__(self, port=DEFAULT_PORT, mode="ok", token=TOY_TOKEN):
        self.mode, self.token, self.calls, self.job_state = mode, token, [], "running"
        toy = self

        class H(http.server.BaseHTTPRequestHandler):
            protocol_version = "HTTP/1.1"

            def log_message(self, *a):
                pass

            def _send(self, code, doc):
                body = doc if isinstance(doc, bytes) else json.dumps(doc).encode()
                self.send_response(code)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(body)))
                self.end_headers()
                self.wfile.write(body)

            def _route(self, method):
                toy.calls.append((method, self.path))
                if toy.token and self.headers.get("Authorization") != "Bearer " + toy.token:
                    return self._send(401, {"error": "unauthorized"})
                if toy.mode == "error":
                    return self._send(500, {"error": "boom"})
                if toy.mode == "broken":
                    return self._send(200, b"{broken")
                p = self.path.split("?")[0]
                if method == "GET" and p == "/api/v1/fleet":
                    if toy.mode == "empty":
                        return self._send(200, {"machines": []})
                    return self._send(200, {"machines": [{"id": "toy", "name": "Toy CCD", "station": "bench", "state": "running", "ready": 1,
                                                          "job": {"id": "j1", "name": "board", "percent": 40, "pause": {"kind": ""}}}]})
                if method == "GET" and p == "/api/v1/machines/toy":
                    return self._send(200, {"id": "toy", "cfg": DCF})
                if method == "GET" and p == "/api/v1/machines/toy/spindle":
                    return self._send(200, {"spindle": "vfd", "guard": "reject", "node_max_rpm": 24000, "active": {"min_rpm": 3000, "max_rpm": 24000}})
                if method == "GET" and p == "/api/v1/machines/toy/head/light":
                    return self._send(200, {"on": False, "level": 0})
                if method == "GET" and p == "/api/v1/jobs/j1":
                    return self._send(200, {"state": toy.job_state, "kind": "drill", "holes_done": 40, "holes_total": 100, "message": ""})
                if method == "POST" and p.startswith("/api/v1/jobs/j1/"):
                    toy.job_state = {"pause": "paused", "resume": "running", "cancel": "cancelled"}.get(p.rsplit("/", 1)[1], toy.job_state)
                    return self._send(200, {"state": toy.job_state})
                self._send(404, {"error": "not_found"})

            def do_GET(self):
                self._route("GET")

            def do_POST(self):
                self.rfile.read(int(self.headers.get("Content-Length") or 0))
                self._route("POST")

        try:
            self.httpd = http.server.ThreadingHTTPServer(("127.0.0.1", port), H)
            self.fallback = False
        except OSError:
            self.httpd = http.server.ThreadingHTTPServer(("127.0.0.1", 0), H)   # 18200 is taken (a real server runs here): another port
            self.fallback = True
        self.httpd.daemon_threads = True
        self.port = self.httpd.server_address[1]
        threading.Thread(target=self.httpd.serve_forever, daemon=True).start()

    def close(self):
        self.httpd.shutdown()
        self.httpd.server_close()


def ask_json(host, port, path, token, timeout=3.0, method="GET"):
    conn = http.client.HTTPConnection(host, port, timeout=timeout)
    try:
        hdr = {"Authorization": "Bearer " + token} if token else {}
        conn.request(method, path, body=b"{}" if method == "POST" else None, headers=hdr)
        r = conn.getresponse()
        data = r.read()
        return r.status, data
    finally:
        conn.close()


def as_doc(status, data, path):
    if status != 200:
        raise Fail("%s answered HTTP %d" % (path, status))
    try:
        doc = json.loads(data.decode())
    except ValueError:
        raise Fail("%s did not answer valid JSON" % path)
    if not isinstance(doc, dict):
        raise Fail("%s did not answer a JSON object" % path)
    return doc


BLANKS = " " + chr(9) + chr(13)


def cfg_value(raw):
    """A data line of a configuration file is 'value   * comment': the comment starts at a * that follows a blank and is followed by a blank (or ends the line)."""
    prev_blank = True
    for i, c in enumerate(raw):
        if c == "*" and prev_blank and (i + 1 >= len(raw) or raw[i + 1] in BLANKS):
            raw = raw[:i]
            break
        prev_blank = c in BLANKS
    return raw.strip()


def table_of(cfg_text):
    """The table parser of the screen (parse_machine_table): the data lines are serial, version, folder, pattern, then scale, offset and course."""
    data = []
    for line in cfg_text.splitlines():
        v = cfg_value(line)
        if v:
            data.append(v)
        if len(data) >= 12:
            break
    if len(data) < 10:
        return None
    try:
        ox, oy, cx, cy = (float(data[4 + i]) for i in (2, 3, 4, 5))
    except ValueError:
        return None
    return (cx - ox, cy - oy) if cx - ox > 0 and cy - oy > 0 else None


def check_server(rep, host, port, token, machine, label, writes_ok=False):
    """The questions the screen asks, answered by a toy or by a real server. GET only unless writes_ok (the toy)."""
    box = {}

    def fleet():
        d = as_doc(*ask_json(host, port, "/api/v1/fleet", token), "/api/v1/fleet")
        ms = d.get("machines")
        if not isinstance(ms, list):
            raise Fail("fleet has no machines list")
        if not ms:
            raise Skip("the fleet is empty: nothing for the screen to show")
        if any(not isinstance(m, dict) or not m.get("id") for m in ms):
            raise Fail("a machine of the fleet has no id")
        box["machines"] = ms
        return "%d machines: %s" % (len(ms), ",".join(str(m["id"]) for m in ms[:6]))

    rep.step("%s: fleet" % label, fleet)

    def pick():
        if machine:
            return machine
        ms = box.get("machines")
        return ms[0]["id"] if ms else ""

    def machine_table():
        mid = pick()
        if not mid:
            raise Skip("no machine to ask about")
        d = as_doc(*ask_json(host, port, "/api/v1/machines/%s" % mid, token), "machines/" + mid)
        size = table_of(d.get("cfg", ""))
        if size is None:
            raise Fail("the machine's configuration has no readable table (the screen would not warn about a board that does not fit)")
        return "table %.1f x %.1f mm" % size

    rep.step("%s: machine table" % label, machine_table)

    def spindle():
        mid = pick()
        if not mid:
            raise Skip("no machine to ask about")
        d = as_doc(*ask_json(host, port, "/api/v1/machines/%s/spindle" % mid, token), "spindle")
        act = d.get("active")
        if not isinstance(act, dict) or not act.get("max_rpm"):
            raise Fail("the spindle answer has no active.max_rpm")
        if act.get("min_rpm", 0) > act["max_rpm"]:
            raise Fail("min_rpm is above max_rpm")
        return "%s %s..%s rpm, node max %s, guard %s" % (d.get("spindle", "?"), act.get("min_rpm"), act["max_rpm"], d.get("node_max_rpm", "?"), d.get("guard", "?"))

    rep.step("%s: spindle limits" % label, spindle)

    def job():
        ms = box.get("machines") or []
        jid = next((m["job"]["id"] for m in ms if isinstance(m.get("job"), dict) and m["job"].get("id")), "")
        if not jid:
            raise Skip("no running job to read")
        d = as_doc(*ask_json(host, port, "/api/v1/jobs/%s" % jid, token), "job")
        if not d.get("state"):
            raise Fail("the job report has no state")
        return "state=%s holes %s/%s" % (d["state"], d.get("holes_done", 0), d.get("holes_total", 0))

    rep.step("%s: job report" % label, job)
    rep.step("%s: wrong token is refused" % label, lambda: _refused(host, port, token))
    if writes_ok:
        def controls():
            for verb, want in (("pause", "paused"), ("resume", "running")):
                d = as_doc(*ask_json(host, port, "/api/v1/jobs/j1/" + verb, token, method="POST"), verb)
                if d.get("state") != want:
                    raise Fail("%s gave state %r, expected %r" % (verb, d.get("state"), want))
            return "pause and resume answered as the screen expects (toy server only)"

        rep.step("%s: pause and resume" % label, controls)


def _refused(host, port, token):
    if not token:
        raise Skip("no token given: the server may be open")
    code, _ = ask_json(host, port, "/api/v1/fleet", "wrong-" + (token or "token") + "-x")
    if code != 401:
        raise Fail("a wrong token got HTTP %d instead of 401" % code)
    return "HTTP 401"


def main(argv=None):
    here = os.path.dirname(os.path.abspath(__file__))
    p = argparse.ArgumentParser(description="Self-test of the EH3D-NODE-HMI-GUI screen settings and of the server it talks to.")
    p.add_argument("--cfg", default=os.path.join(os.path.dirname(here), "hmi.cfg.example"), help="settings file to check (default: hmi.cfg.example)")
    p.add_argument("--server", default="", help="HOST:PORT of a real EH3D server to ask (GET only)")
    p.add_argument("--token", default=os.environ.get("EH3D_TOKEN", ""), help="token of the real server (or EH3D_TOKEN); never printed")
    p.add_argument("--machine", default="", help="machine id on the real server")
    p.add_argument("--out-dir", default=".", help="where the report goes")
    p.add_argument("--port", type=int, default=DEFAULT_PORT, help="port of the toy server (default 18200)")
    args = p.parse_args(argv)
    rep = Report(NODE, args.server or "toy server")
    rep.hide(args.token)

    def cfg():
        with open(args.cfg, encoding="utf-8") as f:
            settings, problems = parse_cfg(f.read())
        if problems:
            raise Fail("%d lines the screen would reject: %s" % (len(problems), "; ".join("line %d %s: %s" % pr for pr in problems[:6])))
        return "%d settings, all accepted" % len(settings)

    rep.step("hmi.cfg: every line is accepted by the screen", cfg)

    def default_port():
        with open(args.cfg, encoding="utf-8") as f:
            settings, _ = parse_cfg(f.read())
        port = settings.get("server_port")
        if port is None:
            raise Skip("server_port is not set: the screen uses its default")
        if int(port) != DEFAULT_PORT:
            raise Fail("server_port is %s, the EH3D server listens on %d" % (port, DEFAULT_PORT))
        return "server_port = %s" % port

    rep.step("hmi.cfg: server port is the server's", default_port)
    toy = ToyServer(args.port)
    try:
        rep.add(OK, "toy server", "listening on 127.0.0.1:%d%s" % (toy.port, " (18200 was busy)" if toy.fallback else ""))
        check_server(rep, "127.0.0.1", toy.port, TOY_TOKEN, "toy", "toy server", writes_ok=True)
    finally:
        toy.close()
    if args.server:
        host, _, port = args.server.rpartition(":")
        if not host or not port.isdigit():
            rep.add(FAIL, "real server", "--server must be HOST:PORT")
        else:
            check_server(rep, host, int(port), args.token, args.machine, "real server")
    else:
        rep.add(SKIP, "real server", "not requested (add --server HOST:PORT --token TOKEN)")
    path = rep.write(args.out_dir)
    print("report: " + path)
    return 1 if rep.counts[FAIL] else 0


if __name__ == "__main__":
    sys.exit(main())
