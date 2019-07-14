#!/bin/bash
#
# Build packages and installer for Windows
#
# Options:
# NATRON_LICENSE=(GPL,COMMERCIAL)
# BUILD_CONFIG=(SNAPSHOT,ALPHA,BETA,RC,STABLE,CUSTOM)
# CUSTOM_BUILD_USER_NAME="Toto" : to be set if BUILD_CONFIG=CUSTOM
# BUILD_NUMBER=X: To be set to indicate the revision number of the build. For example RC1,RC2, RC3 etc...
# DISABLE_BREAKPAD=1: Disable automatic crash report
# NO_ZIP=1: Do not produce a zip self-contained archive with Natron distribution.
# OFFLINE=1 : Make the offline installer too
# NO_INSTALLER: Do not make any installer, only zip if NO_ZIP!=1
# Usage:
# NO_ZIP=1 NATRON_LICENSE=GPL OFFLINE=1 BUILD_CONFIG=BETA BUILD_NUMBER=1 sh build-installer 64

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

source common.sh
source common-buildmaster.sh

if [ -z "${BUILD_CONFIG:-}" ]; then
    echo "Please define BUILD_CONFIG"
    exit 1
fi
NATRON_BRANCH=${GIT_BRANCH:-}
if [ -z "${NATRON_BRANCH:-}" ] && [ "$BUILD_CONFIG" = "SNAPSHOT" ]; then
    echo "No branch selected, using MASTER"
    NATRON_BRANCH=$MASTER_BRANCH
elif [ -z "${NATRON_BRANCH:-}" ] && [ "$BUILD_CONFIG" = "STABLE" ]; then
    echo "No branch selected, using NATRON_RELEASE_BRANCH"
    NATRON_BRANCH=$NATRON_RELEASE_BRANCH
fi
if [ "$BUILD_CONFIG" = "SNAPSHOT" ]; then
    if [ -f "commits-hash-$NATRON_BRANCH.sh" ]; then
        source "commits-hash-$NATRON_BRANCH.sh"
    fi
else
    if [ -f commits-hash.sh ]; then
        source commits-hash.sh
    fi
fi


if [ "$NATRON_LICENSE" = "GPL" ]; then
    FFMPEG_BIN_PATH=$SDK_HOME/ffmpeg-gpl2
elif [ "$NATRON_LICENSE" = "COMMERCIAL" ]; then
    FFMPEG_BIN_PATH=$SDK_HOME/ffmpeg-lgpl
fi

if [ -d "${TMP_BINARIES_PATH}/symbols" ]; then
    rm -rf ${TMP_BINARIES_PATH}/symbols
fi
mkdir -p ${TMP_BINARIES_PATH}/symbols


if [ -z "${BUNDLE_IO:-}" ]; then
    BUNDLE_IO=1
fi
if [ -z "${BUNDLE_MISC:-}" ]; then
    BUNDLE_MISC=1
fi
if [ -z "${BUNDLE_ARENA:-}" ]; then
    BUNDLE_ARENA=1
fi
if [ -z "${BUNDLE_GMIC:-}" ]; then
    BUNDLE_GMIC=1
fi
BUNDLE_CV=0


if [ "$BUILD_CONFIG" = "ALPHA" ]; then
    if [ -z "${BUILD_NUMBER:-}" ]; then
	echo "You must supply a BUILD_NUMBER when BUILD_CONFIG=ALPHA"
	exit 1
    fi
    NATRON_VERSION=$NATRON_VERSION_NUMBER-alpha-$BUILD_NUMBER
elif [ "$BUILD_CONFIG" = "BETA" ]; then
    if [ -z "${BUILD_NUMBER:-}" ]; then
	echo "You must supply a BUILD_NUMBER when BUILD_CONFIG=BETA"
	exit 1
    fi
    NATRON_VERSION=$NATRON_VERSION_NUMBER-beta-$BUILD_NUMBER
elif [ "$BUILD_CONFIG" = "RC" ]; then
    if [ -z "${BUILD_NUMBER:-}" ]; then
	echo "You must supply a BUILD_NUMBER when BUILD_CONFIG=RC"
	exit 1
    fi
    NATRON_VERSION=$NATRON_VERSION_NUMBER-RC$BUILD_NUMBER
elif [ "$BUILD_CONFIG" = "STABLE" ]; then
    NATRON_VERSION=$NATRON_VERSION_NUMBER
elif [ "$BUILD_CONFIG" = "CUSTOM" ]; then
    if [ -z "${CUSTOM_BUILD_USER_NAME:-}" ]; then
	echo "You must supply a CUSTOM_BUILD_USER_NAME when BUILD_CONFIG=CUSTOM"
	exit 1
    fi
    NATRON_VERSION="$CUSTOM_BUILD_USER_NAME"
fi

if [ "$BUILD_CONFIG" = "SNAPSHOT" ]; then
    NATRON_VERSION="${NATRON_DEVEL_GIT:0:7}"
    REPO_BRANCH="snapshots/${NATRON_BRANCH}"
    REPO_BRANCH_LATEST="snapshots/latest"
    APP_INSTALL_SUFFIX="INRIA/Natron-snapshots"
    ONLINE_TAG="snapshot"
    REPO_SUFFIX="snapshot-${NATRON_BRANCH}"
else
    REPO_BRANCH="releases/${NATRON_BRANCH}"
    REPO_BRANCH_LATEST="releases/latest"
    APP_INSTALL_SUFFIX="INRIA/Natron-${NATRON_VERSION}"
    ONLINE_TAG="release"
    REPO_SUFFIX="release" #-$NATRON_BRANCH
fi

REPO_DIR=""
if [ "${QID:-}" != "" ]; then
    REPO_DIR="${QUEUE_TMP_PATH}${QID}_${BITS}"
else
    REPO_DIR="$REPO_DIR_PREFIX$REPO_SUFFIX${BITS}"
fi

if [ "$BITS" = 64 ]; then
    DUMP_SYMS="$SDK_HOME/bin/dump_syms.exe"
else
    DUMP_SYMS="$SDK_HOME/dump_syms_x64/dump_syms.exe"
fi

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

ARCHIVE_DIR="$REPO_DIR/archive"
ARCHIVE_DATA_DIR="$ARCHIVE_DIR/data"
rm -rf "$REPO_DIR/packages" "$REPO_DIR/installers" "$ARCHIVE_DATA_DIR"
mkdir -p "$REPO_DIR/packages"
mkdir -p "$REPO_DIR/installers"

DATE=$(date "+%Y-%m-%d")
PKGOS_ORIG="$PKGOS"
PKGOS="Windows-x86_${BITS}bit"
#REPO_OS="Windows/$REPO_BRANCH/${BITS}bit/packages"
REPO_OS="packages/Windows/${BITS}/${REPO_BRANCH}"
REPO_OS_LATEST="packages/Windows/${BITS}/${REPO_BRANCH_LATEST}"

TMP_BUILD_DIR=$TMP_PATH

if [ -d "$TMP_BUILD_DIR" ]; then
    rm -rf "$TMP_BUILD_DIR"
fi
mkdir -p "$TMP_BUILD_DIR"


# tag symbols we want to keep with 'release'
if [ "$BUILD_CONFIG" != "" ] && [ "$BUILD_CONFIG" != "SNAPSHOT" ]; then
    BPAD_TAG="-release"
    if [ -f "${TMP_BINARIES_PATH}/natron-fullversion.txt" ]; then
        GET_VER=$(cat "${TMP_BINARIES_PATH}/natron-fullversion.txt")
        if [ "$GET_VER" != "" ]; then
            TAG=$GET_VER
        fi
    fi
fi

# SETUP
INSTALLER_PATH="$TMP_BUILD_DIR/Natron-installer"
XML="$INC_PATH/xml"
QS="$INC_PATH/qs"

mkdir -p "${INSTALLER_PATH}/config" "${INSTALLER_PATH}/packages"
$GSED -e "s/_VERSION_/${NATRON_VERSION_NUMBER}/;s#_RBVERSION_#${NATRON_BRANCH}#g;s#_OS_BRANCH_LATEST_#${REPO_OS_LATEST}#g;s#_OS_BRANCH_BIT_#${REPO_OS}#g;s#_URL_#${REPO_URL}#g;s#_APP_INSTALL_SUFFIX_#${APP_INSTALL_SUFFIX}#g" < "$INC_PATH/config/$PKGOS_ORIG.xml" > "${INSTALLER_PATH}/config/config.xml"
cp "$INC_PATH/config/"*.png "${INSTALLER_PATH}/config/"


# OFX IO
if [ "${BUNDLE_IO:-}" = "1" ]; then         

    OFX_IO_VERSION="$TAG"
    OFX_IO_PATH="${INSTALLER_PATH}/packages/$IOPLUG_PKG"
    mkdir -p "$OFX_IO_PATH/data" "$OFX_IO_PATH/meta" "$OFX_IO_PATH/data/Plugins/OFX/Natron"
    $GSED -e "s/_VERSION_/${OFX_IO_VERSION}/;s/_DATE_/${DATE}/" < "$XML/openfx-io.xml" > "$OFX_IO_PATH/meta/package.xml"
    cp "$QS/openfx-io.qs" "$OFX_IO_PATH/meta/installscript.qs"
    #cat "${TMP_BINARIES_PATH}/docs/openfx-io/VERSION" > "$OFX_IO_PATH/meta/ofx-io-license.txt"
    #echo "" >> "$OFX_IO_PATH/meta/ofx-io-license.txt"
    #cat "${TMP_BINARIES_PATH}/docs/openfx-io/LICENSE" >> "$OFX_IO_PATH/meta/ofx-io-license.txt"
    cp -a "${TMP_BINARIES_PATH}/OFX/Plugins/IO.ofx.bundle" "$OFX_IO_PATH/data/Plugins/OFX/Natron/"
    #for depend in "${IO_DLL[@]}"; do
    #    cp "$SDK_HOME/bin/$depend" "$OFX_IO_PATH/data/Plugins/OFX/Natron/IO.ofx.bundle/Contents/Win${BITS}/"
    #done
    #OpenColorIO and SeExpr are located in $SDK_HOME/lib and not bin
    #cp $SDK_HOME/lib/{libopencolorio.dll,libseexpr.dll} "$OFX_IO_PATH/data/Plugins/OFX/Natron/IO.ofx.bundle/Contents/Win${BITS}/"
    

    if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
        dump_syms_safe "$OFX_IO_PATH/data/Plugins/OFX/Natron"/*/*/*/IO.ofx "${TMP_BINARIES_PATH}/symbols/IO.ofx-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
    fi
    if [ "${DEBUG_MODE:-}" != "1" ]; then
        for dir in "$OFX_IO_PATH/data/Plugins/OFX/Natron"; do
            echo "*** stripping binaries in $dir"
            find "$dir" -type f \( -iname '*.exe' -o -iname '*.dll' -o -iname '*.pyd' -o -iname '*.ofx' \) -exec strip -s {} \;
            echo "*** stripping binaries in $dir... done!"
        done
    fi
    echo "*** $OFX_IO_PATH contents:"
    echo "========================================================================"
    ls -R "$OFX_IO_PATH"
    echo "========================================================================"
fi

# OFX MISC
if [ "${BUNDLE_MISC:-}" = "1" ]; then 
    OFX_MISC_VERSION="$TAG"
    OFX_MISC_PATH="${INSTALLER_PATH}/packages/$MISCPLUG_PKG"
    mkdir -p "$OFX_MISC_PATH/data" "$OFX_MISC_PATH/meta" "$OFX_MISC_PATH/data/Plugins/OFX/Natron"
    $GSED "s/_VERSION_/${OFX_MISC_VERSION}/;s/_DATE_/${DATE}/" < "$XML/openfx-misc.xml" > "$OFX_MISC_PATH/meta/package.xml"
    cp "$QS/openfx-misc.qs" "$OFX_MISC_PATH/meta/installscript.qs"
    #cat "${TMP_BINARIES_PATH}/docs/openfx-misc/VERSION" > "$OFX_MISC_PATH/meta/ofx-misc-license.txt"
    #echo "" >> "$OFX_MISC_PATH/meta/ofx-misc-license.txt"
    #cat "${TMP_BINARIES_PATH}/docs/openfx-misc/LICENSE" >> "$OFX_MISC_PATH/meta/ofx-misc-license.txt"
    cp -a "${TMP_BINARIES_PATH}/OFX/Plugins"/{CImg,Misc}.ofx.bundle "$OFX_MISC_PATH/data/Plugins/OFX/Natron/"

    if [ -d "${TMP_BINARIES_PATH}/OFX/Plugins/ShaderToy.ofx.bundle"  ]; then
        cp -a "${TMP_BINARIES_PATH}/OFX/Plugins/Shadertoy.ofx.bundle" "$OFX_MISC_PATH/data/Plugins/OFX/Natron/"
    fi    

    if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
        dump_syms_safe "$OFX_MISC_PATH/data/Plugins/OFX/Natron"/*/*/*/CImg.ofx "${TMP_BINARIES_PATH}/symbols/CImg.ofx-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
        "$SDK_HOME/bin/dump_syms.exe" "$OFX_MISC_PATH/data/Plugins/OFX/Natron"/*/*/*/Misc.ofx > "${TMP_BINARIES_PATH}/symbols/Misc.ofx-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
        if [ -d "$OFX_MISC_PATH/data/Plugins/OFX/Natron/Shadertoy.ofx.bundle" ]; then
            dump_syms_safe "$OFX_MISC_PATH/data/Plugins/OFX/Natron"/*/*/*/Shadertoy.ofx "${TMP_BINARIES_PATH}/symbols/Shadertoy.ofx-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
        fi
    fi
    if [ "${DEBUG_MODE:-}" != "1" ]; then
        for dir in "$OFX_MISC_PATH/data/Plugins/OFX/Natron"; do
            echo "*** stripping binaries in $dir"
            find "$dir" -type f \( -iname '*.exe' -o -iname '*.dll' -o -iname '*.pyd' -o -iname '*.ofx' \) -exec strip -s {} \;
            echo "*** stripping binaries in $dir... done!"
        done
    fi
    echo "*** $OFX_MISC_PATH contents:"
    echo "========================================================================"
    ls -R "$OFX_MISC_PATH"
    echo "========================================================================"
fi

# NATRON
NATRON_PACKAGE_PATH="${INSTALLER_PATH}/packages/$NATRON_PKG"
mkdir -p "$NATRON_PACKAGE_PATH/meta" "$NATRON_PACKAGE_PATH/data/docs" "$NATRON_PACKAGE_PATH/data/bin" "$NATRON_PACKAGE_PATH/data/Resources" "$NATRON_PACKAGE_PATH/data/Plugins/PyPlugs"
$GSED "s/_VERSION_/${TAG}/;s/_DATE_/${DATE}/" < "$XML/natron.xml" > "$NATRON_PACKAGE_PATH/meta/package.xml"
cp "$QS/$PKGOS_ORIG/natron.qs" "$NATRON_PACKAGE_PATH/meta/installscript.qs"
cp -a "${TMP_BINARIES_PATH}/docs/natron"/* "$NATRON_PACKAGE_PATH/data/docs/"
cat "${TMP_BINARIES_PATH}/docs/natron/LICENSE.txt" > "$NATRON_PACKAGE_PATH/meta/natron-license.txt"

cp "${TMP_BINARIES_PATH}/PyPlugs"/* "$NATRON_PACKAGE_PATH/data/Plugins/PyPlugs/"

if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    cp "${TMP_BINARIES_PATH}/bin/Natron.exe" "$NATRON_PACKAGE_PATH/data/bin/Natron-bin.exe"
    cp "${TMP_BINARIES_PATH}/bin/NatronRenderer.exe" "$NATRON_PACKAGE_PATH/data/bin/NatronRenderer-bin.exe"
    cp "${TMP_BINARIES_PATH}/bin/NatronCrashReporter.exe" "$NATRON_PACKAGE_PATH/data/bin/Natron.exe"
    cp "${TMP_BINARIES_PATH}/bin/NatronRendererCrashReporter.exe" "$NATRON_PACKAGE_PATH/data/bin/NatronRenderer.exe"
else
    cp "${TMP_BINARIES_PATH}/bin/Natron.exe" "$NATRON_PACKAGE_PATH/data/bin/Natron.exe"
    cp "${TMP_BINARIES_PATH}/bin/NatronRenderer.exe" "$NATRON_PACKAGE_PATH/data/bin/NatronRenderer.exe"
fi

if [ -f "${TMP_BINARIES_PATH}/bin/NatronProjectConverter.exe" ]; then
    cp "${TMP_BINARIES_PATH}/bin/NatronProjectConverter.exe" "$NATRON_PACKAGE_PATH/data/bin/NatronProjectConverter.exe"
fi

if [ -f "${TMP_BINARIES_PATH}/bin/natron-python.exe" ]; then
    cp "${TMP_BINARIES_PATH}/bin/natron-python.exe" "$NATRON_PACKAGE_PATH/data/bin/natron-python.exe"
fi

cp "$SDK_HOME"/bin/{iconvert.exe,idiff.exe,igrep.exe,iinfo.exe} "$NATRON_PACKAGE_PATH/data/bin/"
cp "$SDK_HOME"/bin/{exrheader.exe,tiffinfo.exe} "$NATRON_PACKAGE_PATH/data/bin/"
cp "$FFMPEG_BIN_PATH"/bin/{ffmpeg.exe,ffprobe.exe} "$NATRON_PACKAGE_PATH/data/bin/"

#rm "$NATRON_PACKAGE_PATH"/data/docs/TuttleOFX-README.txt
mkdir -p "$NATRON_PACKAGE_PATH"/data/Resources/{pixmaps,stylesheets}
cp "$CWD/include/config/natronProjectIcon_windows.ico" "$NATRON_PACKAGE_PATH/data/Resources/pixmaps/"
cp "${TMP_BINARIES_PATH}/Resources/stylesheets/mainstyle.qss" "$NATRON_PACKAGE_PATH/data/Resources/stylesheets/"

# Docs
mkdir -p "$NATRON_PACKAGE_PATH/data/Resources/docs"
cp -a "${TMP_BINARIES_PATH}/Resources/docs/html" "$NATRON_PACKAGE_PATH/data/Resources/docs/"

if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    dump_syms_safe "$NATRON_PACKAGE_PATH/data/bin/Natron-bin.exe" "${TMP_BINARIES_PATH}/symbols/Natron-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
    dump_syms_safe "$NATRON_PACKAGE_PATH/data/bin/NatronRenderer-bin.exe" "${TMP_BINARIES_PATH}/symbols/NatronRenderer-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
fi


if [ "${DEBUG_MODE:-}" != "1" ]; then
    for dir in "$NATRON_PACKAGE_PATH/data/bin"; do
        echo "*** stripping binaries in $dir"
        find "$dir" -type f \( -iname '*.exe' -o -iname '*.dll' -o -iname '*.pyd' -o -iname '*.ofx' \) -exec strip -s {} \;
        echo "*** stripping binaries in $dir... done!"
    done
fi

echo "*** $NATRON_PACKAGE_PATH contents:"
echo "========================================================================"
ls -R "$NATRON_PACKAGE_PATH"
echo "========================================================================"

# OCIO
OCIO_VERSION="$OCIO_PROFILES_VERSION"
OCIO_PACKAGE_PATH="${INSTALLER_PATH}/packages/${OCIO_PKG}"
mkdir -p "$OCIO_PACKAGE_PATH/meta" "$OCIO_PACKAGE_PATH/data/Resources"
$GSED "s/_VERSION_/${OCIO_VERSION}/;s/_DATE_/${DATE}/" < "$XML/ocio.xml" > "$OCIO_PACKAGE_PATH/meta/package.xml"
cp "$QS/ocio.qs" "$OCIO_PACKAGE_PATH/meta/installscript.qs"
cp -a "${TMP_BINARIES_PATH}/Resources/OpenColorIO-Configs" "$OCIO_PACKAGE_PATH/data/Resources/"

# CORE LIBS
CLIBS_VERSION="$CORELIBS_VERSION"
CLIBS_PATH="${INSTALLER_PATH}/packages/$CORELIBS_PKG"
mkdir -p "$CLIBS_PATH/meta" "$CLIBS_PATH/data/bin" "$CLIBS_PATH/data/lib" "$CLIBS_PATH/data/Resources/pixmaps" "$CLIBS_PATH/data/Resources/etc/fonts"
$GSED "s/_VERSION_/${CLIBS_VERSION}/;s/_DATE_/${DATE}/" < "$XML/corelibs.xml" > "$CLIBS_PATH/meta/package.xml"
cp "$QS/$PKGOS_ORIG/corelibs.qs" "$CLIBS_PATH/meta/installscript.qs"

cp -a "$SDK_HOME/etc/fonts"/* "$CLIBS_PATH/data/Resources/etc/fonts"
cp -a "$SDK_HOME/share/poppler" "$CLIBS_PATH/data/Resources/"
cp -a "$SDK_HOME/share/qt4/plugins"/* "$CLIBS_PATH/data/bin/"
rm -f "$CLIBS_PATH/data/bin"/*/*d4.dll || true

# PATH=/c/Program\ Files/INRIA/Natron-snapshots/bin:/c/Windows/System32:/c/Windows:/c/Windows/System32/Wbem:/c/Windows/System32/WindowsPowerShell/v1.0/ /usr/bin/cygcheck.exe /c/Program\ Files/INRIA/Natron-snapshots/bin/*.dll /c/Program\ Files/INRIA/Natron-snapshots/bin/*exe /c/Program\ Files/INRIA/Natron-snapshots/bin/*/*dll  |fgrep mingw64 |sed -e 's@ *C:\\msys64\\mingw64\\bin\\@@' | grep -v "^Q.*4.dll$" | sort |uniq|fgrep -v libgcc_s_
NATRON_DLL=(
    fbclient.dll
    libboost_filesystem-mt.dll
    libboost_regex-mt.dll
    libboost_serialization-mt.dll
    libboost_system-mt.dll
    libboost_thread-mt.dll
    libbz2-1.dll
    libcairo-2.dll
    LIBEAY32.dll
    libexpat-1.dll
    libfontconfig-1.dll
    libfreetype-6.dll
    libfribidi-0.dll
    libgif-7.dll
    libglib-2.0-0.dll
    libgomp-1.dll
    libgraphite2.dll
    libHalf-2_2.dll
    libharfbuzz-0.dll
    libiconv-2.dll
    libicudt61.dll
    libicuuc61.dll
    libidn2-0.dll
    libIex-2_2.dll
    libIlmImf-2_2.dll
    libIlmThread-2_2.dll
    libImath-2_2.dll
    libintl-8.dll
    libjasper-4.dll
    libjpeg-8.dll
    liblcms2-2.dll
    liblzma-5.dll
    libmng-2.dll
    libnghttp2-14.dll
    libogg-0.dll
    libOpenColorIO.dll
    libopenh264.dll
    libOpenImageIO.dll
    libpcre-1.dll
    libpixman-1-0.dll
    libpng16-16.dll
    libPtex.dll
    libpyside-python${PYVER}.dll
    libpython${PYVER}.dll
    libshiboken-python${PYVER}.dll
    libsnappy.dll
    libsqlite3-0.dll
    libssh2-1.dll
    libstdc++-6.dll
    libtheoradec-1.dll
    libtiff-5.dll
    libtinyxml-0.dll
    libunistring-2.dll
    libvorbis-0.dll
    libwebp-7.dll
    libwinpthread-1.dll
    libxml2-2.dll
    phonon4.dll
    SSLEAY32.dll
    yaml-cpp.dll
    zlib1.dll
)
if [ "$NATRON_LICENSE" = "GPL" ]; then
    cp "${SDK_HOME}/libraw-gpl2/bin/libraw_r-16.dll" "$CLIBS_PATH/data/bin/"
else
    cp "${SDK_HOME}/libraw-lgpl/bin/libraw_r-16.dll" "$CLIBS_PATH/data/bin/"
fi

# FFmpeg dependencies
# cygcheck.exe /mingw64/ffmpeg-lgpl/bin/*.dll |fgrep mingw64 |sed -e 's@ *C:\\msys64\\mingw64\\bin\\@@' |sort |uniq|fgrep -v libgcc_s_
NATRON_DLL=(
    "${NATRON_DLL[@]}"
    libass-9.dll
    libbluray-2.dll
    libbz2-1.dll
    libcaca-0.dll
    libcairo-2.dll
    libcelt0-2.dll
    libcroco-0.6-3.dll
    libexpat-1.dll
    libffi-6.dll
    libfontconfig-1.dll
    libfreetype-6.dll
    libfribidi-0.dll
    libgdk_pixbuf-2.0-0.dll
    libgio-2.0-0.dll
    libglib-2.0-0.dll
    libgmodule-2.0-0.dll
    libgmp-10.dll
    libgnutls-30.dll
    libgobject-2.0-0.dll
    libgraphite2.dll
    libgsm.dll
    libharfbuzz-0.dll
    libhogweed-4.dll
    libiconv-2.dll
    libidn2-0.dll
    libintl-8.dll
    liblzma-5.dll
    libmfx-1.dll
    libmodplug-1.dll
    libmp3lame-0.dll
    libnettle-6.dll
    libogg-0.dll
    libopenal-1.dll
    libopenh264.dll
    libopenjp2-7.dll
    libopus-0.dll
    libp11-kit-0.dll
    libpango-1.0-0.dll
    libpangocairo-1.0-0.dll
    libpangoft2-1.0-0.dll
    libpangowin32-1.0-0.dll
    libpcre-1.dll
    libpixman-1-0.dll
    libpng16-16.dll
    librsvg-2-2.dll
    librtmp-1.dll
    libsnappy.dll
    libspeex-1.dll
    libsrt.dll
    libstdc++-6.dll
    libtasn1-6.dll
    libtheoradec-1.dll
    libtheoraenc-1.dll
    libunistring-2.dll
    libvorbis-0.dll
    libvorbisenc-2.dll
    libvpx-1.dll
    libwavpack-1.dll
    libwebp-7.dll
    libwebpmux-3.dll
    libwinpthread-1.dll
    libxml2-2.dll
    SDL2.dll
    zlib1.dll
)
if [ "$NATRON_LICENSE" = "GPL" ]; then
    NATRON_DLL=(
        "${NATRON_DLL[@]}"
        #libopencore-amrnb-0.dll # Apache 2 / GPL3
        #libopencore-amrwb-0.dll # Apache 2 / GPL3
        libx264-152.dll
        libx265.dll
        #libx265_main10.dll
        libhdr10plus.dll
        xvidcore.dll
    )
fi
if [ "${BITS}" = "32" ]; then
    NATRON_DLL=("${NATRON_DLL[@]}" "libgcc_s_dw2-1.dll")
else
    NATRON_DLL=("${NATRON_DLL[@]}" "libgcc_s_seh-1.dll")
fi

# sort ids:
NATRON_DLL=($(echo "${NATRON_DLL[@]}" | tr ' ' '\n' | sort -u | tr '\n' ' '))
for depend in "${NATRON_DLL[@]}"; do
    cp "$SDK_HOME/bin/$depend" "$CLIBS_PATH/data/bin/"
done
# SeExpr 1.0.1 installs in lib, not bin
if [ -f "$SDK_HOME/lib/libSeExpr.dll" ]; then
    cp "$SDK_HOME/lib/libSeExpr.dll" "$CLIBS_PATH/data/bin/"
elif [ -f "$SDK_HOME/bin/libSeExpr.dll" ]; then
    cp "$SDK_HOME/bin/libSeExpr.dll" "$CLIBS_PATH/data/bin/"
fi

# custom fontconfig
#cp $SDK_HOME/fontconfig/bin/LIBFONTCONFIG-1.DLL $CLIBS_PATH/data/bin/

#if [ -d "$SDK_HOME/Plugins/Shadertoy.ofx.bundle" ]; then
cp "$SDK_HOME/osmesa/lib/osmesa.dll" "$CLIBS_PATH/data/bin/"
#fi

#Copy Qt dlls (required for all PySide modules to work correctly)
cp "$SDK_HOME/bin"/Qt*4.dll "$CLIBS_PATH/data/bin/"

#Ignore debug dlls of Qt
rm "$CLIBS_PATH/data/bin"/*d4.dll || true
rm "$CLIBS_PATH/data/bin/sqldrivers"/{*mysql*,*psql*} || true

#Copy ffmpeg binaries
# ffmpeg 3
FFMPEG_DLLS_3=(
    avcodec-57.dll
    avdevice-57.dll
    avfilter-6.dll
    avformat-57.dll
    avresample-3.dll
    avutil-55.dll
    swresample-2.dll
    swscale-4.dll
)
# ffmpeg 4
FFMPEG_DLLS=(
    avcodec-58.dll
    avdevice-58.dll
    avfilter-7.dll
    avformat-58.dll
    avutil-56.dll
    swresample-3.dll
    swscale-5.dll
)

if [ "$NATRON_LICENSE" = "GPL" ]; then
    # ffmpeg 3
    FFMPEG_DLLS_3=(
        "${FFMPEG_DLLS_3[@]}"
        postproc-54.dll
    )
    # ffmpeg 4
    FFMPEG_DLLS=(
        "${FFMPEG_DLLS[@]}"
        postproc-55.dll
    )
fi

for depend in "${FFMPEG_DLLS[@]}"; do 
    cp "$FFMPEG_BIN_PATH/bin/$depend" "$CLIBS_PATH/data/bin"
done
#Also embbed ffmpeg.exe and ffprobe.exe
#cp "$FFMPEG_BIN_PATH/bin/ffmpeg.exe" "$FFMPEG_BIN_PATH/bin/ffprobe.exe" "$CLIBS_PATH/data/bin"

if [ "${DEBUG_MODE:-}" != "1" ]; then
    for dir in "$CLIBS_PATH/data/bin"; do
        echo "*** stripping binaries in $dir"
        find "$dir" -type f \( -iname '*.exe' -o -iname '*.dll' -o -iname '*.pyd' -o -iname '*.ofx' \) -exec strip -s {} \;
        echo "*** stripping binaries in $dir... done!"
    done
fi

CORE_DOC="$CLIBS_PATH"
#echo "" >> "$CORE_DOC/meta/3rdparty-license.txt"
#cat "$CWD/include/natron/3rdparty.txt" >> "$CORE_DOC/meta/3rdparty-license.txt"


#Copy Python distrib
mkdir -p "$CLIBS_PATH/data/Plugins"
cp -a "$SDK_HOME/lib/python${PYVER}" "$CLIBS_PATH/data/lib/"
mv "$CLIBS_PATH/data/lib/python${PYVER}/site-packages/PySide" "$CLIBS_PATH/data/Plugins/"
rm -rf "$CLIBS_PATH/data/lib/python${PYVER}"/{test,config,config-${PYVER}m}
#rm -rf "$CLIBS_PATH/data/lib/python${PYVER}/site-packages"/{sphinx*,pygments,markupsafe,Mako*,mako,jinja*,docutils*,beaker,Beaker*,babel,alabaster,Babel*,colorama*,easy_install*,Markup*,pkg_res*,Pygm*,pytz,setuptools,snow*}

( cd "$CLIBS_PATH/data/lib/python${PYVER}/site-packages";
  for dir in *; do
      if [ -d "$dir" ]; then
          rm -rf "$dir"
      fi
  done
)

# Strip Python
PYDIR="$CLIBS_PATH/data/lib/python${PYVER:-}"
find "${PYDIR}" -type f -name '*.pyo' -exec rm {} \;
(cd "${PYDIR}"; xargs rm -rf || true) < "$INC_PATH/python-exclude.txt"


(cd "$CLIBS_PATH" ; find . -type d -name __pycache__ -exec rm -rf {} \;)

if [ "${DEBUG_MODE:-}" != "1" ]; then
    for dir in "$CLIBS_PATH/data/Plugins/PySide" "$CLIBS_PATH/data/lib/python${PYVER:-}"; do
        echo "*** stripping binaries in $dir"
        find "$dir" -type f \( -iname '*.exe' -o -iname '*.dll' -o -iname '*.pyd' -o -iname '*.ofx' \) -exec strip -s {} \;
        echo "*** stripping binaries in $dir... done!"
    done
fi

# python zip
cd "$PYDIR"
if [ "${USE_QT5:-}" != 1 ]; then
  rm -rf site-packages/shiboken2* site-packages/PySide2 || true
fi
# remove pyc files
find . -type f -name '*.pyc' -exec rm {} \;
# generate pyo files
"$SDK_HOME"/bin/python.exe -O -m compileall . || true
PY_ZIP_FILES=(bsddb compiler ctypes curses distutils email encodings hotshot idlelib importlib json logging multiprocessing pydoc_data sqlite3 unittest wsgiref xml)
zip -q -r "../python${PYVERNODOT}.zip" ./*.py* "${PY_ZIP_FILES[@]}"
rm -rf ./*.py* "${PY_ZIP_FILES[@]}" || true

echo "*** $CLIBS_PATH contents:"
echo "========================================================================"
ls -R "$CLIBS_PATH"
echo "========================================================================"

# OFX ARENA
if [ "${BUNDLE_ARENA:-}" = "1" ]; then
    poppler_version=75
    ARENA_DLL=(
        libcdr-0.1.dll
        libicuin61.dll
        libpoppler-${poppler_version}.dll
        libpoppler-glib-8.dll
        librevenge-0.0.dll
        librevenge-stream-0.0.dll
        libzip.dll
    )
    # note: check that poppler doesn't depend on nss3
    if ldd "$SDK_HOME/bin/libpoppler-${poppler_version}.dll" | fgrep nss3.dll; then
        echo "Error: poppler was built with NSS3 support:"
        ldd "$SDK_HOME/bin/libpoppler-${poppler_version}.dll"
    fi
    OFX_ARENA_VERSION="$TAG"
    OFX_ARENA_PATH="${INSTALLER_PATH}/packages/$ARENAPLUG_PKG"
    mkdir -p "$OFX_ARENA_PATH/meta" "$OFX_ARENA_PATH/data/Plugins/OFX/Natron"
    $GSED "s/_VERSION_/${OFX_ARENA_VERSION}/;s/_DATE_/${DATE}/" < "$XML/openfx-arena.xml" > "$OFX_ARENA_PATH/meta/package.xml"
    cp "$QS/openfx-arena.qs" "$OFX_ARENA_PATH/meta/installscript.qs"
    cp -av "${TMP_BINARIES_PATH}/OFX/Plugins/Arena.ofx.bundle" "$OFX_ARENA_PATH/data/Plugins/OFX/Natron/"
    if [ -d "${TMP_BINARIES_PATH}"/OFX/Plugins/ArenaCL.ofx.bundle ]; then
        cp -av "${TMP_BINARIES_PATH}/Plugins/ArenaCL.ofx.bundle" "$OFX_ARENA_PATH/data/Plugins/OFX/Natron/"
    fi

    for depend in "${ARENA_DLL[@]}"; do
        cp "$SDK_HOME/bin/$depend"  "$OFX_ARENA_PATH/data/Plugins/OFX/Natron/Arena.ofx.bundle/Contents/Win${BITS}/"
    done
    
    #OpenColorIO is located in $SDK_HOME/lib and not bin
#    cp "$SDK_HOME/lib/LIBOPENCOLORIO.DLL" "$OFX_ARENA_PATH/data/Plugins/OFX/Natron/Arena.ofx.bundle/Contents/Win${BITS}/"

    if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
        dump_syms_safe "$OFX_ARENA_PATH/data/Plugins/OFX/Natron"/*/*/*/Arena.ofx "${TMP_BINARIES_PATH}/symbols/Arena.ofx-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
        if [ -d "${TMP_BINARIES_PATH}"/OFX/Plugins/ArenaCL.ofx.bundle ]; then
            dump_syms_safe "$OFX_ARENA_PATH/data/Plugins/OFX/Natron/*/*/*/ArenaCL.ofx" "${TMP_BINARIES_PATH}/symbols/ArenaCL.ofx-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
        fi
    fi
    if [ "${DEBUG_MODE:-}" != "1" ]; then
        for dir in "$OFX_ARENA_PATH/data/Plugins/OFX/Natron"; do
            echo "*** stripping binaries in $dir"
            find "$dir" -type f \( -iname '*.exe' -o -iname '*.dll' -o -iname '*.pyd' -o -iname '*.ofx' \) -exec strip -s {} \;
            echo "*** stripping binaries in $dir... done!"
        done
    fi
    echo "*** $OFX_ARENA_PATH contents:"
    echo "========================================================================"
    ls -R "$OFX_ARENA_PATH"
    echo "========================================================================"

fi

# OFX GMIC
if [ "${BUNDLE_GMIC:-}" = "1" ]; then
    GMIC_DLL=(
        libfftw3-3.dll
        libcurl-4.dll
        libbrotlidec.dll
        libbrotlicommon.dll
        libnghttp2-14.dll
    )
    OFX_GMIC_VERSION="$TAG"
    OFX_GMIC_PATH="${INSTALLER_PATH}/packages/$GMICPLUG_PKG"
    mkdir -p "$OFX_GMIC_PATH/meta" "$OFX_GMIC_PATH/data/Plugins/OFX/Natron"
    $GSED "s/_VERSION_/${OFX_GMIC_VERSION}/;s/_DATE_/${DATE}/" < "$XML/openfx-gmic.xml" > "$OFX_GMIC_PATH/meta/package.xml"
    cp "$QS/openfx-gmic.qs" "$OFX_GMIC_PATH/meta/installscript.qs"
    cp -av "${TMP_BINARIES_PATH}/OFX/Plugins/GMIC.ofx.bundle" "$OFX_GMIC_PATH/data/Plugins/OFX/Natron/"
    for depend in "${GMIC_DLL[@]}"; do
        cp "$SDK_HOME/bin/$depend"  "$OFX_GMIC_PATH/data/Plugins/OFX/Natron/GMIC.ofx.bundle/Contents/Win${BITS}/"
    done
    
    if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
        dump_syms_safe "$OFX_GMIC_PATH/data/Plugins/OFX/Natron"/*/*/*/GMIC.ofx "${TMP_BINARIES_PATH}/symbols/GMIC.ofx-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
    fi
    if [ "${DEBUG_MODE:-}" != "1" ]; then
        for dir in "$OFX_GMIC_PATH/data/Plugins/OFX/Natron"; do
            echo "*** stripping binaries in $dir"
            find "$dir" -type f \( -iname '*.exe' -o -iname '*.dll' -o -iname '*.pyd' -o -iname '*.ofx' \) -exec strip -s {} \;
            echo "*** stripping binaries in $dir... done!"
        done
    fi
    echo "*** $OFX_GMIC_PATH contents:"
    echo "========================================================================"
    ls -R "$OFX_GMIC_PATH"
    echo "========================================================================"

fi

# MESA
if [ ! -d "$SRC_PATH/natron-windows-mesa" ]; then
    ( cd "$SRC_PATH";
      $WGET "$THIRD_PARTY_BIN_URL/natron-windows-mesa.tar" -O "$SRC_PATH/natron-windows-mesa.tar"
      tar xvf natron-windows-mesa.tar
    )
fi
MESA_PKG_PATH="${INSTALLER_PATH}/packages/fr.inria.mesa"
mkdir -p "$MESA_PKG_PATH/meta" "$MESA_PKG_PATH/data/bin"
$GSED "s/_DATE_/${DATE}/" < "$XML/mesa.xml" > "$MESA_PKG_PATH/meta/package.xml"
cp "$QS/mesa.qs" "$MESA_PKG_PATH/meta/installscript.qs"
cp "$SRC_PATH/natron-windows-mesa/win${BITS}/opengl32.dll" "$MESA_PKG_PATH/data/bin/"


echo "*** $MESA_PKG_PATH contents:"
echo "========================================================================"
ls -R "$MESA_PKG_PATH"
echo "========================================================================"

# OFX CV
if [ "${BUNDLE_CV:-}" = "1" ]; then 
    CV_DLL=(
        libopencv_core2411.dll
        libopencv_imgproc2411.dll
        libopencv_photo2411.dll
    )
    SEGMENT_DLL=(
        libopencv_flann2411.dll
        libjasper-4.dll
        libopencv_calib3d2411.dll
        libopencv_features2d2411.dll
        libopencv_highgui2411.dll
        libopencv_ml2411.dll
        libopencv_video2411.dll
        libopencv_legacy2411.dll
    )
    OFX_CV_VERSION="$TAG"
    OFX_CV_PATH="${INSTALLER_PATH}/packages/$CVPLUG_PKG"
    mkdir -p "$OFX_CV_PATH"/{data,meta} "$OFX_CV_PATH/data/Plugins/OFX/Natron" "$OFX_CV_PATH/data/docs/openfx-opencv"
    $GSED "s/_VERSION_/${OFX_CV_VERSION}/;s/_DATE_/${DATE}/" < "$XML/openfx-opencv.xml" > "$OFX_CV_PATH/meta/package.xml"
    cp "$QS/openfx-opencv.qs" "$OFX_CV_PATH/meta/installscript.qs"
    cp -a "${TMP_BINARIES_PATH}/docs/openfx-opencv" "$OFX_CV_PATH/data/docs/"
    #cat "$OFX_CV_PATH/data/docs/openfx-opencv/README" > "$OFX_CV_PATH/meta/ofx-cv-license.txt"
    cp -a "${TMP_BINARIES_PATH}/Plugins"/{inpaint,segment}.ofx.bundle "$OFX_CV_PATH/data/Plugins/OFX/Natron/"
    for depend in "${CV_DLL[@]}"; do
        cp -v "$SDK_HOME/bin/$depend" "$OFX_CV_PATH/data/Plugins/OFX/Natron/inpaint.ofx.bundle/Contents/Win${BITS}/"
        cp -v "$SDK_HOME/bin/$depend" "$OFX_CV_PATH/data/Plugins/OFX/Natron/segment.ofx.bundle/Contents/Win${BITS}/"
    done
    for depend in "${SEGMENT_DLL[@]}"; do
        cp -v "$SDK_HOME/bin/$depend" "$OFX_CV_PATH/data/Plugins/OFX/Natron/segment.ofx.bundle/Contents/Win${BITS}/"
    done
    if [ "${DEBUG_MODE:-}" != "1" ]; then
        for dir in "$OFX_CV_PATH/data/Plugins/OFX/Natron"; do
            echo "*** stripping binaries in $dir"
            find "$dir" -type f \( -iname '*.exe' -o -iname '*.dll' -o -iname '*.pyd' -o -iname '*.ofx' \) -exec strip -s {} \;
            echo "*** stripping binaries in $dir... done!"
        done
    fi
    #$BUNDLE_CV
fi

# manifests

#if [ "${BUNDLE_MISC:-}" = "1" ]; then 
#    CIMG_MANIFEST=$OFX_MISC_PATH/data/Plugins/OFX/Natron/CImg.ofx.bundle/Contents/Win${BITS}/manifest
#    echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" > $CIMG_MANIFEST
#    echo "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">" >> $CIMG_MANIFEST
#    echo "<assemblyIdentity name=\"CImg.ofx\" version=\"1.0.0.0\" type=\"win32\" processorArchitecture=\"amd64\"/>" >> "$CIMG_MANIFEST"
#
#    for depend in "${CIMG_DLL[@]}"; do
#        echo "<file name=\"${depend}\"></file>" >> "$CIMG_MANIFEST"
#    done
#    echo "</assembly>" >> $CIMG_MANIFEST
#    cd "$OFX_MISC_PATH/data/Plugins/OFX/Natron/CImg.ofx.bundle/Contents/Win${BITS}"
#    mt -manifest manifest -outputresource:"CImg.ofx;2"
#fi

#if [ "${BUNDLE_IO:-}" = "1" ]; then 
#    IO_MANIFEST=$OFX_IO_PATH/data/Plugins/OFX/Natron/IO.ofx.bundle/Contents/Win${BITS}/manifest
#    echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" > $IO_MANIFEST
#    echo "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">" >> $IO_MANIFEST
#    echo "<assemblyIdentity name=\"IO.ofx\" version=\"1.0.0.0\" type=\"win32\" processorArchitecture=\"amd64\"/>" >> $IO_MANIFEST
#    for depend in "${IO_DLL[@]}"; do
#        echo "<file name=\"${depend}\"></file>" >> $IO_MANIFEST
#    done
#    #OpenColorIO and SeExpr are located in /lib and not /bin that's why they are not in $IO_DLL
#    echo "<file name=\"LIBOPENCOLORIO.DLL\"></file>" >> $IO_MANIFEST
#    echo "<file name=\"LIBSEEXPR.DLL\"></file>" >> $IO_MANIFEST
#    echo "</assembly>" >> $IO_MANIFEST
#    cd "$OFX_IO_PATH/data/Plugins/OFX/Natron/IO.ofx.bundle/Contents/Win${BITS}"
#    mt -manifest manifest -outputresource:"IO.ofx;2"
#fi

if [ "${BUNDLE_ARENA:-}" = "1" ]; then 
    ARENA_MANIFEST="$OFX_ARENA_PATH/data/Plugins/OFX/Natron/Arena.ofx.bundle/Contents/Win${BITS}/manifest"
    
    echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" > "$ARENA_MANIFEST"
    echo "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">" >> "$ARENA_MANIFEST"
    echo "<assemblyIdentity name=\"Arena.ofx\" version=\"1.0.0.0\" type=\"win32\" processorArchitecture=\"amd64\"/>" >> "$ARENA_MANIFEST"
    for depend in "${ARENA_DLL[@]}"; do
        echo "<file name=\"${depend}\"></file>" >> "$ARENA_MANIFEST"
    done
    #OpenColorIO is located in /lib and not /bin that's why they are not in $IO_DLL
#    echo "<file name=\"LIBOPENCOLORIO.DLL\"></file>" >> "$ARENA_MANIFEST"
    echo "</assembly>" >> "$ARENA_MANIFEST"
    cd "$OFX_ARENA_PATH/data/Plugins/OFX/Natron/Arena.ofx.bundle/Contents/Win${BITS}"
    mt -manifest manifest -outputresource:"Arena.ofx;2"

fi

if [ "${BUNDLE_GMIC:-}" = "1" ]; then 
    GMIC_MANIFEST="$OFX_GMIC_PATH/data/Plugins/OFX/Natron/GMIC.ofx.bundle/Contents/Win${BITS}/manifest"
    
    echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" > "$GMIC_MANIFEST"
    echo "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">" >> "$GMIC_MANIFEST"
    echo "<assemblyIdentity name=\"GMIC.ofx\" version=\"1.0.0.0\" type=\"win32\" processorArchitecture=\"amd64\"/>" >> "$GMIC_MANIFEST"
    for depend in "${GMIC_DLL[@]}"; do
        echo "<file name=\"${depend}\"></file>" >> "$GMIC_MANIFEST"
    done
    echo "</assembly>" >> "$GMIC_MANIFEST"
    cd "$OFX_GMIC_PATH/data/Plugins/OFX/Natron/GMIC.ofx.bundle/Contents/Win${BITS}"
    mt -manifest manifest -outputresource:"GMIC.ofx;2"

fi

if [ "${BUNDLE_CV:-}" = "1" ]; then 
    INPAINT_MANIFEST="$OFX_CV_PATH/data/Plugins/OFX/Natron/inpaint.ofx.bundle/Contents/Win${BITS}/manifest"
    echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" > "$INPAINT_MANIFEST"
    echo "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">" >> "$INPAINT_MANIFEST"
    echo "<assemblyIdentity name=\"inpaint.ofx\" version=\"1.0.0.0\" type=\"win32\" processorArchitecture=\"amd64\"/>" >> "$INPAINT_MANIFEST"
    
    for depend in "${CV_DLL[@]}"; do
        echo "<file name=\"${depend}\"></file>" >> "$INPAINT_MANIFEST"
    done
    echo "</assembly>" >> "$INPAINT_MANIFEST"
    cd "$OFX_CV_PATH/data/Plugins/OFX/Natron/inpaint.ofx.bundle/Contents/Win${BITS}"
    mt -manifest manifest -outputresource:"inpaint.ofx;2"

    SEGMENT_MANIFEST="$OFX_CV_PATH/data/Plugins/OFX/Natron/segment.ofx.bundle/Contents/Win${BITS}/manifest"
    echo "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" > "$SEGMENT_MANIFEST"
    echo "<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">" >> "$SEGMENT_MANIFEST"
    echo "<assemblyIdentity name=\"segment.ofx\" version=\"1.0.0.0\" type=\"win32\" processorArchitecture=\"amd64\"/>" >> "$SEGMENT_MANIFEST"
    
    for depend in "${CV_DLL[@]}"; do
        echo "<file name=\"${depend}\"></file>" >> "$SEGMENT_MANIFEST"
    done
    for depend in "${SEGMENT_DLL[@]}"; do
        echo "<file name=\"${depend}\"></file>" >> "$SEGMENT_MANIFEST"
    done
    echo "</assembly>" >> "$SEGMENT_MANIFEST"
    cd "$OFX_CV_PATH/data/Plugins/OFX/Natron/segment.ofx.bundle/Contents/Win${BITS}"
    mt -manifest manifest -outputresource:"segment.ofx;2"
fi

# Clean and perms
(cd "${INSTALLER_PATH}" && find . -type d -name .git -exec rm -rf {} \;)

if [ "$BUILD_CONFIG" = "SNAPSHOT" ]; then
    SNAPDATE="${NATRON_BRANCH}-${TAG}-"
fi

# Build repo and package
ZIP_ARCHIVE_BASE=Natron-${SNAPDATE:-}$NATRON_VERSION-${PKGOS}-no-installer

if [ "${DEBUG_MODE:-}" = "1" ]; then
    ZIP_ARCHIVE_BASE="${ZIP_ARCHIVE_BASE}-debug"
fi

if [ "${NO_INSTALLER:-}" != "1" ]; then
    ONLINE_INSTALL="Natron-${PKGOS}-online-$ONLINE_TAG-setup.exe"
    BUNDLED_INSTALL="Natron-${SNAPDATE:-}$NATRON_VERSION-${PKGOS}-setup.exe"
    if [ "${DEBUG_MODE:-}" = "1" ]; then
        BUNDLED_INSTALL="${BUNDLED_INSTALL}-debug"
    fi

    "$SDK_HOME/bin/repogen" -v --update-new-components -p "${INSTALLER_PATH}/packages" -c "${INSTALLER_PATH}/config/config.xml" "$REPO_DIR/packages"

    if [ "${OFFLINE:-}" == "1" ]; then
        "$SDK_HOME/bin/binarycreator" -v -f -p "${INSTALLER_PATH}/packages" -c "${INSTALLER_PATH}/config/config.xml" -i "${PACKAGES},fr.inria.mesa" "$REPO_DIR/installers/$BUNDLED_INSTALL"
    fi
    cd "$REPO_DIR/installers"
    "$SDK_HOME/bin/binarycreator" -v -n -p "${INSTALLER_PATH}/packages" -c "${INSTALLER_PATH}/config/config.xml" "$ONLINE_INSTALL"
fi

if [ "${NO_ZIP:-}" != "1" ]; then
    mkdir -p "$REPO_DIR/installers" || true
    mkdir -p "$ARCHIVE_DATA_DIR" || true
    cp -a "${INSTALLER_PATH}/packages"/fr.*/data/* "$ARCHIVE_DATA_DIR/"
    mkdir -p "$ARCHIVE_DATA_DIR/bin/mesa"
    mv "$ARCHIVE_DATA_DIR/bin/opengl32.dll" "$ARCHIVE_DATA_DIR/bin/mesa/"
    mv "$ARCHIVE_DATA_DIR" "$ARCHIVE_DIR/${ZIP_ARCHIVE_BASE}"
    (cd "$ARCHIVE_DIR" && zip -q -r "${ZIP_ARCHIVE_BASE}.zip" "${ZIP_ARCHIVE_BASE}" && rm -rf "${ZIP_ARCHIVE_BASE}";)
    mv "$ARCHIVE_DIR/${ZIP_ARCHIVE_BASE}.zip" "$REPO_DIR/installers/"
fi

BUILD_ARCHIVE="$CWD/archive"
if [ ! -d "$BUILD_ARCHIVE" ]; then
    mkdir -p "$BUILD_ARCHIVE"
fi
# waste of space #cp $REPO_DIR/installers/* "$BUILD_ARCHIVE/" || true

# UnitTests
#if [ "${UNIT_TEST:-}" = "1" ]; then
#
#UNIT_TMP=${INSTALLER_PATH}/UnitTests
#UNIT_LOG=$REPO_DIR/logs/unit_tests.Windows${BITS}.$TAG.log
#
#if [ ! -d "$UNIT_TMP" ]; then
#  mkdir -p "$UNIT_TMP"
#fi
#cp -a ${INSTALLER_PATH}/packages/*/data/* $UNIT_TMP/
#cd "$CWD"
#if [ -d "$CWD/Natron-Tests" ]; then 
#  cd Natron-Tests
#  $GIT pull
#else
#  $GIT clone $GIT_UNIT
#  cd Natron-Tests
#fi
#echo "Running unit tests ..."
#export FONTCONFIG_PATH=$UNIT_TMP/Resources/etc/fonts/fonts.conf
#mkdir -p ~/.cache/INRIA/Natron/{ViewerCache,DiskCache} || true
#FFMPEG="/mingw64/ffmpeg-GPL/bin/ffmpeg.exe" COMPARE=`pwd`/compare.exe sh runTests.sh $UNIT_TMP/bin/NatronRenderer-bin.exe >& "$UNIT_LOG"
#
#fi

echo "All Done!!!"

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
