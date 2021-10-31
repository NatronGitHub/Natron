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
set -x # Print commands and their arguments as they are executed.

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

# Generate dll versions with dlls on the system
env BITS="$BITS" PYVER="$PYVER" NATRON_LICENSE="$NATRON_LICENSE" ./genDllVersions.sh

source dllVersions.sh

updateBuildOptions

# Pretty architecture/platform identifier (used for breakpad symbol files)
PKGOS_BITS="${PKGOS}-x86_${BITS}bit"

if [ -z "${NATRON_BUILD_CONFIG:-}" ]; then
    echo "NATRON_BUILD_CONFIG empty"
    exit 1
fi

DUMP_SYMS="$SDK_HOME/bin/dump_syms.exe"

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
if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    mkdir "${BUILD_ARCHIVE_DIRECTORY}/symbols"
fi

# If we are in DEBUG_SYMBOLS mode there might already be deployed installer, remove them
if [ ! -z "$DEBUG_SCRIPTS" ]; then
    (cd "${TMP_BINARIES_PATH}" ; find . -type d -name 'Natron-*' -exec rm -rf {} \;) || true
fi


mkdir -p "${TMP_PORTABLE_DIR}"


# The following should be consistent with paths configured in uploadArtifactsMain.sh
if [ "${NATRON_BUILD_CONFIG}" = "SNAPSHOT" ]; then
    REMOTE_PATH="${REMOTE_PREFIX}/snapshots"
    APP_INSTALL_SUFFIX="INRIA/Natron-snapshot"
elif [ "${NATRON_BUILD_CONFIG}" = "RELEASE" ] ||  [ "${NATRON_BUILD_CONFIG}" = "STABLE" ]; then
    REMOTE_PATH="${REMOTE_PREFIX}/releases"
    APP_INSTALL_SUFFIX="INRIA/Natron-${NATRON_VERSION_FULL}"
else
    REMOTE_PATH="${REMOTE_PREFIX}/other_builds"
    APP_INSTALL_SUFFIX="INRIA/Natron"
fi

REMOTE_PROJECT_PATH="$REMOTE_PATH/$PKGOS/$BITS/$BUILD_NAME"
REMOTE_ONLINE_PACKAGES_PATH="$REMOTE_PROJECT_PATH/packages"

# The date passed to the ReleaseDate tag of the xml config file of the installer. This has a different format than CURRENT_DATE.
INSTALLER_XML_DATE="$(date -u "+%Y-%m-%d")"


# tag symbols we want to keep with 'release'
VERSION_TAG="${CURRENT_DATE}"
if [ "${NATRON_BUILD_CONFIG}" = "RELEASE" ] || [ "${NATRON_BUILD_CONFIG}" = "STABLE" ]; then
    BPAD_TAG="-release"
    VERSION_TAG="$NATRON_VERSION_FULL"
fi

# SETUP
INSTALLER_PATH="${TMP_BINARIES_PATH}/Natron-installer"
XML="$INC_PATH/xml"
QS="$INC_PATH/qs"

mkdir -p "$INSTALLER_PATH/config" "$INSTALLER_PATH/packages"

# Customize the config file
$GSED -e "s/_VERSION_/${NATRON_VERSION_FULL}/;s#_RBVERSION_#${NATRON_GIT_BRANCH}#g;s#_REMOTE_PATH_#${REMOTE_ONLINE_PACKAGES_PATH}#g;s#_REMOTE_URL_#${REMOTE_URL}#g;s#_APP_INSTALL_SUFFIX_#${APP_INSTALL_SUFFIX}#g" < "$INC_PATH/config/$PKGOS.xml" > "$INSTALLER_PATH/config/config.xml"

# Copy installer images to the config folder
cp "$INC_PATH/config/"*.png "$INSTALLER_PATH/config/"

# make sure we have mt and qtifw
if [ ! -f "$SDK_HOME/bin/mt.exe" ] || [ ! -f "$SDK_HOME/bin/binarycreator.exe" ]; then
    if [ ! -d "$SRC_PATH/natron-windows-installer" ]; then
        ( cd "$SRC_PATH";
          $CURL "$THIRD_PARTY_BIN_URL/natron-windows-installer.zip" --output "$SRC_PATH/natron-windows-installer.zip"
          unzip natron-windows-installer.zip
        )
    fi
    cp "$SRC_PATH/natron-windows-installer/mingw$BITS/bin/"{archivegen.exe,binarycreator.exe,installerbase.exe,installerbase.manifest,repogen.exe,mt.exe} "$SDK_HOME/bin/"
fi

function installPlugin() {
    OFX_BINARY="$1"
    DEPENDENCIES_DLL="$2"
    PACKAGE_NAME="$3"
    PACKAGE_XML="$4"
    PACKAGE_INSTALL_SCRIPT="$5"

    if [ ! -d "${TMP_BINARIES_PATH}/OFX/Plugins/${OFX_BINARY}.ofx.bundle" ]; then
        return 0
    fi

    # Create package
    PKG_PATH="$INSTALLER_PATH/packages/$PACKAGE_NAME"
    if [ ! -d "$PKG_PATH" ]; then
        mkdir -p "$PKG_PATH/data" "$PKG_PATH/meta" "$PKG_PATH/data/Plugins/OFX/Natron"
    fi

    #  portable archive
    if [ ! -d "${TMP_PORTABLE_DIR}/Plugins/OFX/Natron" ]; then
        mkdir -p "${TMP_PORTABLE_DIR}/Plugins/OFX/Natron"
    fi

    # Configure config xml file
    if [ ! -f "$PKG_PATH/meta/package.xml" ]; then
        $GSED -e "s/_VERSION_/${VERSION_TAG}/;s/_DATE_/${INSTALLER_XML_DATE}/" < "$PACKAGE_XML" > "$PKG_PATH/meta/package.xml"
    fi
    if [ ! -f "$PKG_PATH/meta/installscript.qs" ]; then
        cp "$PACKAGE_INSTALL_SCRIPT" "$PKG_PATH/meta/installscript.qs"
    fi


    cp -a "${TMP_BINARIES_PATH}/OFX/Plugins/${OFX_BINARY}.ofx.bundle" "${TMP_PORTABLE_DIR}/Plugins/OFX/Natron/"
    cp -a "${TMP_BINARIES_PATH}/OFX/Plugins/${OFX_BINARY}.ofx.bundle" "$PKG_PATH/data/Plugins/OFX/Natron/"

        # Dump symbols
    if [ "${DISABLE_BREAKPAD:-}" != "1" ] && [ -d "${TMP_BINARIES_PATH}/OFX/Plugins/${OFX_BINARY}.ofx.bundle" ]; then
        dump_syms_safe "$PKG_PATH/data/Plugins/OFX/Natron/${OFX_BINARY}.ofx.bundle/Contents/Win${BITS}/${OFX_BINARY}.ofx" "${BUILD_ARCHIVE_DIRECTORY}/symbols/${OFX_BINARY}.ofx-${VERSION_TAG}${BPAD_TAG:-}-${PKGOS_BITS}.sym"
    fi

    # Strip ofx binaries (must be done before manifest)
    if [ "$COMPILE_TYPE" != "debug" ]; then
        for dir in "$PKG_PATH/data/Plugins/OFX/Natron" "${TMP_PORTABLE_DIR}/Plugins/OFX/Natron"; do
            echo "*** stripping ofx binaries in $dir"
            find "$dir" -type f \( -iname '*.ofx' \) -exec strip -s {} \;
            echo "*** stripping ofx binaries in $dir... done!"
        done
    fi

    if [ ! -z "$DEPENDENCIES_DLL" ]; then
        for depend in ${DEPENDENCIES_DLL}; do
            cp "$depend" "$PKG_PATH/data/Plugins/OFX/Natron/${OFX_BINARY}.ofx.bundle/Contents/Win${BITS}/"
            cp "$depend" "${TMP_PORTABLE_DIR}/Plugins/OFX/Natron/${OFX_BINARY}.ofx.bundle/Contents/Win${BITS}/"
        done

        # The plug-in needs a manifest embedeed into its own DLL to indicate the relative path of its own
        # dependencies.
        # If some of them are shared with Natron, its better to install them with Natron dependencies.
        PLUGIN_MANIFEST="$PKG_PATH/data/Plugins/OFX/Natron/${OFX_BINARY}.ofx.bundle/Contents/Win${BITS}/manifest"
        echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" > "$PLUGIN_MANIFEST"
        echo "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">" >> "$PLUGIN_MANIFEST"
        echo "<assemblyIdentity name=\"${OFX_BINARY}.ofx\" version=\"1.0.0.0\" type=\"win32\" processorArchitecture=\"amd64\"/>" >> "$PLUGIN_MANIFEST"
        DEPS_DLL="$(cd "$PKG_PATH/data/Plugins/OFX/Natron/${OFX_BINARY}.ofx.bundle/Contents/Win${BITS}";ls ./*.dll)" || true
        for depend in $DEPS_DLL; do
            trimmeddep="$(basename "$depend")"
            echo "<file name=\"${trimmeddep}\"></file>" >> "$PLUGIN_MANIFEST"
        done
        echo "</assembly>" >> "$PLUGIN_MANIFEST"
        for location in "$PKG_PATH/data" "${TMP_PORTABLE_DIR}"; do
            (cd "$location/Plugins/OFX/Natron/${OFX_BINARY}.ofx.bundle/Contents/Win${BITS}"; mt -nologo -manifest "$PLUGIN_MANIFEST" -outputresource:"${OFX_BINARY}.ofx;2")
        done

    fi

    # Strip all binaries
    if [ "$COMPILE_TYPE" != "debug" ]; then
        for dir in "$PKG_PATH/data/Plugins/OFX/Natron" "${TMP_PORTABLE_DIR}/Plugins/OFX/Natron"; do
            echo "*** stripping binaries in $dir"
            find "$dir" -type f \( -iname '*.exe' -o -iname '*.dll' -o -iname '*.pyd' \) -exec strip -s {} \;
            echo "*** stripping binaries in $dir... done!"
        done
    fi

    return 0
}

# Natron package
NATRON_PKG="fr.inria.natron"
NATRON_PACKAGE_PATH="${INSTALLER_PATH}/packages/${NATRON_PKG}"
PACKAGES="${NATRON_PKG}"

PACKAGES="${PACKAGES},fr.inria.openfx.io"
installPlugin "IO" "" "fr.inria.openfx.io" "$XML/openfx-io.xml" "$QS/openfx-io.qs"
PACKAGES="${PACKAGES},fr.inria.openfx.misc"
installPlugin "Misc" "" "fr.inria.openfx.misc" "$XML/openfx-misc.xml" "$QS/openfx-misc.qs"
installPlugin "CImg" "" "fr.inria.openfx.misc" "$XML/openfx-misc.xml" "$QS/openfx-misc.qs"
installPlugin "Shadertoy" "" "fr.inria.openfx.misc" "$XML/openfx-misc.xml" "$QS/openfx-misc.qs"
PACKAGES="${PACKAGES},fr.inria.openfx.extra"
installPlugin "Arena" "${ARENA_DLL[*]}" "fr.inria.openfx.extra" "$XML/openfx-arena.xml" "$QS/openfx-arena.qs"
installPlugin "GMIC" "${GMIC_DLL[*]}" "fr.inria.openfx.extra" "$XML/openfx-gmic.xml" "$QS/openfx-gmic.qs"

# Create package directories
mkdir -p "$NATRON_PACKAGE_PATH/meta"

# Configure natron package xml
$GSED "s/_VERSION_/${VERSION_TAG}/;s/_DATE_/${INSTALLER_XML_DATE}/" < "$XML/natron.xml" > "$NATRON_PACKAGE_PATH/meta/package.xml"
cp "$QS/$PKGOS/natron.qs" "$NATRON_PACKAGE_PATH/meta/installscript.qs"
cat "${TMP_BINARIES_PATH}/docs/natron/LICENSE.txt" > "$NATRON_PACKAGE_PATH/meta/natron-license.txt"

# We copy all files to both the portable archive and the package for the installer in a loop
COPY_LOCATIONS=("${TMP_PORTABLE_DIR}" "$NATRON_PACKAGE_PATH/data")

for location in "${COPY_LOCATIONS[@]}"; do

    mkdir -p "$location/docs" "$location/bin" "$location/Resources" "$location/Plugins/PyPlugs"

    cp -a "${TMP_BINARIES_PATH}/docs/natron"/* "$location/docs/"
    cp -a "${TMP_BINARIES_PATH}/PyPlugs"/* "$location/Plugins/PyPlugs/"

    # Include in the portable version the test program that we will use later on
    if [ "$location" = "${TMP_PORTABLE_DIR}" ]; then
        cp "${TMP_BINARIES_PATH}/bin/Tests.exe" "$location/bin/"
    fi

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

    cp "$SDK_HOME"/bin/{iconvert.exe,idiff.exe,igrep.exe,iinfo.exe} "$location/bin/"
    cp "$SDK_HOME"/bin/{exrheader.exe,tiffinfo.exe} "$location/bin/"
    cp "${FFMPEG_PATH}"/bin/{ffmpeg.exe,ffprobe.exe} "$location/bin/"

    mkdir -p "${location}"/Resources/{pixmaps,stylesheets}
    cp "$CWD/include/config/natronProjectIcon_windows.ico" "$location/Resources/pixmaps/"
    cp "${TMP_BINARIES_PATH}/Resources/stylesheets/mainstyle.qss" "$location/Resources/stylesheets/"


    # OCIO -> has its own package, see below
    #cp -a "${TMP_BINARIES_PATH}/Resources/OpenColorIO-Configs" "$location/Resources/"

    # Strip binaries when not in debug
    if [ "$COMPILE_TYPE" != "debug" ]; then
        for dir in $location/bin; do
            echo "*** stripping binaries in $dir"
            find "$dir" -type f \( -iname '*.exe' -o -iname '*.dll' -o -iname '*.pyd' -o -iname '*.ofx' \) -exec strip -s {} \;
            echo "*** stripping binaries in $dir... done!"
        done
    fi

    # end for all locations
done

# Dump symbols for crash reporting on Natron binaries
if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    dump_syms_safe "${TMP_BINARIES_PATH}/bin/Natron.exe" "${BUILD_ARCHIVE_DIRECTORY}/symbols/Natron-${VERSION_TAG}${BPAD_TAG:-}-${PKGOS_BITS}.sym"
    dump_syms_safe "${TMP_BINARIES_PATH}/bin/NatronRenderer.exe" "${BUILD_ARCHIVE_DIRECTORY}/symbols/NatronRenderer-${VERSION_TAG}${BPAD_TAG:-}-${PKGOS_BITS}.sym"
fi



# OCIO package
OCIO_PKG="fr.inria.natron.color"
PACKAGES="${PACKAGES},${OCIO_PKG}"
OCIO_PACKAGE_PATH="${INSTALLER_PATH}/packages/${OCIO_PKG}"

# OCIO package version (linux/windows)
# bump number when OpenColorIO-Configs changes
OCIO_VERSION="20180327000000"
# OCIO
for c in blender natron nuke-default; do
    lib="${TMP_BINARIES_PATH}/Resources/OpenColorIO-Configs/${c}/config.ocio"
    LAST_MODIFICATION_DATE="$(date -u -r "$lib" "+%Y%m%d%H%M%S")"
    if [ "$LAST_MODIFICATION_DATE" -gt "$OCIO_VERSION" ]; then
        OCIO_VERSION="$LAST_MODIFICATION_DATE"
    fi
done

# Create package directories
mkdir -p "$OCIO_PACKAGE_PATH/meta"
$GSED "s/_VERSION_/${OCIO_VERSION}/;s/_DATE_/${INSTALLER_XML_DATE}/" "$XML/ocio.xml" > "$OCIO_PACKAGE_PATH/meta/package.xml"
cat "$QS/ocio.qs" > "$OCIO_PACKAGE_PATH/meta/installscript.qs"

# We copy all files to both the portable archive and the package for the installer in a loop
COPY_LOCATIONS=("${TMP_PORTABLE_DIR}" "$OCIO_PACKAGE_PATH/data")

for location in "${COPY_LOCATIONS[@]}"; do

    mkdir -p "$location/Resources"
    cp -LR "${TMP_BINARIES_PATH}/Resources/OpenColorIO-Configs" "$location/Resources/"

    # end for all locations
done


# Distribute Natron dependencies in a separate package so that the user only
# receive updates for DLLs when we actually update them
# rather than every time we recompile Natron
CORELIBS_PKG="fr.inria.natron.libs"
PACKAGES="${PACKAGES},${CORELIBS_PKG}"
DLLS_PACKAGE_PATH="${INSTALLER_PATH}/packages/${CORELIBS_PKG}"
mkdir -p "${DLLS_PACKAGE_PATH}/meta"

# We copy all files to both the portable archive and the package for the installer in a loop
COPY_LOCATIONS=("${TMP_PORTABLE_DIR}" "$DLLS_PACKAGE_PATH/data")


for location in "${COPY_LOCATIONS[@]}"; do
    mkdir -p "$location/bin" "$location/lib" "$location/Resources/pixmaps" "$location/Resources/etc/fonts"
    #cp -a "$SDK_HOME/etc/fonts"/* "$location/Resources/etc/fonts"
    cp -a "${TMP_BINARIES_PATH}/Resources/etc/fonts"/* "$location/Resources/etc/fonts/"
    cp -a "$SDK_HOME/share/poppler" "$location/Resources/"
    cp -a "$SDK_HOME/share/qt4/plugins"/* "$location/bin/"
    rm -f "$location/bin"/*/*d4.dll || true

    for depend in "${NATRON_DLL[@]}"; do
        cp "$depend" "$location/bin/"
    done

    #Copy Qt dlls (required for all PySide modules to work correctly)
    cp "$SDK_HOME/bin"/Qt*4.dll "$location/bin/"


    # Ignore debug dlls of Qt
    rm "$location/bin"/*d4.dll || true
    rm "$location/bin/sqldrivers"/{*mysql*,*psql*} || true

    if [ "$COMPILE_TYPE" != "debug" ]; then
        dir="$location/bin"
        echo "*** stripping binaries in $dir"
        find "$dir" -type f \( -iname '*.exe' -o -iname '*.dll' -o -iname '*.pyd' -o -iname '*.ofx' \) -exec strip -s {} \;
        echo "*** stripping binaries in $dir... done!"
    fi

    # end for all locations
done

# Copy Python distrib
mkdir -p "${TMP_PORTABLE_DIR}/Plugins"
cp -a "$SDK_HOME/lib/python${PYVER}" "${TMP_PORTABLE_DIR}/lib/"

mv "${TMP_PORTABLE_DIR}/lib/python${PYVER}/site-packages/PySide" "${TMP_PORTABLE_DIR}/Plugins/"
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
(cd "${TMP_PORTABLE_DIR}" ; find . -type d -name __pycache__ -exec rm -rf {} \;)

if [ "$COMPILE_TYPE" != "debug" ]; then
for dir in "${TMP_PORTABLE_DIR}/Plugins/PySide" "${TMP_PORTABLE_DIR}/lib/python${PYVER:-}"; do
    echo "*** stripping binaries in $dir"
    find "$dir" -type f \( -iname '*.exe' -o -iname '*.dll' -o -iname '*.pyd' -o -iname '*.ofx' \) -exec strip -s {} \;
    echo "*** stripping binaries in $dir... done!"
done
fi

# python zip
if [ "${USE_QT5:-}" != 1 ]; then
    rm -rf  "$PYDIR"/site-packages/shiboken2*  "$PYDIR"/site-packages/PySide2 || true
fi

export PY_BIN="$SDK_HOME/bin/python.exe"
export PYDIR="$PYDIR"
. "$CWD"/zip-python.sh

NATRON_PYTHON="${TMP_PORTABLE_DIR}/bin/natron-python"
# Install pip
if [ -x "${NATRON_PYTHON}" ]; then
    if [ "$PYV" = "2" ]; then
        $CURL --remote-name --insecure https://bootstrap.pypa.io/pip/${PYVER}/get-pip.py
    else
        $CURL --remote-name --insecure https://bootstrap.pypa.io/get-pip.py
    fi
    "${NATRON_PYTHON}" get-pip.py
    rm get-pip.py
    # Install qtpy
    if [ "${USE_QT5:-}" != 1 ]; then
        # Qt4 support was dropped after QtPy 1.11.2
        "${NATRON_PYTHON}" -m pip install qtpy==1.11.2
    else
        "${NATRON_PYTHON}"" -m pip install qtpy
    fi
    # Run extra user provided pip install scripts
    if [ -f "${EXTRA_PYTHON_MODULES_SCRIPT:-}" ]; then
        "${NATRON_PYTHON}"" "$EXTRA_PYTHON_MODULES_SCRIPT" || true
    fi
fi


# Copy Python distrib to installer package
cp -r "$PYDIR" "$DLLS_PACKAGE_PATH/data/lib/"
cp "${TMP_PORTABLE_DIR}"/lib/python*.zip "${DLLS_PACKAGE_PATH}/data/lib/"
mkdir -p "${DLLS_PACKAGE_PATH}/data/Plugins"
cp -a "${TMP_PORTABLE_DIR}/Plugins/PySide" "${DLLS_PACKAGE_PATH}/data/Plugins/"

# Configure the package date using the most recent DLL modification date
CLIBS_VERSION="00000000000000"
for depend in "${NATRON_DLL[@]}"; do
    LAST_MODIFICATION_DATE="$(date -u -r "$depend" "+%Y%m%d%H%M%S")"
    if [ "$LAST_MODIFICATION_DATE" -gt "$CLIBS_VERSION" ]; then
        CLIBS_VERSION="$LAST_MODIFICATION_DATE"
    fi
done
$GSED "s/_VERSION_/${CLIBS_VERSION}/;s/_DATE_/${INSTALLER_XML_DATE}/" < "$XML/corelibs.xml" > "$DLLS_PACKAGE_PATH/meta/package.xml"
cp "$QS/$PKGOS/corelibs.qs" "$DLLS_PACKAGE_PATH/meta/installscript.qs"

# Mesa opengl32.dll is an optional package in the installer, so that the user can still fallback
# on a software renderer if OpenGL driver is not good enough
if [ ! -d "$SRC_PATH/natron-windows-mesa" ]; then
    ( cd "$SRC_PATH";
      $CURL "$THIRD_PARTY_BIN_URL/natron-windows-mesa.tar.xz" --output "$SRC_PATH/natron-windows-mesa.tar.xz"
      tar xvf natron-windows-mesa.tar.xz
    )
fi

PACKAGES="${PACKAGES},fr.inria.mesa"
MESA_PKG_PATH="$INSTALLER_PATH/packages/fr.inria.mesa"
mkdir -p "$MESA_PKG_PATH/meta" "$MESA_PKG_PATH/data/bin" "${TMP_PORTABLE_DIR}/bin/mesa"
$GSED "s/_DATE_/${INSTALLER_XML_DATE}/" < "$XML/mesa.xml" > "$MESA_PKG_PATH/meta/package.xml"
cp "$QS/mesa.qs" "$MESA_PKG_PATH/meta/installscript.qs"
cp "$SRC_PATH/natron-windows-mesa/win${BITS}/opengl32.dll" "$MESA_PKG_PATH/data/bin/"
cp "$SRC_PATH/natron-windows-mesa/win${BITS}/opengl32.dll" "${TMP_PORTABLE_DIR}/bin/mesa/"

# Generate documentation
bash "$CWD"/gen-natron-doc.sh

# Copy documentation installed in the portable dir to installer package
if [ -d "$NATRON_PACKAGE_PATH/data/Resources/docs" ]; then
    rm -rf "$NATRON_PACKAGE_PATH/data/Resources/docs"
fi
if [ -d "${TMP_PORTABLE_DIR}/Resources/docs" ]; then
    cp -r "${TMP_PORTABLE_DIR}/Resources/docs" "$NATRON_PACKAGE_PATH/data/Resources/"
fi

# At this point we can run Natron unit tests to check that the deployment is ok.
echo "Running Tests program..."
# Do not fail tests temporarily
$TIMEOUT -s KILL 1800 "${TMP_PORTABLE_DIR}/bin/Tests" || true
rm "${TMP_PORTABLE_DIR}/bin/Tests"
echo "Done"

# Clean and perms
(cd "$INSTALLER_PATH" && find . -type d -name .git -exec rm -rf {} \;)

# Create installers

ONLINE_INSTALL_DIR="online_installer"
#BUNDLED_INSTALL_DIR="offline_installer"
#ZIP_INSTALL_DIR="compressed_no_installer"

# These have easily identifiable filenames and extensions anyway:
BUNDLED_INSTALL_DIR="."
ZIP_INSTALL_DIR="."

mkdir -p "${BUILD_ARCHIVE_DIRECTORY}/$BUNDLED_INSTALL_DIR"

# Online installer (legacy)
if [ "$WITH_ONLINE_INSTALLER" = "1" ] && [ ! -f "/Setup.exe" ]; then
    mkdir -p "${BUILD_ARCHIVE_DIRECTORY}/$ONLINE_INSTALL_DIR"
    mkdir -p "${BUILD_ARCHIVE_DIRECTORY}/$ONLINE_INSTALL_DIR/packages"

    # Gen repo for online install
    "$SDK_HOME/bin/repogen" -v --update-new-components -p "$INSTALLER_PATH/packages" -c "$INSTALLER_PATH/config/config.xml" "${BUILD_ARCHIVE_DIRECTORY}/$ONLINE_INSTALL_DIR/packages"

    # Online installer
    "$SDK_HOME/bin/binarycreator" -v -n -p "$INSTALLER_PATH/packages" -c "$INSTALLER_PATH/config/config.xml" "${BUILD_ARCHIVE_DIRECTORY}/$ONLINE_INSTALL_DIR/${INSTALLER_BASENAME}-online.exe"
fi

# Offline installer (legacy)
if [ ! -f "/Setup.exe" ]; then
    "$SDK_HOME/bin/binarycreator" -v -f -p "$INSTALLER_PATH/packages" -c "$INSTALLER_PATH/config/config.xml" -i "${PACKAGES}" "${BUILD_ARCHIVE_DIRECTORY}/$BUNDLED_INSTALL_DIR/${INSTALLER_BASENAME}.exe"
fi

# Portable zip (+ setup if found)
if [ "$DISABLE_PORTABLE_ARCHIVE" != "1" ]; then
    mkdir -p "${BUILD_ARCHIVE_DIRECTORY}/$ZIP_INSTALL_DIR"
    if [ -f "/Setup.exe" ]; then
        cp -a /Setup.exe "${TMP_BINARIES_PATH}/${PORTABLE_DIRNAME}/"
    fi
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
