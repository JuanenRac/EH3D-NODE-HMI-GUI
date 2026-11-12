@echo off
rem EH3D-NODE-HMI-GUI test launcher: the tests, shown in a live panel; the version is never changed. GPL-3.0-or-later.
rem PRETTY_RUN_BEGIN
set "NOPAUSE=1"
if not defined PRETTY_MODE set "PRETTY_MODE=test"
set "PRETTY_PY=python"
where py >nul 2>&1 && set "PRETTY_PY=py -3"
if not defined PRETTY_RUN if exist "%~dp0tools\pretty_run.py" (
    set "PRETTY_RUN=1"
    %PRETTY_PY% "%~dp0tools\pretty_run.py" --wrap --mode %PRETTY_MODE% -- cmd /d /c "%~f0" %*
    exit /b
)
rem PRETTY_RUN_END
cd /d "%~dp0"
rem The same steps as .github/workflows/ci.yml: the repository checks, the tests of the tools, the pictures and the languages, and the host tests of the core.
py -3 tools/ci_validate.py
if errorlevel 1 exit /b 1
py -3 -m unittest discover -s tools -p "test_*.py"
if errorlevel 1 exit /b 1
if exist tools\make_locales.py (
    py -3 tools/make_locales.py --check
    if errorlevel 1 exit /b 1
)
py -3 tools/make_architecture.py --check
if errorlevel 1 exit /b 1
py -3 tools/make_structure.py --check --readmes
if errorlevel 1 exit /b 1
cmake -S tests -B build\host
if errorlevel 1 exit /b 1
cmake --build build\host --config Debug
if errorlevel 1 exit /b 1
ctest --test-dir build\host -C Debug --output-on-failure
exit /b %ERRORLEVEL%
