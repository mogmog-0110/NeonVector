@echo off
echo Reconfiguring CMake...
cd /d "%~dp0"
cmake -B build
if %ERRORLEVEL% EQU 0 (
    echo.
    echo ✓ Reconfiguration complete!
    echo.
    echo You can now:
    echo 1. Reopen the folder in Visual Studio
    echo 2. Or run: cmake --build build --config Debug
    pause
) else (
    echo.
    echo ✗ Reconfiguration failed
    pause
)