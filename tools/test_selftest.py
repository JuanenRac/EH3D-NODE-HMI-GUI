"""Tests of tools/selftest.py: the settings check and the toy server the screen talks to."""
import contextlib
import io
import os
import socket
import tempfile
import unittest

import selftest as st

HERE = os.path.dirname(os.path.abspath(__file__))
EXAMPLE = os.path.join(os.path.dirname(HERE), "hmi.cfg.example")


def run(argv):
    out = io.StringIO()
    with tempfile.TemporaryDirectory() as d, contextlib.redirect_stdout(out):
        code = st.main(["--port", "0", "--out-dir", d] + argv)
        files = [f for f in os.listdir(d) if f.startswith("selftest_")]
        report = ""
        if files:
            with open(os.path.join(d, files[0]), encoding="utf-8") as f:
                report = f.read()
    return code, out.getvalue(), report


def cfg_file(text):
    f = tempfile.NamedTemporaryFile("w", suffix=".cfg", delete=False, encoding="utf-8")
    f.write(text)
    f.close()
    return f.name


class Config(unittest.TestCase):
    def test_the_example_is_accepted(self):
        with open(EXAMPLE, encoding="utf-8") as f:
            settings, problems = st.parse_cfg(f.read())
        self.assertEqual(problems, [])
        self.assertEqual(settings["server_port"], "18200")

    def test_comment_after_a_value_is_rejected_like_the_firmware(self):
        settings, problems = st.parse_cfg("brightness_percent = 80   # percent\n")
        self.assertEqual([p[1] for p in problems], ["brightness_percent"])
        self.assertNotIn("brightness_percent", settings)

    def test_bad_lines(self):
        text = "nonsense\nunknown_key = 1\nbrightness_percent = 5\ntoken = short\nspindle_over = maybe\nsound_finish = voice:xx:f\nlanguage = es\n"
        settings, problems = st.parse_cfg(text)
        self.assertEqual([p[0] for p in problems], [1, 2, 3, 4, 5, 6])
        self.assertEqual(settings, {"language": "es"})

    def test_notices_and_flags(self):
        settings, problems = st.parse_cfg("sound_finish = voice:es:f\nsound_check_ok = default\nconfirm_moves = off\nspindle_max_rpm = 0\n")
        self.assertEqual(problems, [])
        self.assertEqual(len(settings), 4)


class Run(unittest.TestCase):
    def test_default_run_passes(self):
        code, out, report = run([])
        self.assertEqual(code, 0, report)
        for text in ("every line is accepted by the screen | ", "toy server: fleet", "toy server: machine table | table 492.5 x 322.5 mm", "toy server: spindle limits",
                     "toy server: wrong token is refused | HTTP 401", "real server | not requested"):
            self.assertIn(text, report)

    def test_bad_config_fails(self):
        path = cfg_file("brightness_percent = 80 # x\nserver_port = 8080\n")
        try:
            code, out, report = run(["--cfg", path])
            self.assertEqual(code, 1)
            self.assertIn("FALLO    hmi.cfg: every line is accepted", report)
            self.assertIn("FALLO    hmi.cfg: server port is the server's | server_port is 8080", report)
        finally:
            os.unlink(path)

    def test_missing_config_fails_without_crashing(self):
        code, out, report = run(["--cfg", os.path.join(HERE, "no_such_file.cfg")])
        self.assertEqual(code, 1)
        self.assertIn("FALLO    hmi.cfg", report)

    def test_real_server_that_is_a_good_toy_passes_and_gets_only_reads(self):
        real = st.ToyServer(0, token="real-token-0123456789")
        try:
            code, out, report = run(["--server", "127.0.0.1:%d" % real.port, "--token", "real-token-0123456789", "--machine", "toy"])
            self.assertEqual(code, 0, report)
            self.assertIn("OK       real server: fleet", report)
            self.assertEqual([c for c in real.calls if c[0] != "GET"], [])     # nothing was started, paused or cancelled
            self.assertNotIn("real-token-0123456789", out + report)
        finally:
            real.close()

    def test_real_server_that_errors_or_sends_broken_json(self):
        for mode, word in (("error", "HTTP 500"), ("broken", "valid JSON")):
            real = st.ToyServer(0, mode=mode, token="")
            try:
                code, out, report = run(["--server", "127.0.0.1:%d" % real.port])
                self.assertEqual(code, 1, mode)
                self.assertIn(word, report)
            finally:
                real.close()

    def test_real_server_without_machines(self):
        real = st.ToyServer(0, mode="empty", token="")
        try:
            code, out, report = run(["--server", "127.0.0.1:%d" % real.port])
            self.assertIn("OMITIDO  real server: fleet", report)
        finally:
            real.close()

    def test_real_server_that_does_not_answer(self):
        s = socket.socket(); s.bind(("127.0.0.1", 0)); port = s.getsockname()[1]; s.close()
        code, out, report = run(["--server", "127.0.0.1:%d" % port])
        self.assertEqual(code, 1)
        self.assertIn("FALLO    real server: fleet", report)

    def test_bad_server_argument(self):
        code, out, report = run(["--server", "nonsense"])
        self.assertEqual(code, 1)
        self.assertIn("--server must be HOST:PORT", report)

    def test_open_real_server_with_a_token_given_is_not_a_failure_of_the_token_test(self):
        real = st.ToyServer(0, token="")
        try:
            code, out, report = run(["--server", "127.0.0.1:%d" % real.port, "--machine", "toy"])
            self.assertIn("OMITIDO  real server: wrong token is refused", report)
        finally:
            real.close()


if __name__ == "__main__":
    unittest.main()
