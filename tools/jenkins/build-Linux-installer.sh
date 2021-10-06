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
# Build packages and installer for Linux
#
# Options:
# DISABLE_BREAKPAD=1: Disable automatic crash report


set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

echo "*** Linux installer..."

source common.sh
source manageBuildOptions.sh
source manageLog.sh
updateBuildOptions

pushd () {
    command pushd "$@" > /dev/null
}

popd () {
    command popd "$@" > /dev/null
}

LD_LIBRARY_PATH="${SDK_HOME}/lib:${FFMPEG_PATH}/lib:${SDK_HOME}/qt${QT_VERSION_MAJOR}/lib"
PATH="${SDK_HOME}/gcc/bin:${SDK_HOME}/bin:$PATH"
export C_INCLUDE_PATH="${SDK_HOME}/gcc/include:${SDK_HOME}/include:${SDK_HOME}/qt${QT_VERSION_MAJOR}/include"
export CPLUS_INCLUDE_PATH="${C_INCLUDE_PATH}"

if [ "${ARCH}" = "x86_64" ]; then
    LD_LIBRARY_PATH="${SDK_HOME}/gcc/lib64:${FFMPEG_PATH}/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
else
    LD_LIBRARY_PATH="${SDK_HOME}/gcc/lib:${FFMPEG_PATH}/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi
export LD_LIBRARY_PATH

# Pretty architecture/platform identifier (used for breakpad symbol files)
PKGOS_BITS="${PKGOS}-x86_${BITS}bit"

if [ -d "${BUILD_ARCHIVE_DIRECTORY}" ]; then
    rm -rf "${BUILD_ARCHIVE_DIRECTORY}"
fi

mkdir -p "${BUILD_ARCHIVE_DIRECTORY}"
if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    mkdir "${BUILD_ARCHIVE_DIRECTORY}/symbols"
fi

INSTALLER_PATH="${TMP_BINARIES_PATH}/Natron-installer"


# If we are in DEBUG_SYMBOLS mode there might already be deployed installer, remove them
if [ ! -z "$DEBUG_SCRIPTS" ]; then
    (cd "${TMP_BINARIES_PATH}" ; find . -type d -name 'Natron-*' -exec rm -rf {} \;) || true
fi

mkdir -p "${TMP_PORTABLE_DIR}"



if [ -z "${NATRON_BUILD_CONFIG:-}" ]; then
    echo "NATRON_BUILD_CONFIG empty"
    exit 1
fi

# The following should be consistent with paths configured in uploadArtifactsMain.sh
if [ "$NATRON_BUILD_CONFIG" = "SNAPSHOT" ]; then
    REMOTE_PATH="${REMOTE_PREFIX}/snapshots"
    APP_INSTALL_SUFFIX="Natron-snapshot"
    APP_ADMIN_INSTALL_SUFFIX="$APP_INSTALL_SUFFIX"
elif [ "$NATRON_BUILD_CONFIG" = "RELEASE" ] || [ "$NATRON_BUILD_CONFIG" = "STABLE" ]; then
    REMOTE_PATH="${REMOTE_PREFIX}/releases"
    APP_INSTALL_SUFFIX="Natron-${NATRON_VERSION_FULL}"
    APP_ADMIN_INSTALL_SUFFIX="$APP_INSTALL_SUFFIX"
else
    REMOTE_PATH="${REMOTE_PREFIX}/other_builds"
    APP_INSTALL_SUFFIX="Natron"
    APP_ADMIN_INSTALL_SUFFIX="$APP_INSTALL_SUFFIX"
fi

REMOTE_PROJECT_PATH="$REMOTE_PATH/$PKGOS/$BITS/$BUILD_NAME"
REMOTE_ONLINE_PACKAGES_PATH="$REMOTE_PROJECT_PATH/packages"

# The date passed to the ReleaseDate tag of the xml config file of the installer. This has a different format than CURRENT_DATE.
INSTALLER_XML_DATE="$(date -u "+%Y-%m-%d")"

# tag symbols we want to keep with 'release'
VERSION_TAG="${CURRENT_DATE}"
if [ "${NATRON_BUILD_CONFIG}" = "RELEASE" ] || [ "$NATRON_BUILD_CONFIG" = "STABLE" ]; then
    BPAD_TAG="-release"
    VERSION_TAG="${NATRON_VERSION_FULL}"
fi


# SETUP
XML="$INC_PATH/xml"
QS="$INC_PATH/qs"

mkdir -p "${INSTALLER_PATH}/config" "${INSTALLER_PATH}/packages"

# Customize the config file
$GSED "s/_VERSION_/${NATRON_VERSION_FULL}/;s#_RBVERSION_#${NATRON_GIT_BRANCH}#g;s#_REMOTE_PATH#${REMOTE_ONLINE_PACKAGES_PATH}#g;s#_REMOTE_URL_#${REMOTE_URL}#g;s#_APP_INSTALL_SUFFIX_#${APP_INSTALL_SUFFIX}#g;s#_APP_ADMIN_INSTALL_SUFFIX_#${APP_ADMIN_INSTALL_SUFFIX}#g" "$INC_PATH/config/$PKGOS.xml" > "${INSTALLER_PATH}/config/config.xml"

# Copy installer images to the config folder
cp "$INC_PATH/config"/*.png "${INSTALLER_PATH}/config/"

function installPlugin() {
    OFX_BINARY="$1"
    PACKAGE_NAME="$2"
    PACKAGE_XML="$3"
    PACKAGE_INSTALL_SCRIPT="$4"
    NATRON_LIBS="$5"

    if [ ! -d "${TMP_BINARIES_PATH}/OFX/Plugins/${OFX_BINARY}.ofx.bundle" ]; then
        return 0
    fi

    # Create package
    PKG_PATH="${INSTALLER_PATH}/packages/$PACKAGE_NAME"
    if [ ! -d "$PKG_PATH" ]; then
        mkdir -p "$PKG_PATH/data" "$PKG_PATH/meta" "$PKG_PATH/data/Plugins/OFX/Natron"
    fi

    # Copy to portable archive
    if [ ! -d "${TMP_PORTABLE_DIR}/Plugins/OFX/Natron" ]; then
        mkdir -p "${TMP_PORTABLE_DIR}/Plugins/OFX/Natron"
    fi

    # Configure package for installer
    if [ ! -f "$PKG_PATH/meta/package.xml" ]; then
        $GSED -e "s/_VERSION_/${VERSION_TAG}/;s/_DATE_/${INSTALLER_XML_DATE}/" < "$PACKAGE_XML" > "$PKG_PATH/meta/package.xml"
    fi
    if [ ! -f "$PKG_PATH/meta/installscript.qs" ]; then
        cp "$PACKAGE_INSTALL_SCRIPT" "$PKG_PATH/meta/installscript.qs"
    fi

    # Dump symbols
    if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
        "${SDK_HOME}/bin/dump_syms" "${TMP_BINARIES_PATH}"/OFX/Plugins/"${OFX_BINARY}".ofx.bundle/Contents/*/"${OFX_BINARY}".ofx > "${BUILD_ARCHIVE_DIRECTORY}/symbols/${OFX_BINARY}.ofx-${CURRENT_DATE}${BPAD_TAG:-}-${PKGOS_BITS}.sym"
    fi




    # Extract dependencies
    OFX_DEPENDS="$(ldd $(find "${TMP_BINARIES_PATH}/OFX/Plugins/${OFX_BINARY}.ofx.bundle/Contents/Linux-"* -maxdepth 1 -type f) | grep "${SDK_HOME}" | awk '{print $3}'|sort|uniq)"
    if [ ! -z "$OFX_DEPENDS" ]; then


        LIBS_DIR="${TMP_BINARIES_PATH}/OFX/Plugins/${OFX_BINARY}.ofx.bundle/Libraries"
        mkdir -p "$LIBS_DIR"

        DEPENDS_NOT_PART_OF_NATRON=()
        for x in $OFX_DEPENDS; do
            # Add the dep only if it is not part of Natron libs
            pluginlib="$(basename "$x")"
            CREATE_SYM_LINK=0
            for y in $NATRON_LIBS; do
               natronlib="$(basename "$y")"
               if [ "$pluginlib" = "$natronlib" ]; then
                    CREATE_SYM_LINK=1
                    break
               fi
            done
            if [ "$CREATE_SYM_LINK" = "1" ]; then
                # Create a sym-link to the already bundled dependency in ${TMP_PORTABLE_DIR}/lib
                # This is a relative path, assuming the plug-in Libraries directory is:
                # ${TMP_PORTABLE_DIR}/Plugins/OFX/Natron/${OFX_BINARY}.ofx.bundle/Libraries
                (cd "$LIBS_DIR"; ln -sf ../../../../../lib/"$pluginlib" .)
            else
                DEPENDS_NOT_PART_OF_NATRON+=("$x")
            fi
        done

        # Copy deps to Libraries directory in the plug-in bundle
        for x in "${DEPENDS_NOT_PART_OF_NATRON[@]-}"; do
            if [ ! -z "$x" ]; then
                pluginlib="$(basename "$x")"
                if [ -f "$x" ] && [ ! -f "$LIBS_DIR/$pluginlib" ] && [ ! -L "$LIBS_DIR/$pluginlib" ]; then
                    cp -f "$x" "$LIBS_DIR/"
                fi
            fi
        done

        # Extract dependencies of the dependencies
        OFX_LIB_DEP=$(ldd $(find "$LIBS_DIR" -maxdepth 1 -type f) |grep "$SDK_HOME" | awk '{print $3}'|sort|uniq)
        for y in $OFX_LIB_DEP; do
            pluginlib="$(basename "$y")"
            if [ ! -f "$LIBS_DIR/$pluginlib" ] && [ ! -L "$LIBS_DIR/$pluginlib" ]; then
                cp -f "$y" "$LIBS_DIR/"
            fi
        done

        # Set the rpath of the shared libraries to origin
        pushd "$LIBS_DIR"
        chmod 755 *.so* || true
        for i in *.so*; do
            patchelf --force-rpath --set-rpath "\$ORIGIN" "$i" || true
        done
        popd
    fi

    # Strip binary
    if [ "$COMPILE_TYPE" != "debug" ]; then
        find "${TMP_BINARIES_PATH}/OFX/Plugins/${OFX_BINARY}.ofx.bundle" -type f \( -iname '*.so' -o -iname '*.ofx' \) -exec strip -s {} \; &>/dev/null
    fi

    for location in "$PKG_PATH/data" "${TMP_PORTABLE_DIR}"; do
        # Copy plug-in bundle
        cp -a "${TMP_BINARIES_PATH}/OFX/Plugins/${OFX_BINARY}.ofx.bundle" "$location/Plugins/OFX/Natron/"
    done

}


# Natron package
NATRON_PKG="fr.inria.natron"
NATRON_PACKAGE_PATH="${INSTALLER_PATH}/packages/${NATRON_PKG}"
PACKAGES="${NATRON_PKG}"

# Create package directories
mkdir -p "$NATRON_PACKAGE_PATH/meta"

# Configure natron package xml
$GSED "s/_VERSION_/${CURRENT_DATE}/;s/_DATE_/${INSTALLER_XML_DATE}/" "$XML/natron.xml" > "$NATRON_PACKAGE_PATH/meta/package.xml"
cat "$QS/$PKGOS/natron.qs" > "$NATRON_PACKAGE_PATH/meta/installscript.qs"
cat "${TMP_BINARIES_PATH}/docs/natron/LICENSE.txt" > "$NATRON_PACKAGE_PATH/meta/natron-license.txt"


#GLIBCXX 3.4.19 is for GCC 4.8.3, see https://gcc.gnu.org/onlinedocs/libstdc++/manual/abi.html
compat_version_script=3.4.19
compat_version=3.4.19
case "${TC_GCC:-}" in
    4.4.[2-9])
        compat_version=3.4.13
        ;;
    4.5.*)
        compat_version=3.4.14
        ;;
    4.6.0)
        compat_version=3.4.15
        ;;
    4.6.[1-9])
        compat_version=3.4.16
        ;;
    4.7.*)
        compat_version=3.4.17
        ;;
    4.8.[0-2])
        compat_version=3.4.18
        ;;
    4.8.[3-9])
        compat_version=3.4.19
        ;;
    4.9.*|5.0.*)
        compat_version=3.4.20
        ;;
    5.[1-9].*|6.0.*)
        compat_version=3.4.21
        ;;
    6.[1-9].*|7.0*)
        compat_version=3.4.22
        ;;
    7.1.*)
        compat_version=3.4.23
        ;;
    7.[2-9].*)
        compat_version=3.4.24
        ;;
    8.*)
        compat_version=3.4.25
        ;;
    9.*)
        compat_version=3.4.26
        ;;
esac


BINARIES_TO_INSTALL=""
if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
mv "${TMP_BINARIES_PATH}/bin/Natron" "${TMP_BINARIES_PATH}/bin/Natron-bin"
mv "${TMP_BINARIES_PATH}/bin/NatronRenderer" "${TMP_BINARIES_PATH}/bin/NatronRenderer-bin"
mv "${TMP_BINARIES_PATH}/bin/NatronCrashReporter" "${TMP_BINARIES_PATH}/bin/Natron"
mv "${TMP_BINARIES_PATH}/bin/NatronRendererCrashReporter" "${TMP_BINARIES_PATH}/bin/NatronRenderer"
BINARIES_TO_INSTALL="${TMP_BINARIES_PATH}/bin/Natron ${TMP_BINARIES_PATH}/bin/NatronRenderer ${TMP_BINARIES_PATH}/bin/Natron-bin ${TMP_BINARIES_PATH}/bin/NatronRenderer-bin"
else
BINARIES_TO_INSTALL="${TMP_BINARIES_PATH}/bin/Natron ${TMP_BINARIES_PATH}/bin/NatronRenderer"
fi

if [ -f "${TMP_BINARIES_PATH}/bin/NatronProjectConverter" ]; then
BINARIES_TO_INSTALL="$BINARIES_TO_INSTALL ${TMP_BINARIES_PATH}/bin/NatronProjectConverter"
fi

if [ -f "${TMP_BINARIES_PATH}/bin/natron-python" ]; then
BINARIES_TO_INSTALL="$BINARIES_TO_INSTALL ${TMP_BINARIES_PATH}/bin/natron-python"
fi

BINARIES_TO_INSTALL="$BINARIES_TO_INSTALL ${SDK_HOME}/bin/iconvert ${SDK_HOME}/bin/idiff ${SDK_HOME}/bin/igrep ${SDK_HOME}/bin/iinfo"
BINARIES_TO_INSTALL="$BINARIES_TO_INSTALL ${SDK_HOME}/bin/exrheader ${SDK_HOME}/bin/tiffinfo"
BINARIES_TO_INSTALL="$BINARIES_TO_INSTALL ${FFMPEG_PATH}/bin/ffmpeg ${FFMPEG_PATH}/bin/ffprobe"

# We copy all files to both the portable archive and the package for the installer in a loop
COPY_LOCATIONS=("${TMP_PORTABLE_DIR}" "$NATRON_PACKAGE_PATH/data")

for location in "${COPY_LOCATIONS[@]}"; do

    mkdir -p "$location/docs" "$location/bin" "$location/Resources" "$location/Plugins/PyPlugs" "$location/Resources/stylesheets"
    cp -pPR "${TMP_BINARIES_PATH}/docs/natron"/* "$location/docs/"
    [ -f "$location/docs/TuttleOFX-README.txt" ] && rm "$location/docs/TuttleOFX-README.txt"
    cp -R "${TMP_BINARIES_PATH}/Resources/etc"  "$location/Resources/"
    cp "${TMP_BINARIES_PATH}/Resources/stylesheets"/mainstyle.qss "$location/Resources/stylesheets/"
    cp "$INC_PATH/natron/natron-mime.sh" "$location/bin/"
    cp "${TMP_BINARIES_PATH}/PyPlugs"/* "$location/Plugins/PyPlugs/"

    # OCIO -> has its own package, see below
    #cp -LR "${TMP_BINARIES_PATH}/Natron/OpenColorIO-Configs" "$location/Resources/"


    # Configure shell launch script with gcc compat version
    $GSED -e "s#${compat_version_script}#${compat_version}#" "$INC_PATH/scripts/Natron-Linux.sh" > "$location/Natron"
    $GSED -e "s#${compat_version_script}#${compat_version}#" -e "s#bin/Natron#bin/NatronRenderer#" "$INC_PATH/scripts/Natron-Linux.sh" > "$location/NatronRenderer"
    chmod a+x "$location/Natron" "$location/NatronRenderer"

    # Include in the portable version the test program that we will use later on
    if [ "$location" = "${TMP_PORTABLE_DIR}" ]; then
        cp "${TMP_BINARIES_PATH}/bin/Tests" "$location/bin/"
    fi


    for b in $BINARIES_TO_INSTALL; do
        cp "$b" "$location/bin/"
    done

    # If the binary contains upper case letters, make a symbolic link.
    # Solves https://github.com/NatronGitHub/Natron/issues/225
    for b in Natron NatronRenderer NatronProjectConverter; do
        if [ -f "$location/bin/$b" ]; then
            fname="$(basename "$b")"
            fnamel="$(echo "$fname" | tr '[:upper:]' '[:lower:]')" # https://stackoverflow.com/a/2264537
            if [ "$fname" != "$fnamel" ]; then
                ln -s "$fname" "$location/bin/$fnamel"
            fi
        fi
    done
    # end for all locations
done

# Dump symbols for crash reporting
if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    "${SDK_HOME}/bin"/dump_syms "${TMP_BINARIES_PATH}/bin/Natron" > "${BUILD_ARCHIVE_DIRECTORY}/symbols/Natron-${CURRENT_DATE}${BPAD_TAG:-}-${PKGOS_BITS}.sym"
    "${SDK_HOME}/bin"/dump_syms "${TMP_BINARIES_PATH}/bin/NatronRenderer" > "${BUILD_ARCHIVE_DIRECTORY}/symbols/NatronRenderer-${CURRENT_DATE}${BPAD_TAG:-}-${PKGOS_BITS}.sym"
fi

# Get all dependencies of the binaries
CORE_DEPENDS="$(ldd $(find "${TMP_PORTABLE_DIR}/bin" -maxdepth 1 -type f) | grep "$SDK_HOME" | awk '{print $3}'|sort|uniq)"

# icu libraries don't seem to be picked up by this ldd call above
pushd "${SDK_HOME}/lib"
for dep in {libicudata.so.*,libicui18n.so.*,libicuuc.so.*,libbz2.so.*,liblcms2.so.*,libcairo.so.*,libOpenColorIO.so.*}; do
    if [ -f "$dep" ]; then
        CORE_DEPENDS="$CORE_DEPENDS ${SDK_HOME}/lib/$dep"
    fi
done
popd


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
LIBS_PACKAGE_PATH="${INSTALLER_PATH}/packages/${CORELIBS_PKG}"
mkdir -p "$LIBS_PACKAGE_PATH/meta"

# Function to fix rpath of libraries in a list of folders
function fixrpath() {
    FOLDERS="$1"
    RPATH="$2"

    for folder in $FOLDERS; do
        (cd "$folder";
        for i in *; do
            if [ -f "$i" ]; then
                chmod u+w $i
                patchelf --force-rpath --set-rpath "\$ORIGIN${RPATH}" "$i" || true
                optlibs="$(ldd "$i" | grep "$SDK_HOME" | awk '{print $3}'|sort|uniq)"
                if [ ! -z "$optlibs" ]; then
                    for r in $optlibs; do
                        echo "Warning: runtime path remaining to $r for $folder/$i"
                    done
                fi
            fi
        done
    )
    done
}

# We copy all files to both the portable archive and the package for the installer in a loop
COPY_LOCATIONS=("${TMP_PORTABLE_DIR}" "$LIBS_PACKAGE_PATH/data")

for location in "${COPY_LOCATIONS[@]}"; do

    mkdir -p "${location}/bin" "${location}/lib" "${location}/Resources/pixmaps"
    #cp "${SDK_HOME}/qt${QT_VERSION_MAJOR}/lib/libQtDBus.so.4" "${location}/lib/"
    cp "${TMP_BINARIES_PATH}/Resources/pixmaps/natronIcon256_linux.png" "${location}/Resources/pixmaps/"
    cp "${TMP_BINARIES_PATH}/Resources/pixmaps/natronProjectIcon_linux.png" "${location}/Resources/pixmaps/"
    cp -pPR "${SDK_HOME}/share/poppler" "${location}/Resources/"
    cp -pPR "${SDK_HOME}/qt${QT_VERSION_MAJOR}/plugins"/* "${location}/bin/"

    # Copy dependencies
    for i in $CORE_DEPENDS; do
        dep=$(basename "$i")
        if [ ! -f "${location}/lib/$dep" ]; then
            cp -f "$i" "${location}/lib/"
        fi
    done

    # Copy dependencies of the libraries
    LIB_DEPENDS=$(ldd $(find "${location}/lib" -maxdepth 1 -type f) |grep "$SDK_HOME" | awk '{print $3}'|sort|uniq)
    for y in $LIB_DEPENDS; do
        dep=$(basename "$y")
        if [ ! -f "${location}/lib/$dep" ]; then
            cp -f "$y" "${location}/lib/"
        fi
    done

    # Qt plug-in dependencies
    QT_PLUG_DEPENDS=$(ldd $(find "${location}/bin" -maxdepth 2 -type f -name '*.so') | grep "$SDK_HOME" | awk '{print $3}'|sort|uniq)
    for z in $QT_PLUG_DEPENDS; do
        dep=$(basename "$z")
        if [ ! -f "${location}/lib/$dep" ]; then
            cp -f "$z" "${location}/lib/"
        fi
    done


    # Copy gcc compat libs
    #if [ -f "$INC_PATH/misc/compat${BITS}.tgz" ] && [ "$SDK_VERSION" = "CY2015" ]; then
    #    tar xvf "$INC_PATH/misc/compat${BITS}.tgz" -C "${location}/lib/"
    #fi


    # done in build-natron.sh
    #mkdir -p "${location}/Resources/etc/fonts/conf.d"
    #cp "${SDK_HOME}/etc/fonts/fonts.conf" "${location}/Resources/etc/fonts/"
    #cp "${SDK_HOME}/share/fontconfig/conf.avail"/* "${location}/Resources/etc/fonts/conf.d/"
    #$GSED -i "s#${SDK_HOME}/#/#;/conf.d/d" "${location}/Resources/etc/fonts/fonts.conf"

    # strip binaries
    if [ "$COMPILE_TYPE" != "debug" ]; then
        strip -s "${location}/bin"/* &>/dev/null || true
        strip -s "${location}/lib"/* &>/dev/null || true
        strip -s "${location}/bin"/*/* &>/dev/null || true
    fi

    if [ ! -d "${location}/Plugins" ]; then
        mkdir -p "${location}/Plugins"
    fi

    # end for all locations
done

# Copy Python distrib
# Remove it if it existed already (with DEBUG_SCRIPTS=1)
if [ -d "${TMP_PORTABLE_DIR}/lib/python${PYVER}" ]; then
    rm -rf "${TMP_PORTABLE_DIR}/lib/python${PYVER}"
fi

# The whitelist of python site-packages:
#python_site_packages=(easy_install.py easy_install.pyc pip pkg_resources PySide README setuptools wheel shiboken.so)
# Note that pip and dependencies were already installed by get-pip.py
python_site_packages=(PySide shiboken.so)

mkdir -p "${TMP_PORTABLE_DIR}/lib/python${PYVER}"

for pydir in "${SDK_HOME}/lib/python${PYVER}" "${SDK_HOME}/qt${QT_VERSION_MAJOR}/lib/python${PYVER}"; do
    (cd "$pydir"; tar cf - . --exclude site-packages)|(cd "${TMP_PORTABLE_DIR}/lib/python${PYVER}"; tar xf -)
    for p in "${python_site_packages[@]}"; do
        if [ -e "$pydir/site-packages/$p" ]; then
            (cd "$pydir"; tar cf - "site-packages/$p") | (cd "${TMP_PORTABLE_DIR}/lib/python${PYVER}"; tar xf -)
        fi
    done
done

# Move PySide to plug-ins directory and keep a symbolic link in site-packages
mv "${TMP_PORTABLE_DIR}/lib/python${PYVER}/site-packages/PySide" "${TMP_PORTABLE_DIR}/Plugins/"
(cd "${TMP_PORTABLE_DIR}/lib/python${PYVER}/site-packages"; ln -sf "../../../Plugins/PySide" . )

# Remove unused stuff
rm -rf "${TMP_PORTABLE_DIR}/lib/python${PYVER}"/{test,config,config-"${PYVER}m"}

# Copy PySide dependencies
PYSIDE_DEPENDS=$(ldd $(find "${SDK_HOME}/qt${QT_VERSION_MAJOR}/lib/python${PYVER}/site-packages/PySide" -maxdepth 1 -type f) | grep "$SDK_HOME" | awk '{print $3}'|sort|uniq)
for y in $PYSIDE_DEPENDS; do
    dep=$(basename "$y")
    if [ ! -f "${TMP_PORTABLE_DIR}/lib/$dep" ]; then
        cp -f "$y" "${TMP_PORTABLE_DIR}/lib/"
    fi
done

# Remove any pycache
(cd "${TMP_PORTABLE_DIR}" ; find . -type d -name __pycache__ -exec rm -rf {} \;)

# Strip pyside and python depends
if [ "${COMPILE_TYPE}" != "debug" ]; then
    strip -s "${TMP_PORTABLE_DIR}/Plugins/PySide"/* "${TMP_PORTABLE_DIR}/lib"/python*/* "${TMP_PORTABLE_DIR}/lib"/python*/*/* &>/dev/null || true
fi

# Remove pyo files and prune unneeded files using python-exclude file
PYDIR="${TMP_PORTABLE_DIR}/lib/python${PYVER:-}"
find "${PYDIR}" -type f -name '*.pyo' -exec rm {} \;
(cd "${PYDIR}"; xargs rm -rf || true) < "$INC_PATH/python-exclude.txt"

# python zip
if [ "${USE_QT5:-}" != 1 ]; then
    rm -rf "$PYDIR"/site-packages/shiboken2* "$PYDIR"/site-packages/PySide2 || true
fi

fixrpath "${TMP_PORTABLE_DIR}/Plugins/PySide" "/../../lib"
fixrpath "${TMP_PORTABLE_DIR}/lib/python${PYVER:-}/lib-dynload ${TMP_PORTABLE_DIR}/lib/python${PYVER:-}/site-packages" "/../.."

export PY_BIN="${SDK_HOME}/bin/python${PYVER:-}"
export PYDIR="$PYDIR"
. "$CWD"/zip-python.sh

# Install pip
if [ -x "${TMP_PORTABLE_DIR}"/bin/natron-python ]; then
    $CURL --remote-name --insecure https://bootstrap.pypa.io/pip/2.7/get-pip.py
    "${TMP_PORTABLE_DIR}"/bin/natron-python get-pip.py
    rm get-pip.py
fi

# Run extra user provided pip install scripts
if [ -f "${EXTRA_PYTHON_MODULES_SCRIPT:-}" ]; then
    "${TMP_PORTABLE_DIR}"/bin/natron-python "$EXTRA_PYTHON_MODULES_SCRIPT" || true
fi


# Copy Python distrib to installer package
cp -pPR "$PYDIR" "$LIBS_PACKAGE_PATH/data/lib/"
cp "${TMP_PORTABLE_DIR}"/lib/python*.zip "${LIBS_PACKAGE_PATH}/data/lib/"
mkdir -p "$LIBS_PACKAGE_PATH/data/Plugins/"
cp -pPR "${TMP_PORTABLE_DIR}/Plugins/PySide" "$LIBS_PACKAGE_PATH/data/Plugins/"

# Fix RPATH (we don't want to link against system libraries when deployed)
for location in "$LIBS_PACKAGE_PATH/data" "${TMP_PORTABLE_DIR}"; do
    fixrpath "${location}/bin" "/../lib"
    fixrpath "${location}/lib" ""
    BIN_SUBDIRS=$(find "${location}/bin" -type d ! -path "${location}/bin")
    fixrpath "$BIN_SUBDIRS" "/../../lib"
done


ALL_NATRON_LIBS=$(ls "$LIBS_PACKAGE_PATH/data/lib"/*.so*)

PACKAGES="${PACKAGES},fr.inria.openfx.io"
installPlugin "IO" "fr.inria.openfx.io" "$XML/openfx-io.xml" "$QS/openfx-io.qs" "$ALL_NATRON_LIBS"
PACKAGES="${PACKAGES},fr.inria.openfx.misc"
installPlugin "Misc" "fr.inria.openfx.misc" "$XML/openfx-misc.xml" "$QS/openfx-misc.qs" "$ALL_NATRON_LIBS"
installPlugin "CImg" "fr.inria.openfx.misc" "$XML/openfx-misc.xml" "$QS/openfx-misc.qs" "$ALL_NATRON_LIBS"
installPlugin "Shadertoy" "fr.inria.openfx.misc" "$XML/openfx-misc.xml" "$QS/openfx-misc.qs" "$ALL_NATRON_LIBS"
PACKAGES="${PACKAGES},fr.inria.openfx.extra"
installPlugin "Arena" "fr.inria.openfx.extra" "$XML/openfx-arena.xml" "$QS/openfx-arena.qs" "$ALL_NATRON_LIBS"
installPlugin "ArenaCL" "fr.inria.openfx.extra" "$XML/openfx-arena.xml" "$QS/openfx-arena.qs" "$ALL_NATRON_LIBS"
PACKAGES="${PACKAGES},fr.inria.openfx.gmic"
installPlugin "GMIC" "fr.inria.openfx.gmic" "$XML/openfx-gmic.xml" "$QS/openfx-gmic.qs" "$ALL_NATRON_LIBS"


# Configure the package date using the most recent library modification date
CLIBS_VERSION="00000000000000"
for lib in $ALL_NATRON_LIBS; do
    LAST_MODIFICATION_DATE=$(date -u -r "$lib" "+%Y%m%d%H%M%S")
    if [ "$LAST_MODIFICATION_DATE" -gt "$CLIBS_VERSION" ]; then
        CLIBS_VERSION="$LAST_MODIFICATION_DATE"
    fi
done
$GSED "s/_VERSION_/${CLIBS_VERSION}/;s/_DATE_/${INSTALLER_XML_DATE}/" "$XML/corelibs.xml" > "$LIBS_PACKAGE_PATH/meta/package.xml"
cat "$QS/$PKGOS/corelibs.qs" > "$LIBS_PACKAGE_PATH/meta/installscript.qs"

# Generate documentation
bash "$CWD"/gen-natron-doc.sh

# Copy documentation installed in the portable dir to installer package
if [ -d "$NATRON_PACKAGE_PATH/data/Resources/docs" ]; then
    rm -rf "$NATRON_PACKAGE_PATH/data/Resources/docs"
fi
if [ -d "${TMP_PORTABLE_DIR}/Resources/docs" ]; then
    cp -R "${TMP_PORTABLE_DIR}/Resources/docs" "$NATRON_PACKAGE_PATH/data/Resources/"
fi


# At this point we can run Natron unit tests to check that the deployment is ok.
rm -rf "$HOME/.cache/INRIA/Natron"* &> /dev/null || true
$TIMEOUT -s KILL 1800 valgrind --tool=memcheck --suppressions="$INC_PATH/natron/valgrind-python.supp" "${TMP_PORTABLE_DIR}/bin/Tests"
rm "${TMP_PORTABLE_DIR}/bin/Tests"

# Clean and perms
(cd "${INSTALLER_PATH}"; find . -type d -name .git -exec rm -rf {} \;)

# Build repo and packages

ONLINE_INSTALL_DIR="online_installer"
#BUNDLED_INSTALL_DIR="offline_installer"
#ZIP_INSTALL_DIR="compressed_no_installer"
#DEB_INSTALL_DIR="deb_package"
#RPM_INSTALL_DIR="rpm_package"
# These have easily identifiable filenames and extensions anyway:
BUNDLED_INSTALL_DIR="."
ZIP_INSTALL_DIR="."
DEB_INSTALL_DIR="."
RPM_INSTALL_DIR="."

if [ -d "${BUILD_ARCHIVE_DIRECTORY}" ]; then
    rm -rf "${BUILD_ARCHIVE_DIRECTORY}"
fi

mkdir -p "${BUILD_ARCHIVE_DIRECTORY}/$BUNDLED_INSTALL_DIR"

if [ "$DISABLE_PORTABLE_ARCHIVE" != "1" ]; then
    # Portable archive
    mkdir -p "${BUILD_ARCHIVE_DIRECTORY}/$ZIP_INSTALL_DIR"
    (cd "${TMP_BINARIES_PATH}" && tar Jcf "${PORTABLE_DIRNAME}.tar.xz" "${PORTABLE_DIRNAME}"; mv "${PORTABLE_DIRNAME}.tar.xz" "${BUILD_ARCHIVE_DIRECTORY}/$ZIP_INSTALL_DIR/${PORTABLE_DIRNAME}.tar.xz")
fi

if [ "$WITH_ONLINE_INSTALLER" = "1" ]; then
    mkdir -p "${BUILD_ARCHIVE_DIRECTORY}/$ONLINE_INSTALL_DIR"
    mkdir -p "${BUILD_ARCHIVE_DIRECTORY}/$ONLINE_INSTALL_DIR/packages"
    # Gen online repo
    "${SDK_HOME}/installer/bin"/repogen -v --update-new-components -p "${INSTALLER_PATH}/packages" -c "${INSTALLER_PATH}/config/config.xml" "${BUILD_ARCHIVE_DIRECTORY}/$ONLINE_INSTALL_DIR/packages"

    # Online installer
    echo "*** Creating online installer ${BUILD_ARCHIVE_DIRECTORY}/$ONLINE_INSTALL_DIR/$INSTALLER_BASENAME"
    "${SDK_HOME}/installer/bin"/binarycreator -v -n -p "${INSTALLER_PATH}/packages" -c "${INSTALLER_PATH}/config/config.xml" "${BUILD_ARCHIVE_DIRECTORY}/$ONLINE_INSTALL_DIR/${INSTALLER_BASENAME}-online"
    (cd "${BUILD_ARCHIVE_DIRECTORY}/$ONLINE_INSTALL_DIR" && tar zcf "${INSTALLER_BASENAME}-online.tgz" "${INSTALLER_BASENAME}-online" && rm "${INSTALLER_BASENAME}-online")
fi

# Offline installer
echo "*** Creating offline installer ${BUILD_ARCHIVE_DIRECTORY}/$BUNDLED_INSTALL_DIR/$INSTALLER_BASENAME"
"${SDK_HOME}/installer/bin"/binarycreator -v -f -p "${INSTALLER_PATH}/packages" -c "${INSTALLER_PATH}/config/config.xml" -i "$PACKAGES" "${BUILD_ARCHIVE_DIRECTORY}/$BUNDLED_INSTALL_DIR/$INSTALLER_BASENAME"
(cd "${BUILD_ARCHIVE_DIRECTORY}/$BUNDLED_INSTALL_DIR" && tar zcf "${INSTALLER_BASENAME}.tgz" "$INSTALLER_BASENAME" && rm "$INSTALLER_BASENAME")



# collect debug versions for gdb
#if [ "$NATRON_BUILD_CONFIG" = "STABLE" ]; then
#    DEBUG_DIR=${INSTALLER_PATH}/Natron-$NATRON_VERSION_STRING-Linux${BITS}-Debug
#    rm -rf "$DEBUG_DIR"
#    mkdir "$DEBUG_DIR"
#    cp -pPR "${SDK_HOME}/bin"/Natron* "$DEBUG_DIR/"
#    cp -pPR "${SDK_HOME}/Plugins"/*.ofx.bundle/Contents/Linux*/*.ofx "$DEBUG_DIR/"
#    ( cd "${INSTALLER_PATH}"; tar Jcf "Natron-$NATRON_VERSION_STRING-Linux${BITS}-Debug.tar.xz" "Natron-$NATRON_VERSION-Linux${BITS}-Debug" )
#    mv "${DEBUG_DIR}.tar.xz" "$BUILD_ARCHIVE"/
#fi

# Build native packages for linux


if ( [ "$NATRON_BUILD_CONFIG" = "RELEASE" ] || [ "$NATRON_BUILD_CONFIG" = "STABLE" ] ) && [ "$DISABLE_RPM_DEB_PKGS" != "1" ]; then
    # rpm
    echo "*** Creating rpm"
    mkdir -p "${BUILD_ARCHIVE_DIRECTORY}/$RPM_INSTALL_DIR"

    if [ ! -f "/usr/bin/rpmbuild" ]; then
        if [ $EUID -ne 0 ]; then
            echo "Error: rpmdevtools not installed, please run: sudo yum install -y rpmdevtools"
            exit 2
        else
            yum install -y rpmdevtools
        fi
    fi
    rm -rf ~/rpmbuild/*
    if $(gpg --list-keys | fgrep build@natron.fr > /dev/null); then
        echo "Info: gpg key for build@natron.fr found, all is right"
    else
        echo "Error: gpg key for build@natron.fr not found"
        exit
    fi
    $GSED "s/REPLACE_VERSION/$(echo "$NATRON_VERSION_STRING" | $GSED 's/-/./g')/;s#__NATRON_INSTALLER__#${INSTALLER_PATH}#;s#__INC__#${INC_PATH}#;s#__TMP_BINARIES_PATH__#${TMP_BINARIES_PATH}#" "$INC_PATH/natron/Natron.spec" > "$TMP_PATH/Natron.spec"
    #Only need to build once, so uncomment as default #echo "" | setsid rpmbuild -bb --define="%_gpg_name build@natron.fr" --sign $INC_PATH/natron/Natron-repo.spec
    echo "" | setsid rpmbuild -bb --define="%_gpg_name build@natron.fr" --sign "$TMP_PATH/Natron.spec"
    mv ~/rpmbuild/RPMS/*/Natron*.rpm "${BUILD_ARCHIVE_DIRECTORY}/$RPM_INSTALL_DIR/"


    # deb
    echo "*** Creating deb"
    mkdir -p "${BUILD_ARCHIVE_DIRECTORY}/$DEB_INSTALL_DIR"

    if [ ! -f "/usr/bin/dpkg-deb" ]; then
        if [ $EUID -ne 0 ]; then
            echo "Error: dpkg-dev not installed, please run: sudo yum install -y dpkg-dev"
            exit 2
        else
            yum install -y dpkg-dev
        fi
    fi

    rm -rf "${INSTALLER_PATH}/natron"
    mkdir -p "${INSTALLER_PATH}/natron"
    cd "${INSTALLER_PATH}/natron"
    mkdir -p opt/Natron2 DEBIAN usr/share/doc/natron usr/share/{applications,pixmaps} usr/share/mime/packages usr/bin

    cp -pPR "${INSTALLER_PATH}/packages"/fr.inria.*/data/* opt/Natron2/
    cp "$INC_PATH/debian"/post* DEBIAN/
    chmod +x DEBIAN/post*

    if [ "${BITS}" = "64" ]; then
        DEB_ARCH=amd64
    else
        DEB_ARCH=i386
    fi  
    DEB_VERSION=$(echo "$NATRON_VERSION_STRING" | $GSED 's/-/./g')
    DEB_DATE=$(date -u +"%a, %d %b %Y %T %z")
    DEB_SIZE=$(du -ks opt|cut -f 1)
    DEB_PKG="natron_${DEB_VERSION}_${DEB_ARCH}.deb"
    
    cat "$INC_PATH/debian/copyright" > usr/share/doc/natron/copyright
    $GSED "s/__VERSION__/${DEB_VERSION}/;s/__ARCH__/${DEB_ARCH}/;s/__SIZE__/${DEB_SIZE}/" "$INC_PATH/debian/control" > DEBIAN/control
    $GSED "s/__VERSION__/${DEB_VERSION}/;s/__DATE__/${DEB_DATE}/" "$INC_PATH/debian/changelog.Debian" > changelog.Debian
    gzip changelog.Debian
    mv changelog.Debian.gz usr/share/doc/natron/
    
    cat "$INC_PATH/natron/Natron2.desktop" > usr/share/applications/Natron2.desktop
    cat "$INC_PATH/natron/x-natron.xml" > usr/share/mime/packages/x-natron.xml
    cp "${TMP_BINARIES_PATH}/Resources/pixmaps/natronIcon256_linux.png" usr/share/pixmaps/
    cp "${TMP_BINARIES_PATH}/Resources/pixmaps/natronProjectIcon_linux.png" usr/share/pixmaps/
    
    (cd usr/bin; ln -sf ../../opt/Natron2/Natron .)
    (cd usr/bin; ln -sf ../../opt/Natron2/NatronRenderer .)

    # why?
    #chown root:root -R "${INSTALLER_PATH}/natron"

    cd "${INSTALLER_PATH}"
    dpkg-deb -Zxz -z9 --build natron
    mv natron.deb "${DEB_PKG}"
    mv "${DEB_PKG}" "${BUILD_ARCHIVE_DIRECTORY}/$DEB_INSTALL_DIR/"
fi

echo "*** Artifacts:"
ls -R  "${BUILD_ARCHIVE_DIRECTORY}"
echo "*** Linux installer: done!"

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
