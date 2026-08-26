#!/bin/bash
#
# Build a self-contained, redistributable Natron-AI folder from the local build.
#
# Run this from the MSYS2 MINGW64 shell:
#     ./tools/package-natron-ai.sh
#
# Produces  dist/NatronAI/  and  dist/NatronAI-win64.zip
#
# The layout is dictated by Global/PythonUtils.cpp:81-89, which computes
# PYTHONHOME as <bin>/.. and then looks for lib/pythonXY.zip, lib/pythonX.Y,
# lib/pythonX.Y/lib-dynload and ../Plugins. Get that wrong and Natron starts but
# has no working Python, which breaks PyPlugs and the Script Editor.

set -euo pipefail

NATRON_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REPO_ROOT="$(cd "${NATRON_DIR}/.." && pwd)"
BUILD_DIR="${NATRON_DIR}/build"
DIST="${NATRON_DIR}/dist/NatronAI"
MINGW="/mingw64"

PYVER="$("${MINGW}/bin/python3.exe" -c 'import sys;print("%d.%d"%sys.version_info[:2])')"
PYVERNODOT="${PYVER//./}"

echo "==> Natron   : ${NATRON_DIR}"
echo "==> Python   : ${PYVER}"
echo "==> Output   : ${DIST}"

if [[ ! -f "${BUILD_DIR}/App/Natron.exe" ]]; then
    echo "ERROR: ${BUILD_DIR}/App/Natron.exe not found. Build first (see AI-BUILD.md)."
    exit 1
fi

rm -rf "${DIST}"
mkdir -p "${DIST}/bin" "${DIST}/lib" "${DIST}/Plugins/OFX/Natron" "${DIST}/Resources"

# ---------------------------------------------------------------------------
# DLL resolution.
#
# ldd cannot be used here: it reports "Exec format error" on a .ofx (it is a DLL
# with a non-standard extension), so an ldd-based sweep silently collects
# nothing and the plug-in then fails to load at runtime with no diagnostic.
# objdump reads the import table regardless of extension.
#
# Dependencies also live in several prefixes, not just /mingw64/bin: the Natron
# pacman repo installs ffmpeg and libraw into their own trees.
# ---------------------------------------------------------------------------
DLL_SEARCH_DIRS=(
    "${MINGW}/bin"
    "${MINGW}/ffmpeg-gpl2/bin"
    "${MINGW}/libraw-gpl2/bin"
    "${MINGW}/osmesa/bin"
)

_resolved_dlls=""

resolve_deps() {
    # $1 = binary to inspect. Copies every resolvable import into bin/, then
    # recurses into each newly copied DLL.
    local target="$1"
    local name src

    objdump -p "${target}" 2>/dev/null | grep 'DLL Name:' | awk '{print $3}' | sort -u |
    while read -r name; do
        [[ -z "${name}" ]] && continue
        # Already shipped?
        [[ -f "${DIST}/bin/${name}" ]] && continue
        src=""
        for d in "${DLL_SEARCH_DIRS[@]}"; do
            if [[ -f "${d}/${name}" ]]; then
                src="${d}/${name}"
                break
            fi
        done
        # Not found in our prefixes => a system DLL (KERNEL32, msvcrt, ...).
        [[ -z "${src}" ]] && continue
        cp -n "${src}" "${DIST}/bin/" 2>/dev/null || true
        resolve_deps "${DIST}/bin/${name}"
    done
}

# ---------------------------------------------------------------------------
# 1. Executables, stripped. RelWithDebInfo leaves ~700 MB of debug info in
#    Natron.exe; stripping takes it to ~16 MB and changes nothing at runtime.
# ---------------------------------------------------------------------------
echo "==> Copying executables"
for exe in App/Natron.exe Renderer/NatronRenderer.exe PythonBin/natron-python.exe; do
    if [[ -f "${BUILD_DIR}/${exe}" ]]; then
        cp "${BUILD_DIR}/${exe}" "${DIST}/bin/"
        strip "${DIST}/bin/$(basename "${exe}")" || true
    fi
done

# ---------------------------------------------------------------------------
# 2. Transitive DLL closure. ldd resolves recursively, so one pass over each
#    executable is enough; filter to mingw64 so we never ship system DLLs.
# ---------------------------------------------------------------------------
echo "==> Resolving DLLs"
for exe in "${DIST}"/bin/*.exe; do
    resolve_deps "${exe}"
done
echo "    $(find "${DIST}/bin" -name '*.dll' | wc -l) DLLs"

# ---------------------------------------------------------------------------
# 3. Qt plugins. Qt loads these by path at runtime, so ldd cannot see them.
#    Without the platforms/ plugin Qt aborts with "could not find or load the Qt
#    platform plugin windows" and the app never opens a window.
# ---------------------------------------------------------------------------
echo "==> Copying Qt plugins"
for grp in platforms imageformats styles iconengines platforminputcontexts; do
    if [[ -d "${MINGW}/share/qt5/plugins/${grp}" ]]; then
        mkdir -p "${DIST}/bin/${grp}"
        cp -r "${MINGW}/share/qt5/plugins/${grp}/"*.dll "${DIST}/bin/${grp}/" 2>/dev/null || true
    fi
done
# Qt plugins pull in their own DLLs; resolve those too.
while read -r qtplugin; do
    resolve_deps "${qtplugin}"
done < <(find "${DIST}/bin" -mindepth 2 -name '*.dll')

# ---------------------------------------------------------------------------
# 4. Python runtime. Natron embeds CPython; without the stdlib the Script
#    Editor, PyPlugs and every startup script are dead.
# ---------------------------------------------------------------------------
echo "==> Copying Python ${PYVER} stdlib"
mkdir -p "${DIST}/lib/python${PYVER}"
cp -r "${MINGW}/lib/python${PYVER}/." "${DIST}/lib/python${PYVER}/"
rm -rf "${DIST}/lib/python${PYVER}/test" \
       "${DIST}/lib/python${PYVER}/tests" \
       "${DIST}/lib/python${PYVER}/idlelib" \
       "${DIST}/lib/python${PYVER}/tkinter" \
       "${DIST}/lib/python${PYVER}/turtledemo"
find "${DIST}/lib/python${PYVER}" -name '__pycache__' -type d -exec rm -rf {} + 2>/dev/null || true

# ---------------------------------------------------------------------------
# 5. OpenFX plug-ins. Without these Natron has only its built-in nodes -- no
#    Grade, Merge, Transform, Read or Write -- which is not a usable compositor.
# ---------------------------------------------------------------------------
echo "==> Copying OpenFX plug-ins"
found_ofx=0
for bundle in "${REPO_ROOT}"/ofx-build/*/*/*/[A-Za-z]*.ofx.bundle; do
    if [[ -d "${bundle}" ]]; then
        cp -r "${bundle}" "${DIST}/Plugins/OFX/Natron/"
        echo "    $(basename "${bundle}")"
        found_ofx=1
    fi
done
if [[ "${found_ofx}" == "0" ]]; then
    echo "    WARNING: no .ofx.bundle found -- the package will have built-in nodes only."
fi

# OFX bundles link against mingw64 DLLs too (OpenImageIO, ffmpeg, libraw, ...).
while read -r ofx; do
    echo "    deps: $(basename "${ofx}")"
    resolve_deps "${ofx}"
done < <(find "${DIST}/Plugins" -name '*.ofx')

# ---------------------------------------------------------------------------
# 6. Resources: colour configs, PyPlugs, fonts.
# ---------------------------------------------------------------------------
echo "==> Copying resources"
if [[ -d "${REPO_ROOT}/OpenColorIO-Configs" ]]; then
    cp -r "${REPO_ROOT}/OpenColorIO-Configs" "${DIST}/Resources/"
fi
if [[ -d "${NATRON_DIR}/Gui/Resources/PyPlugs" ]]; then
    mkdir -p "${DIST}/Plugins/PyPlugs"
    cp -r "${NATRON_DIR}/Gui/Resources/PyPlugs/." "${DIST}/Plugins/PyPlugs/"
fi
for f in "${NATRON_DIR}/Gui/Resources/Fonts"; do
    [[ -d "${f}" ]] && cp -r "${f}" "${DIST}/Resources/" || true
done

# Fontconfig configuration. AppManager.cpp:339-344 points FONTCONFIG_PATH at
# <bin>/../Resources/etc/fonts and prints a warning when it is absent; the
# comment there notes it is "required by plugins using fontconfig", which is
# every text plug-in. Without it font lookup falls back to defaults and text
# renders differently -- or not at all -- on the target machine.
if [[ -d "${MINGW}/etc/fonts" ]]; then
    mkdir -p "${DIST}/Resources/etc"
    cp -r "${MINGW}/etc/fonts" "${DIST}/Resources/etc/"
    echo "    fontconfig"
fi

# ---------------------------------------------------------------------------
# 7. Launcher. Sets OCIO and the OFX path relative to the folder, so the whole
#    thing is movable and needs no installation.
# ---------------------------------------------------------------------------
cat > "${DIST}/Natron-AI.bat" <<'LAUNCHER'
@echo off
REM Portable Natron with the AI Assistant panel. No installation needed --
REM unzip anywhere and run this file.
setlocal
set "HERE=%~dp0"
set "PATH=%HERE%bin;%PATH%"
set "OFX_PLUGIN_PATH=%HERE%Plugins\OFX\Natron"
if exist "%HERE%Resources\OpenColorIO-Configs\blender\config.ocio" (
    set "OCIO=%HERE%Resources\OpenColorIO-Configs\blender\config.ocio"
)
start "" "%HERE%bin\Natron.exe" %*
endlocal
LAUNCHER

cat > "${DIST}/README.txt" <<'READMETXT'
Natron with the AI Assistant panel (portable build)
===================================================

Run Natron-AI.bat. Nothing is installed and nothing is written outside this
folder except Natron's own settings in %APPDATA%. It runs alongside an existing
Natron install without interfering with it.

Using the AI Assistant
----------------------
1. Right-click any panel's tab bar -> "AI Assistant here".
2. The panel prints the local MCP endpoint and a session token when it opens.
3. Type a request and press Enter (Shift+Enter for a newline).

The panel drives an external agent CLI ("claude"), which must be installed
separately and signed in. Natron never sees or stores your provider credentials:
the CLI authenticates with your own subscription and Natron only starts it as a
child process. If "claude" works in a terminal, it works here.

Anything destructive -- deleting a node, overwriting a file -- raises a native
confirmation dialog inside Natron before it happens. Everything the assistant
does in one turn collapses into a single Ctrl+Z.

Note: the conversation and the project content are sent to the CLI's provider.

Included plug-ins
-----------------
Misc.ofx.bundle  - Grade, Merge, Transform, ChromaKeyer, Reformat, and ~130 more
IO.ofx.bundle    - Read/Write for EXR, PNG, JPEG, TIFF, DPX, video, and OCIO

Not included: GMIC and Arena (openfx-gmic, openfx-arena), and the CImg nodes.
READMETXT

# ---------------------------------------------------------------------------
# 8. Zip it.
# ---------------------------------------------------------------------------
# ---------------------------------------------------------------------------
# 7b. Guard: every plug-in a bundled PyPlug instantiates must actually ship.
#
# This is how the Glow failure escaped -- the CImg plug-ins were skipped at
# build time, so Glow.py's createNode("net.sf.cimg.CImgBloom") returned None and
# the user got "'NoneType' object has no attribute 'setScriptName'". Natron
# lists the PyPlug in its menu regardless, so nothing warns until someone
# clicks it.
# ---------------------------------------------------------------------------
echo "==> Checking PyPlug dependencies"
if [[ -d "${DIST}/Plugins/PyPlugs" ]]; then
    grep -ho 'createNode("[^"]*"' "${DIST}/Plugins/PyPlugs/"*.py 2>/dev/null \
        | sed 's/createNode("//; s/"//' | sort -u > "${DIST}/.needed_ids" || true

    cat > "${DIST}/.checkdeps.py" <<'CHECKDEPS'
import os, sys
here = os.path.dirname(os.path.abspath(sys.argv[0])) if sys.argv else "."
needed = [l.strip() for l in open(NEEDED_FILE) if l.strip()]
have = set(NatronEngine.natron.getPluginIDs())
missing = [n for n in needed if n not in have]
sys.stderr.write("PYPLUG_DEPS %d needed, %d missing\n" % (len(needed), len(missing)))
for m in missing:
    sys.stderr.write("PYPLUG_MISSING %s\n" % m)
os._exit(0)
CHECKDEPS
    # Inject the path rather than relying on __file__, which is undefined for a
    # script Natron exec()s rather than imports.
    sed -i "s|NEEDED_FILE|r'$(cygpath -m "${DIST}/.needed_ids")'|" "${DIST}/.checkdeps.py"

    dep_out="$( "${DIST}/bin/NatronRenderer.exe" -t "${DIST}/.checkdeps.py" 2>&1 || true )"
    echo "${dep_out}" | grep 'PYPLUG_DEPS' || true
    if echo "${dep_out}" | grep -q 'PYPLUG_MISSING'; then
        echo "${dep_out}" | grep 'PYPLUG_MISSING'
        echo "ERROR: bundled PyPlugs reference plug-ins that are not in the package."
        rm -f "${DIST}/.checkdeps.py" "${DIST}/.needed_ids"
        exit 1
    fi
    rm -f "${DIST}/.checkdeps.py" "${DIST}/.needed_ids"
fi

echo "==> Zipping"
( cd "${NATRON_DIR}/dist" && rm -f NatronAI-win64.zip && zip -qr9 NatronAI-win64.zip NatronAI )

echo
echo "==> Done"
du -sh "${DIST}" "${NATRON_DIR}/dist/NatronAI-win64.zip"
