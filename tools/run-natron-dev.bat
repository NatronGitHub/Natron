@echo off
REM Launch the locally built Natron (feat/ai-chat-panel) with the environment it
REM needs. Double-click this, or run it from cmd.
REM
REM The build links against MSYS2/mingw64 DLLs, so mingw64\bin must be on PATH or
REM Windows will refuse to start the executable with a missing-DLL error.

setlocal

set "MSYS2_ROOT=C:\msys64"
set "REPO_ROOT=%~dp0..\.."
set "NATRON_DIR=%~dp0.."

if not exist "%MSYS2_ROOT%\mingw64\bin" (
    echo ERROR: mingw64 not found at %MSYS2_ROOT%\mingw64
    echo Edit MSYS2_ROOT at the top of this script if MSYS2 lives elsewhere.
    pause
    exit /b 1
)

if not exist "%NATRON_DIR%\build\App\Natron.exe" (
    echo ERROR: Natron.exe not built yet.
    echo Build it first - see AI-BUILD.md.
    pause
    exit /b 1
)

REM mingw64 DLLs first, otherwise the exe will not load.
set "PATH=%MSYS2_ROOT%\mingw64\bin;%PATH%"

REM Colour configs. Without OCIO, colour-managed nodes complain on startup.
if exist "%REPO_ROOT%\OpenColorIO-Configs\blender\config.ocio" (
    set "OCIO=%REPO_ROOT%\OpenColorIO-Configs\blender\config.ocio"
)

REM OpenFX plug-ins, if any were built or installed separately. A stock source
REM build has none, which means only Natron's built-in nodes (Dot, Backdrop,
REM Roto, Viewer, Group, ...) are available - no Grade, Blur, Merge and so on.
if exist "%REPO_ROOT%\Plugins" (
    set "OFX_PLUGIN_PATH=%REPO_ROOT%\Plugins"
)

echo Starting Natron...
echo   OCIO             = %OCIO%
echo   OFX_PLUGIN_PATH  = %OFX_PLUGIN_PATH%
echo.

start "" "%NATRON_DIR%\build\App\Natron.exe" %*

endlocal
