@echo off
rem PRETTY_RUN_BEGIN
set "NOPAUSE=1"
if not defined PRETTY_MODE set "PRETTY_MODE=build"
set "PRETTY_PY=python"
where py >nul 2>&1 && set "PRETTY_PY=py -3"
if not defined PRETTY_RUN if exist "%~dp0tools\pretty_run.py" (
    set "PRETTY_RUN=1"
    %PRETTY_PY% "%~dp0tools\pretty_run.py" --wrap --mode %PRETTY_MODE% -- cmd /d /c "%~f0" %*
    exit /b
)
rem PRETTY_RUN_END
rem EH3D-NODE-HMI-GUI: builds the host tests of the core. The firmware itself is built with tools\build_node.sh (WSL or Linux, Docker).
rem   build.bat            runs the host tests
setlocal
cd /d "%~dp0"
cmake -S tests -B build\host
if errorlevel 1 goto :end
cmake --build build\host --config Debug
if errorlevel 1 goto :end
ctest --test-dir build\host -C Debug --output-on-failure
:end
if not defined NOPAUSE pause
endlocal
