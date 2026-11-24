#!/bin/bash
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2016 INRIA and Alexandre Gauthier
#
# Natron is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Natron is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Natron.  If not, see <http://www.gnu.org/licenses/gpl-2.0.html>
# ***** END LICENSE BLOCK *****
#
# Build packages and installer for Windows
#
# Options:
# DISABLE_BREAKPAD=1: Disable automatic crash report

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

echo "*** Windows installer..."

source common.sh
source manageBuildOptions.sh
source manageLog.sh

pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd "$@" > /dev/null
}

# List the DLLs to bundle, from the import tables of the binaries (updateBuildOptions first, for
# TMP_BINARIES_PATH). The FFmpeg, LibRaw and (COMMERCIAL) libheif and libtiff of the licence come
# first, as on PATH at run time (common.sh). An empty LIBHEIF_PATH or LIBTIFF_PATH adds no entry.
updateBuildOptions
env MINGW_PREFIX="$SDK_HOME" TMP_BINARIES_PATH="$TMP_BINARIES_PATH" PYVER="$PYVER" QT_VERSION_MAJOR="${QT_VERSION_MAJOR}" NATRON_LICENSE="$NATRON_LICENSE" \
    FFMPEG_PATH="$FFMPEG_PATH" DLL_SEARCH_DIRS="$FFMPEG_PATH/bin:$LIBRAW_PATH/bin:${LIBHEIF_PATH:+$LIBHEIF_PATH/bin:}${LIBTIFF_PATH:+$LIBTIFF_PATH/bin:}$SDK_HOME/bin" ./genDllVersions.sh

source dllVersions.sh

# Pretty architecture/platform identifier (used for breakpad symbol files)
if [ "$ARCH" = "x86_64" ]; then
    PKGOS_BITS="${PKGOS}-x86_${BITS}bit"
else
    PKGOS_BITS="${PKGOS}-${WIN_ARCH}"
fi

if [ -z "${NATRON_BUILD_CONFIG:-}" ]; then
    echo "NATRON_BUILD_CONFIG empty"
    exit 1
fi

DUMP_SYMS="$SDK_HOME/bin/natron_dump_syms_dwarf.exe"

# Qt modules that are GPL-3 only (no LGPL), left out of COMMERCIAL builds with their PySide modules
# (QtCharts.pyd, ...) and the Qt plug-ins that link them; genDllVersions.sh doesn't bundle their
# DLLs. check-commercial-bundle.sh (is_gpl_lib) rejects the same modules. GPL builds keep them.
QT_GPL_LIBS=( Charts DataVisualization NetworkAuth Designer VirtualKeyboard Quick3D Bodymovin )

dump_syms_safe() {
    if [ $# != 2 ]; then
        echo "Usage: dump_syms_safe <bin> <output>"
        exit 1
    fi
    dumpfail=0
    echo "*** dumping syms from $1"
    "$DUMP_SYMS" "$1" > "$2" || dumpfail=1
    if [ $dumpfail = 1 ]; then
        echo "Error: $DUMP_SYMS failed on $1"
        rm "$2"
    fi
    echo "*** dumping syms from $1... done!"
}


if [ -d "${BUILD_ARCHIVE_DIRECTORY}" ]; then
    rm -rf "${BUILD_ARCHIVE_DIRECTORY}"
fi

mkdir -p "${BUILD_ARCHIVE_DIRECTORY}"
# the list of bundled DLLs, for comparing installers
cp dllVersions.txt "${BUILD_ARCHIVE_DIRECTORY}/${INSTALLER_BASENAME}-dlls.txt"
if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    mkdir "${BUILD_ARCHIVE_DIRECTORY}/symbols"
fi

# If we are in DEBUG_SYMBOLS mode there might already be deployed installer, remove them
if [ ! -z "$DEBUG_SCRIPTS" ]; then
    (cd "${TMP_BINARIES_PATH}" ; find . -type d -name 'Natron-*' -exec rm -rf {} \;) || true
fi


mkdir -p "${TMP_PORTABLE_DIR}"


# tag symbols we want to keep with 'release'
VERSION_TAG="${CURRENT_DATE}"
if [ "${NATRON_BUILD_CONFIG}" = "RELEASE" ] || [ "${NATRON_BUILD_CONFIG}" = "STABLE" ]; then
    BPAD_TAG="-release"
    VERSION_TAG="$NATRON_VERSION_FULL"
fi

# Everything goes into the portable tree ${TMP_PORTABLE_DIR}, which is zipped with Inno Setup's
# Setup.exe: Setup.exe installs from the unzipped tree (Natron.iss, "external" files). The Qt
# Installer Framework installers and their packages/ tree are retired (#1133).

# make sure we have mt (to embed the plug-in manifests) and the Inno Setup installer

MT_BIN=mt.exe
if ! which ${MT_BIN}; then
    # Add Windows SDK to path so that mt.exe is available: the one for the build machine (the x64
    # one also runs emulated on ARM64).
    WIN_SDK_MAJOR_VERSION=10
    WIN_SDK_BASE_PATH="/c/Program Files (x86)/Windows Kits/${WIN_SDK_MAJOR_VERSION}/bin"
    MT_HOST_ARCH=x64
    if [ "$ARCH" = "aarch64" ]; then
        MT_HOST_ARCH=arm64
    fi
    MT_BIN=$(find "${WIN_SDK_BASE_PATH}" -name mt.exe | grep "/${MT_HOST_ARCH}/" | sort -n | tail -1)
    if [[ -z "${MT_BIN}" ]]; then
        MT_BIN=$(find "${WIN_SDK_BASE_PATH}" -name mt.exe | grep "/x64/" | sort -n | tail -1)
    fi

    if [[ -z "${MT_BIN}" ]]; then
        echo "Failed to find mt.exe"
        exit 1
    fi
fi

# The installer is Inno Setup's Setup.exe (${MINGW_PACKAGE_PREFIX}-natron-setup, from
# natron-build-deps-common), embedded in the portable zip.
if [ ! -f "${NATRON_SETUP_EXE:-}" ]; then
    echo "Error: ${MINGW_PREFIX}/share/natron-setup/Setup.exe not found: install ${MINGW_PACKAGE_PREFIX}-natron-setup (tools/MINGW-packages)"
    exit 1
fi

strip_binaries() { # strip_binaries <dir>...
    local dir
    if [ "$COMPILE_TYPE" = "debug" ]; then
        return 0
    fi
    for dir in "$@"; do
        echo "*** stripping binaries in $dir"
        find "$dir" -type f \( -iname '*.exe' -o -iname '*.dll' -o -iname '*.pyd' -o -iname '*.ofx' \) -exec strip -s {} \;
        echo "*** stripping binaries in $dir... done!"
    done
}

# installPlugin <plug-in name> [<name of the array of its private DLLs>]
function installPlugin() {
    local OFX_BINARY="$1" deps_name="${2:-}"
    local bundle="${TMP_PORTABLE_DIR}/Plugins/OFX/Natron/${OFX_BINARY}.ofx.bundle"
    local bindir="$bundle/Contents/${OFX_ARCHDIR}"
    local -a deps=()
    local depend

    if [ ! -d "${TMP_BINARIES_PATH}/OFX/Plugins/${OFX_BINARY}.ofx.bundle" ]; then
        return 0
    fi
    if [ -n "$deps_name" ]; then
        local -n deps_ref="$deps_name"
        deps=(${deps_ref[@]+"${deps_ref[@]}"})
    fi

    mkdir -p "${TMP_PORTABLE_DIR}/Plugins/OFX/Natron"
    cp -a "${TMP_BINARIES_PATH}/OFX/Plugins/${OFX_BINARY}.ofx.bundle" "${TMP_PORTABLE_DIR}/Plugins/OFX/Natron/"

    # Dump symbols
    if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
        dump_syms_safe "$bindir/${OFX_BINARY}.ofx" "${BUILD_ARCHIVE_DIRECTORY}/symbols/${OFX_BINARY}.ofx-${VERSION_TAG}${BPAD_TAG:-}-${PKGOS_BITS}.sym"
    fi

    if [ "${#deps[@]}" -gt 0 ]; then
        cp "${deps[@]}" "$bindir/"
    fi

    # Strip the ofx binary and its private DLLs. This must be done before the manifest is embedded
    # (mt.exe below): nothing may touch the .ofx after that.
    strip_binaries "$bindir"

    if [ "${#deps[@]}" -gt 0 ]; then
        # The plug-in needs a manifest embedded into its own DLL that lists its private DLLs, which
        # are next to it. The DLLs that it shares with Natron are in bin/ (genDllVersions.sh).
        # The manifest file itself is not shipped.
        local manifest="${TMP_BINARIES_PATH}/${OFX_BINARY}.ofx.manifest"
        {
            echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>"
            echo "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">"
            echo "<assemblyIdentity name=\"${OFX_BINARY}.ofx\" version=\"1.0.0.0\" type=\"win32\" processorArchitecture=\"${WIN_MANIFEST_ARCH}\"/>"
            for depend in "${deps[@]}"; do
                echo "<file name=\"$(basename "$depend")\"></file>"
            done
            echo "</assembly>"
        } > "$manifest"
        (cd "$bindir" && "${MT_BIN}" -nologo -manifest "$manifest" -outputresource:"${OFX_BINARY}.ofx;2")
        rm -f "$manifest"
    fi

    return 0
}

installPlugin IO
installPlugin Misc
installPlugin CImg
installPlugin Shadertoy
installPlugin Arena ARENA_DLL
installPlugin GMIC GMIC_DLL

location="${TMP_PORTABLE_DIR}"
mkdir -p "$location/docs" "$location/bin" "$location/lib" "$location/Resources" "$location/Plugins/PyPlugs"

cp -a "${TMP_BINARIES_PATH}/docs/natron"/* "$location/docs/"
cp -a "${TMP_BINARIES_PATH}/PyPlugs"/* "$location/Plugins/PyPlugs/"
# a Python module: Plugins/ is on the Python path
cp "${TMP_BINARIES_PATH}/natron_convert_project.py" "$location/Plugins/"

# the test program, run below and removed before the zip is made
cp "${TMP_BINARIES_PATH}/bin/Tests.exe" "$location/bin/"

if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    cp "${TMP_BINARIES_PATH}/bin/Natron.exe" "$location/bin/Natron-bin.exe"
    cp "${TMP_BINARIES_PATH}/bin/NatronRenderer.exe" "$location/bin/NatronRenderer-bin.exe"
    cp "${TMP_BINARIES_PATH}/bin/NatronCrashReporter.exe" "$location/bin/Natron.exe"
    cp "${TMP_BINARIES_PATH}/bin/NatronRendererCrashReporter.exe" "$location/bin/NatronRenderer.exe"
else
    cp "${TMP_BINARIES_PATH}/bin/Natron.exe" "$location/bin/Natron.exe"
    cp "${TMP_BINARIES_PATH}/bin/NatronRenderer.exe" "$location/bin/NatronRenderer.exe"
fi

if [ -f "${TMP_BINARIES_PATH}/bin/NatronProjectConverter.exe" ]; then
    cp "${TMP_BINARIES_PATH}/bin/NatronProjectConverter.exe" "$location/bin/NatronProjectConverter.exe"
fi

if [ -f "${TMP_BINARIES_PATH}/bin/natron-python.exe" ]; then
    cp "${TMP_BINARIES_PATH}/bin/natron-python.exe" "$location/bin/natron-python.exe"
fi

# OIIO, OpenEXR, libtiff and FFmpeg tools (listed by genDllVersions.sh)
cp "${NATRON_TOOLS[@]}" "$location/bin/"

mkdir -p "${location}"/Resources/{pixmaps,stylesheets,etc/fonts}
cp "$CWD/include/config/natronProjectIcon_windows.ico" "$location/Resources/pixmaps/"
cp "${TMP_BINARIES_PATH}/Resources/stylesheets/mainstyle.qss" "$location/Resources/stylesheets/"
cp -LR "${TMP_BINARIES_PATH}/Resources/OpenColorIO-Configs" "$location/Resources/"
cp -a "${TMP_BINARIES_PATH}/Resources/etc/fonts"/* "$location/Resources/etc/fonts/"
# poppler-data, for Arena's ReadPDF, which only GPL builds have (parts of poppler-data are GPL)
if [ "$NATRON_LICENSE" = "GPL" ]; then
    cp -a "$SDK_HOME/share/poppler" "$location/Resources/"
fi

# Qt plug-ins, and the DLLs of genDllVersions.sh (all the Qt DLLs among them, for PySide)
cp -a "$SDK_HOME/share/qt${QT_VERSION_MAJOR}/plugins"/* "$location/bin/"
rm "$location/bin/sqldrivers"/{*mysql*,*psql*} || true
if [ "$NATRON_LICENSE" = "COMMERCIAL" ]; then
    # the Qt plug-ins that link the GPL-3-only Qt modules
    rm -rf "$location/bin/designer" "$location/bin/virtualkeyboard" "$location/bin/assetimporters"
    rm -f "$location/bin/platforminputcontexts"/*virtualkeyboard*.dll
fi
cp "${NATRON_DLL[@]}" "$location/bin/"

strip_binaries "$location/bin"

# Dump symbols for crash reporting on Natron binaries
if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    dump_syms_safe "${TMP_BINARIES_PATH}/bin/Natron.exe" "${BUILD_ARCHIVE_DIRECTORY}/symbols/Natron-${VERSION_TAG}${BPAD_TAG:-}-${PKGOS_BITS}.sym"
    dump_syms_safe "${TMP_BINARIES_PATH}/bin/NatronRenderer.exe" "${BUILD_ARCHIVE_DIRECTORY}/symbols/NatronRenderer-${VERSION_TAG}${BPAD_TAG:-}-${PKGOS_BITS}.sym"
fi

# Copy Python distrib
mkdir -p "${TMP_PORTABLE_DIR}/Plugins"
cp -a "$SDK_HOME/lib/python${PYVER}" "${TMP_PORTABLE_DIR}/lib/"


# PySide2/shiboken2 go with Qt5 and PySide6/shiboken6 with Qt6, as in genDllVersions.sh.
if [ "$QT_VERSION_MAJOR" = 5 ]; then
    PYSIDE_MAJOR=2
else
    PYSIDE_MAJOR="$QT_VERSION_MAJOR"
fi
PYSIDE_PLUGIN_PATH="${TMP_PORTABLE_DIR}/Plugins/PySide${PYSIDE_MAJOR}"
mv "${TMP_PORTABLE_DIR}/lib/python${PYVER}/site-packages/PySide${PYSIDE_MAJOR}" "${TMP_PORTABLE_DIR}/lib/python${PYVER}/site-packages/shiboken${PYSIDE_MAJOR}" "${TMP_PORTABLE_DIR}/Plugins"
if [ "$NATRON_LICENSE" = "COMMERCIAL" ]; then
    # the PySide modules of the GPL-3 Qt modules (QtCharts.pyd, QtCharts.pyi, ...)
    for qtlib in "${QT_GPL_LIBS[@]}"; do
        rm -f "${PYSIDE_PLUGIN_PATH}/Qt${qtlib}".*
    done
fi

rm -rf "${TMP_PORTABLE_DIR}/lib/python${PYVER}"/{test,config,config-"${PYVER}"m}

( cd "${TMP_PORTABLE_DIR}/lib/python${PYVER}/site-packages";
for dir in *; do
if [ -d "$dir" ]; then
    rm -rf "$dir"
fi
done
)

# Strip Python
PYDIR="${TMP_PORTABLE_DIR}/lib/python${PYVER:-}"
find "${PYDIR}" -type f -name '*.pyo' -exec rm {} \;
(cd "${PYDIR}"; xargs rm -rf || true) < "$INC_PATH/python-exclude.txt"
(cd "${TMP_PORTABLE_DIR}" ;
    find . -type d -name __pycache__ -exec rm -rf {} \; || true
)

if [ "$COMPILE_TYPE" != "debug" ]; then
for dir in "${PYSIDE_PLUGIN_PATH}" "${TMP_PORTABLE_DIR}/lib/python${PYVER:-}"; do
    echo "*** stripping binaries in $dir"
    find "$dir" -type f \( -iname '*.exe' -o -iname '*.dll' -o -iname '*.pyd' -o -iname '*.ofx' \) -exec strip -s {} \;
    echo "*** stripping binaries in $dir... done!"
done
fi

# python zip

export PY_BIN="$SDK_HOME/bin/python.exe"
export PYDIR="$PYDIR"
#. "$CWD"/zip-python.sh
. "$CWD"/zip-python-mingw.sh

NATRON_PYTHON="${TMP_PORTABLE_DIR}/bin/natron-python"
# Install pip
if [ -x "${NATRON_PYTHON}" ]; then
    pushd "$PYDIR/../.."
    # get-pip.py of a fixed pypa/get-pip commit (pip 26.3, Python 3.10 or later), checked against
    # its sha256 before it runs: the bundle's Python runs it (ensurepip is not shipped, see
    # python-exclude.txt). To update: take the commit of a newer pypa/get-pip tag and its sha256.
    GET_PIP_COMMIT=03f582d4710047617da50bd36a871b596e651363
    GET_PIP_SHA256=f7654c5589318e1d0789575b2586ce76937eb9943602adda8b15b3be3aa0dac3
    rm -f get-pip.py
    $CURL --fail --output get-pip.py "https://raw.githubusercontent.com/pypa/get-pip/${GET_PIP_COMMIT}/public/get-pip.py"
    if ! echo "${GET_PIP_SHA256}  get-pip.py" | sha256sum -c -; then
        echo "Error: get-pip.py does not have the expected sha256"
        exit 1
    fi
    # pip and the Python packages that Natron ships, at fixed versions (pure Python)
    "${NATRON_PYTHON}" get-pip.py pip==26.3
    rm get-pip.py
    # Install qtpy
    "${NATRON_PYTHON}" -m pip install qtpy==2.4.3

    # Useful Python packages
    "${NATRON_PYTHON}" -m pip install future==1.0.0 six==1.17.0 #psutil
    # Run extra user provided pip install scripts
    if [ -f "${EXTRA_PYTHON_MODULES_SCRIPT:-}" ]; then
        "${NATRON_PYTHON}" "$EXTRA_PYTHON_MODULES_SCRIPT" || true
    fi
    "${NATRON_PYTHON}" -m compileall -f "${PYDIR}/site-packages"
    popd
fi


# Mesa opengl32.dll is an optional component of the installer (Natron.iss), so that the user can still fallback
# on a software renderer if OpenGL driver is not good enough.
# natron-windows-mesa only has x86 builds. On ARM64, MSYS2's mesa package (opengl32.dll and
# libgallium_wgl.dll, with their dependencies) should be bundled instead, which is not done yet
# (#1494): the ARM64 installer has no software OpenGL component.
if [ "$ARCH" = "x86_64" ]; then
    MESA_SHA256=123500be4539357c8055c7057cf5a11ef2557b893bee2458f595b5febff5dce5
    if [ ! -d "$SRC_PATH/natron-windows-mesa" ]; then
        ( cd "$SRC_PATH";
          rm -f natron-windows-mesa.tar.xz
          $CURL --fail "$THIRD_PARTY_BIN_URL/natron-windows-mesa.tar.xz" --output "$SRC_PATH/natron-windows-mesa.tar.xz"
          if ! echo "${MESA_SHA256}  natron-windows-mesa.tar.xz" | sha256sum -c -; then
              echo "Error: natron-windows-mesa.tar.xz does not have the expected sha256"
              exit 1
          fi
          tar xvf natron-windows-mesa.tar.xz
        )
    fi

    mkdir -p "${TMP_PORTABLE_DIR}/bin/mesa"
    cp "$SRC_PATH/natron-windows-mesa/win${BITS}/opengl32.dll" "${TMP_PORTABLE_DIR}/bin/mesa/"
else
    echo "Warning: no software OpenGL (Mesa opengl32.dll) for $WIN_ARCH yet (#1494)"
fi

# Generate documentation
bash "$CWD"/gen-natron-doc.sh

# At this point we can run Natron unit tests to check that the deployment is ok.
echo "Running Tests program..."
# With only the bundle's bin/ and the Windows directories on PATH (not $SDK_HOME/bin), so that a
# DLL missing from bin/ makes it fail to start. Starting must succeed; the test results do not
# fail the build yet.
BUNDLE_PATH="${TMP_PORTABLE_DIR}/bin:$(cygpath -u -S):$(cygpath -u -W)"
if ! $TIMEOUT -s KILL 300 env PATH="$BUNDLE_PATH" "${TMP_PORTABLE_DIR}/bin/Tests.exe" --gtest_list_tests > /dev/null; then
    echo "Error: Tests.exe does not start with PATH=$BUNDLE_PATH: a DLL is probably missing from bin/ (genDllVersions.sh)"
    exit 1
fi
$TIMEOUT -s KILL 1800 env PATH="$BUNDLE_PATH" "${TMP_PORTABLE_DIR}/bin/Tests.exe" || true
rm -f "${TMP_PORTABLE_DIR}/bin/Tests.exe"
echo "Done"

# Clean and perms
(cd "${TMP_PORTABLE_DIR}" && find . -type d -name .git -prune -exec rm -rf {} \;)

# A COMMERCIAL build must not contain or link any GPL library (#1302, #1177). This runs on the
# finished tree (after Mesa, the docs and pip), just before it is zipped: it inspects the contents
# of the binaries, where genDllVersions.sh only checked the DLL names. The portable tree is the
# only one (there is no IFW packages/ tree any more).
if [ "$NATRON_LICENSE" = "COMMERCIAL" ]; then
    "$CWD"/check-commercial-bundle.sh "${TMP_PORTABLE_DIR}"
fi

# Create the installer: the portable zip, with Inno Setup's Setup.exe
ZIP_INSTALL_DIR="."

# Portable zip, with Setup.exe
if [ "$DISABLE_PORTABLE_ARCHIVE" != "1" ]; then
    mkdir -p "${BUILD_ARCHIVE_DIRECTORY}/$ZIP_INSTALL_DIR"
    cp -a "${NATRON_SETUP_EXE}" "${TMP_BINARIES_PATH}/${PORTABLE_DIRNAME}/Setup.exe"
    (cd "${TMP_BINARIES_PATH}" && zip -9 -q -r "${PORTABLE_DIRNAME}.zip" "${PORTABLE_DIRNAME}"; mv "${PORTABLE_DIRNAME}.zip" "${BUILD_ARCHIVE_DIRECTORY}/$ZIP_INSTALL_DIR/${PORTABLE_DIRNAME}.zip")
fi

echo "*** Artifacts:"
ls -R  "${BUILD_ARCHIVE_DIRECTORY}"
echo "*** Windows installer: done!"

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
