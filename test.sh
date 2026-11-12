#!/usr/bin/env bash
# EH3D-NODE-HMI-GUI test launcher: the tests, shown in a live panel; the version is never changed. GPL-3.0-or-later.
# PRETTY_RUN_BEGIN
if [ -z "${PRETTY_RUN:-}" ] && [ -f "$(dirname "$0")/tools/pretty_run.py" ] && command -v python3 >/dev/null 2>&1; then
    NOPAUSE=1 PRETTY_RUN=1 exec python3 "$(dirname "$0")/tools/pretty_run.py" --wrap --mode "${PRETTY_MODE:-test}" -- bash "$0" "$@"
fi
# PRETTY_RUN_END
cd "$(dirname "$0")" || exit 1
# The same steps as .github/workflows/ci.yml: the repository checks, the tests of the tools, the pictures and the languages, and the host tests of the core.
if command -v python3 >/dev/null 2>&1; then PY=python3; else PY="py -3"; fi
$PY tools/ci_validate.py || exit 1
$PY -m unittest discover -s tools -p "test_*.py" || exit 1
if [ -f tools/make_locales.py ]; then $PY tools/make_locales.py --check || exit 1; fi
$PY tools/make_architecture.py --check || exit 1
$PY tools/make_structure.py --check --readmes || exit 1
cmake -S tests -B build/host || exit 1
cmake --build build/host || exit 1
ctest --test-dir build/host -C Debug --output-on-failure
