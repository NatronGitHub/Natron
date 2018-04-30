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

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

#
# Build packages and installer for Linux
# OFFLINE=1: Make offline installer
# TAR_BUILD=1: Tar the build
# RPM_BUILD=1: Make RPM
# DEB_BUILD=1: Make DEB
# DISABLE_BREAKPAD=1: Disable automatic crash report
# NO_INSTALLER=1: Do not build installer
# BUILD_CONFIG=(SNAPSHOT,ALPHA,BETA,RC,STABLE,CUSTOM)
# CUSTOM_BUILD_USER_NAME="Toto" : to be set if BUILD_CONFIG=CUSTOM
# BUILD_NUMBER=X: To be set to indicate the revision number of the build. For example RC1,RC2, RC3 etc...
# NATRON_LICENSE=(GPL,COMMERCIAL)
# Usage: 
# OFFLINE=1 BUILD_CONFIG=SNAPSHOT sh build-installer.sh
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

PID=$$
if [ -f "$TMPDIR/natron-build-installer.pid" ]; then
    OLDPID=$(cat "$TMPDIR/natron-build-installer.pid")
    PIDS=$(ps aux|awk '{print $2}')
    for i in $PIDS;do
        if [ "$i" = "$OLDPID" ]; then
            echo "already running ..."
            exit 1
        fi
    done
fi
echo $PID > "$TMPDIR/natron-build-installer.pid"

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
    ONLINE_TAG="snapshot"
    REPO_SUFFIX="snapshot-${NATRON_BRANCH}"
else
    REPO_BRANCH="releases/${NATRON_BRANCH}"
    REPO_BRANCH_LATEST="releases/latest"
    ONLINE_TAG="release"
    REPO_SUFFIX="release"
fi

REPO_DIR=""
if [ "${QID:-}" != "" ]; then
    REPO_DIR="${QUEUE_TMP_PATH}${QID}_${BITS}"
else
    REPO_DIR="$REPO_DIR_PREFIX$REPO_SUFFIX$BIT"
fi

DATE=$(date +%Y-%m-%d)
PKGOS_ORIG=$PKGOS
PKGOS=Linux-x86_${BITS}bit
#REPO_OS=Linux/$REPO_BRANCH/${BITS}bit/packages
REPO_OS=packages/Linux/${BITS}/${REPO_BRANCH}
REPO_OS_LATEST=packages/Linux/${BITS}/${REPO_BRANCH_LATEST}

if [ -x "$SDK_HOME/bin/qmake" ]; then
    QTDIR="$SDK_HOME"
elif [ -x "$SDK_HOME/qt4/bin/qmake" ]; then
    QTDIR="$SDK_HOME/qt4"
elif [ -x "$SDK_HOME/qt5/bin/qmake" ]; then
    QTDIR="$SDK_HOME/qt5"
else
    echo "*** Error: cannot find Qt"
    exit
fi

LD_LIBRARY_PATH="$SDK_HOME/lib:$FFMPEG_PATH/lib:$LIBRAW_PATH/lib:$QTDIR/lib"
PATH="$SDK_HOME/gcc/bin:$SDK_HOME/bin:$PATH"

if [ "$ARCH" = "x86_64" ]; then
    LD_LIBRARY_PATH="$SDK_HOME/gcc/lib64:$LD_LIBRARY_PATH"
else
    LD_LIBRARY_PATH="$SDK_HOME/gcc/lib:$LD_LIBRARY_PATH"
fi
export LD_LIBRARY_PATH

if [ ! -d "$TMP_PATH" ]; then
    mkdir -p "$TMP_PATH"
fi

if [ -d "$TMP_BINARIES_PATH/symbols" ]; then
    rm -rf "$TMP_BINARIES_PATH/symbols"
fi
mkdir -p "$TMP_BINARIES_PATH/symbols"

# tag symbols we want to keep with 'release'
if [ "$BUILD_CONFIG" != "" ] && [ "$BUILD_CONFIG" != "SNAPSHOT" ]; then
    BPAD_TAG="-release"
    if [ -f "$TMP_BINARIES_PATH/natron-fullversion.txt" ]; then
        GET_VER=$(cat "$TMP_BINARIES_PATH/natron-fullversion.txt")
        if [ "$GET_VER" != "" ]; then
            TAG=$GET_VER
        fi
    fi
fi

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

# SETUP
INSTALLER="$TMP_PATH/Natron-installer"
XML="$INC_PATH/xml"
QS="$INC_PATH/qs"

if [ -d "$INSTALLER" ]; then
    rm -rf "$INSTALLER"
fi

mkdir -p "$INSTALLER/config" "$INSTALLER/packages"
$GSED "s/_VERSION_/${NATRON_VERSION_NUMBER}/;s#_RBVERSION_#${NATRON_BRANCH}#g;s#_OS_BRANCH_BIT_#${REPO_OS}#g;s#_OS_BRANCH_LATEST_#${REPO_OS_LATEST}#g;s#_URL_#${REPO_URL}#g" "$INC_PATH/config/$PKGOS_ORIG.xml" > "$INSTALLER/config/config.xml"
cp "$INC_PATH/config"/*.png "$INSTALLER/config/"

# OFX IO
if [ "$BUNDLE_IO" = "1" ]; then
    OFX_IO_VERSION="$TAG"
    OFX_IO_PATH="$INSTALLER/packages/$IOPLUG_PKG"
    mkdir -p "$OFX_IO_PATH/data" "$OFX_IO_PATH/meta" "$OFX_IO_PATH/data/Plugins/OFX/Natron" "$OFX_IO_PATH/data/bin"
    $GSED "s/_VERSION_/${OFX_IO_VERSION}/;s/_DATE_/${DATE}/" "$XML/openfx-io.xml" > "$OFX_IO_PATH/meta/package.xml"
    cat "$QS/openfx-io.qs" > "$OFX_IO_PATH/meta/installscript.qs"
    cp "$FFMPEG_PATH/bin/"{ffmpeg,ffprobe} "$OFX_IO_PATH/data/"
    ( cd "$OFX_IO_PATH/data/bin";
      ln -sf ../ffmpeg .
      ln -sf ../ffprobe .
    )
    OFX_IO_EXTRA="iconvert idiff igrep iinfo exrheader tiffinfo" 
    for iobin in $OFX_IO_EXTRA; do
        cp "$SDK_HOME/bin/$iobin" "$OFX_IO_PATH/data/"
    done

    if [ "${DEBUG_MODE:-}" != "1" ]; then 
        strip -s "$OFX_IO_PATH/data/ffmpeg"
        strip -s "$OFX_IO_PATH/data/ffprobe"
    fi

    cp -a "$TMP_BINARIES_PATH/OFX/Plugins/IO.ofx.bundle" "$OFX_IO_PATH/data/Plugins/OFX/Natron/"

    if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
        "$SDK_HOME/bin/dump_syms" "$OFX_IO_PATH"/data/Plugins/OFX/Natron/*/*/*/IO.ofx > "$TMP_BINARIES_PATH/symbols/IO.ofx-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
        #$SDK_HOME/bin/dump_syms "$SDK_HOME/lib/libOpenImageIO.so.1."* > "$TMP_BINARIES_PATH/symbols/libOpenImageIO.so.1."*"-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
    fi

    if [ "${DEBUG_MODE:-}" != "1" ]; then
        find "$OFX_IO_PATH/data/Plugins/OFX/Natron" -type f -exec strip -s {} \; &>/dev/null
    fi

    IO_LIBS="$OFX_IO_PATH/data/Plugins/OFX/Natron/IO.ofx.bundle/Libraries"
    mkdir -p "$IO_LIBS"

    OFX_EXTRA_DEPENDS=$(ldd $(find "$OFX_IO_PATH/data" -maxdepth 1 -type f) |grep /opt | awk '{print $3}'|sort|uniq)
    for x in $OFX_EXTRA_DEPENDS; do
        cp -fv "$x" "$IO_LIBS/"
    done

    OFX_DEPENDS=$(env LD_LIBRARY_PATH="$IO_LIBS:$LD_LIBRARY_PATH" ldd "$INSTALLER"/packages/*/data/Plugins/OFX/Natron/*/*/*/*|grep /opt | awk '{print $3}'|sort|uniq)
    for x in $OFX_DEPENDS; do
        cp -fv "$x" "$IO_LIBS/"
    done

    cp -v "$LIBRAW_PATH"/lib/libraw_r.so.16 "$IO_LIBS/"
    cp -v "$FFMPEG_PATH"/lib/{libavcodec.so.57,libavdevice.so.57,libavfilter.so.6,libavformat.so.57,libavresample.so.3,libavutil.so.55,libpostproc.so.54,libswresample.so.2,libswscale.so.4} "$IO_LIBS/"
    OFX_LIB_DEP=$(env LD_LIBRARY_PATH="$IO_LIBS:$LD_LIBRARY_PATH" ldd "$IO_LIBS"/*|grep /opt | awk '{print $3}'|sort|uniq)
    for y in $OFX_LIB_DEP; do
        cp -fv "$y" "$IO_LIBS/"
    done

    rm -f "$IO_LIBS"/{liblcms*,libgcc*,libstdc*,libbz2*,libfont*,libfree*,libpng*,libjpeg*,libtiff*,libz.*}
    (cd "$IO_LIBS" ;
     ln -sf ../../../../../lib/libbz2.so.1 .
     ln -sf ../../../../../lib/libfontconfig.so.1 .
     ln -sf ../../../../../lib/libfreetype.so.6 .
     ln -sf ../../../../../lib/libpng16.so.16 .
     ln -sf ../../../../../lib/libjpeg.so.8 .
     ln -sf ../../../../../lib/libtiff.so.5 .
     ln -sf ../../../../../lib/libz.so.1 .
     ln -sf ../../../../../lib/libgcc_s.so.1 .
     ln -sf ../../../../../lib/libstdc++.so.6 .
     ln -sf ../../../../../lib/liblcms2.so.2 .
     for i in *.so*; do
         patchelf --set-rpath "\$ORIGIN" "$i" || true
     done;
    )

    if [ "${DEBUG_MODE:-}" != "1" ]; then
        strip -s "$IO_LIBS"/* &>/dev/null || true
    fi

fi # BUNDLE_IO

# OFX MISC
if [ "$BUNDLE_MISC" = "1" ]; then
    OFX_MISC_VERSION=$TAG
    OFX_MISC_PATH=$INSTALLER/packages/$MISCPLUG_PKG
    mkdir -p "$OFX_MISC_PATH/data" "$OFX_MISC_PATH/meta" "$OFX_MISC_PATH/data/Plugins/OFX/Natron"
    $GSED "s/_VERSION_/${OFX_MISC_VERSION}/;s/_DATE_/${DATE}/" "$XML/openfx-misc.xml" > "$OFX_MISC_PATH/meta/package.xml"
    cat "$QS/openfx-misc.qs" > "$OFX_MISC_PATH/meta/installscript.qs"
    cp -a "$TMP_BINARIES_PATH/OFX/Plugins"/{CImg,Misc}.ofx.bundle "$OFX_MISC_PATH/data/Plugins/OFX/Natron/"

    if [ -d "$TMP_BINARIES_PATH/OFX/Plugins/Shadertoy.ofx.bundle" ]; then
        cp -a "$TMP_BINARIES_PATH/OFX/Plugins/Shadertoy.ofx.bundle" "$OFX_MISC_PATH/data/Plugins/OFX/Natron/"
        STOY_LIBS="$OFX_MISC_PATH/data/Plugins/OFX/Natron/Shadertoy.ofx.bundle/Libraries"
        mkdir -p "$STOY_LIBS"
        (cd "$STOY_LIBS" ;
         ln -sf ../../../../../lib/libgcc_s.so.1 .
         ln -sf ../../../../../lib/libstdc++.so.6 .
        )
    fi

    if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
        "$SDK_HOME/bin"/dump_syms "$OFX_MISC_PATH/data/Plugins/OFX/Natron"/*/*/*/CImg.ofx > "$TMP_BINARIES_PATH/symbols/CImg.ofx-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
        "$SDK_HOME/bin"/dump_syms "$OFX_MISC_PATH/data/Plugins/OFX/Natron"/*/*/*/Misc.ofx > "$TMP_BINARIES_PATH/symbols/Misc.ofx-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
        if [ -d "$OFX_MISC_PATH/data/Plugins/OFX/Natron/Shadertoy.ofx.bundle" ]; then
            "$SDK_HOME/bin"/dump_syms "$OFX_MISC_PATH/data/Plugins/OFX/Natron"/*/*/*/Shadertoy.ofx > "$TMP_BINARIES_PATH/symbols/Shadertoy.ofx-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
        fi
    fi

    if [ "${DEBUG_MODE:-}" != "1" ]; then
        find "$OFX_MISC_PATH/data/Plugins/OFX/Natron" -type f -exec strip -s {} \; &>/dev/null
    fi

    CIMG_LIBS="$OFX_MISC_PATH/data/Plugins/OFX/Natron/CImg.ofx.bundle/Libraries"
    mkdir -p "$CIMG_LIBS"

    OFX_CIMG_DEPENDS=$(ldd "$OFX_MISC_PATH/data/Plugins/OFX/Natron"/*/*/*/*|grep /opt | awk '{print $3}'|sort|uniq)
    for x in $OFX_CIMG_DEPENDS; do
        cp -fv "$x" "$CIMG_LIBS/"
    done
    rm -f "$CIMG_LIBS"/{libgcc*,libstdc*,libgomp*}
    (cd "$CIMG_LIBS" ;
     ln -sf ../../../../../lib/libgcc_s.so.1 .
     ln -sf ../../../../../lib/libstdc++.so.6 .
     ln -sf ../../../../../lib/libgomp.so.1 .
    )

    if [ "${DEBUG_MODE:-}" != "1" ]; then
        strip -s "$CIMG_LIBS"/* &>/dev/null || true
    fi

    MISC_LIBS="$OFX_MISC_PATH/data/Plugins/OFX/Natron/Misc.ofx.bundle/Libraries"
    mkdir -p "$MISC_LIBS"
    (cd "$MISC_LIBS" ;
     ln -sf ../../../../../lib/libgcc_s.so.1 .
     ln -sf ../../../../../lib/libstdc++.so.6 .
    )

fi # BUNDLE_MISC

# OFX ARENA
if [ "$BUNDLE_ARENA" = "1" ]; then
    OFX_ARENA_VERSION="$TAG"
    OFX_ARENA_PATH="$INSTALLER/packages/$ARENAPLUG_PKG"
    mkdir -p "$OFX_ARENA_PATH/meta" "$OFX_ARENA_PATH/data/Plugins/OFX/Natron"
    $GSED "s/_VERSION_/${OFX_ARENA_VERSION}/;s/_DATE_/${DATE}/" "$XML/openfx-arena.xml" > "$OFX_ARENA_PATH/meta/package.xml"
    cat "$QS/openfx-arena.qs" > "$OFX_ARENA_PATH/meta/installscript.qs"
    cp -av "$TMP_BINARIES_PATH/OFX/Plugins/Arena.ofx.bundle" "$OFX_ARENA_PATH/data/Plugins/OFX/Natron/"
    if [ -d "$TMP_BINARIES_PATH/OFX/Plugins/ArenaCL.ofx.bundle" ]; then
        cp -av "$TMP_BINARIES_PATH/Plugins/ArenaCL.ofx.bundle" "$OFX_ARENA_PATH/data/Plugins/OFX/Natron/"
    fi

    if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
        "$SDK_HOME/bin"/dump_syms "$OFX_ARENA_PATH/data/Plugins/OFX/Natron"/*/*/*/Arena.ofx > "$TMP_BINARIES_PATH/symbols/Arena.ofx-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
        if [ -d "$TMP_BINARIES_PATH/Plugins/ArenaCL.ofx.bundle" ]; then
            "$SDK_HOME/bin"/dump_syms "$OFX_ARENA_PATH/data/Plugins/OFX/Natron"/*/*/*/ArenaCL.ofx > "$TMP_BINARIES_PATH/symbols/ArenaCL.ofx-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
        fi
    fi

    if [ "${DEBUG_MODE:-}" != "1" ]; then
        find "$OFX_ARENA_PATH/data/Plugins/OFX/Natron" -type f -exec strip -s {} \; &>/dev/null
    fi

    ARENA_LIBS="$OFX_ARENA_PATH/data/Plugins/OFX/Natron/Arena.ofx.bundle/Libraries"
    mkdir -p "$ARENA_LIBS"
    OFX_ARENA_DEPENDS=$(ldd "$OFX_ARENA_PATH/data/Plugins/OFX/Natron"/*/*/*/* | grep /opt | awk '{print $3}'|sort|uniq)
    for x in $OFX_ARENA_DEPENDS; do
        cp -fv "$x" "$ARENA_LIBS/"
    done
    rm -f "$ARENA_LIBS"/{libcairo*,libpix*,liblcms*,libgomp*,libOpenColorIO*,libtinyxml*libbz2*,libfont*,libz.so*,libglib-2*,libgthread*,libpng*,libfree*,libexpat*,libgcc*,libstdc*}
    (cd "$ARENA_LIBS" ; 
     ln -sf ../../../../../lib/libbz2.so.1 .
     ln -sf ../../../../../lib/libexpat.so.1 .
     ln -sf ../../../../../lib/libfontconfig.so.1 .
     ln -sf ../../../../../lib/libfreetype.so.6 .
     ln -sf ../../../../../lib/libglib-2.0.so.0 .
     ln -sf ../../../../../lib/libgthread-2.0.so.0 .
     ln -sf ../../../../../lib/libpng16.so.16 .
     ln -sf ../../../../../lib/libz.so.1 .
     ln -sf ../../../../../lib/libgcc_s.so.1 .
     ln -sf ../../../../../lib/libstdc++.so.6 .
     ln -sf ../../../../../lib/libOpenColorIO.so.1 .
     ln -sf ../../../../../lib/libtinyxml.so .
     ln -sf ../../../../../lib/libgomp.so.1 .
     ln -sf ../../../../../lib/libpixman-1.so.0 .
     ln -sf ../../../../../lib/libcairo.so.2 .
     ln -sf ../../../../../lib/liblcms2.so.2 .
     for i in *.so*; do
         patchelf --set-rpath "\$ORIGIN" "$i" || true
     done;
    )

    if [ "${DEBUG_MODE:-}" != "1" ]; then
        strip -s "$ARENA_LIBS/*" &>/dev/null || true
    fi

fi # BUNDLE_ARENA

# OFX GMIC
if [ "$BUNDLE_GMIC" = "1" ]; then
    OFX_GMIC_VERSION=$TAG
    OFX_GMIC_PATH=$INSTALLER/packages/$GMICPLUG_PKG
    mkdir -p "$OFX_GMIC_PATH/data" "$OFX_GMIC_PATH/meta" "$OFX_GMIC_PATH/data/Plugins/OFX/Natron"
    $GSED "s/_VERSION_/${OFX_GMIC_VERSION}/;s/_DATE_/${DATE}/" "$XML/openfx-gmic.xml" > "$OFX_GMIC_PATH/meta/package.xml"
    cat "$QS/openfx-gmic.qs" > "$OFX_GMIC_PATH/meta/installscript.qs"
    cp -a "$TMP_BINARIES_PATH/OFX/Plugins"/GMIC.ofx.bundle "$OFX_GMIC_PATH/data/Plugins/OFX/Natron/"

    if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
        "$SDK_HOME/bin"/dump_syms "$OFX_GMIC_PATH/data/Plugins/OFX/Natron"/*/*/*/GMIC.ofx > "$TMP_BINARIES_PATH/symbols/GMIC.ofx-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
        if [ -d "$OFX_GMIC_PATH/data/Plugins/OFX/Natron/Shadertoy.ofx.bundle" ]; then
            "$SDK_HOME/bin"/dump_syms "$OFX_GMIC_PATH/data/Plugins/OFX/Natron"/*/*/*/Shadertoy.ofx > "$TMP_BINARIES_PATH/symbols/Shadertoy.ofx-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
        fi
    fi

    if [ "${DEBUG_MODE:-}" != "1" ]; then
        find "$OFX_GMIC_PATH/data/Plugins/OFX/Natron" -type f -exec strip -s {} \; &>/dev/null
    fi

    GMIC_LIBS="$OFX_GMIC_PATH/data/Plugins/OFX/Natron/GMIC.ofx.bundle/Libraries"
    mkdir -p "$GMIC_LIBS"
    OFX_GMIC_DEPENDS=$(ldd "$OFX_GMIC_PATH/data/Plugins/OFX/Natron"/*/*/*/*|grep /opt | awk '{print $3}'|sort|uniq)
    for x in $OFX_GMIC_DEPENDS; do
        cp -fv "$x" "$GMIC_LIBS/"
    done
    rm -f "$GMIC_LIBS"/{libgcc*,libstdc*,libgomp*}
    (cd "$GMIC_LIBS" ;
     ln -sf ../../../../../lib/libgcc_s.so.1 .
     ln -sf ../../../../../lib/libstdc++.so.6 .
     ln -sf ../../../../../lib/libgomp.so.1 .
    )
    if [ "${DEBUG_MODE:-}" != "1" ]; then
        strip -s "$CIMG_LIBS"/* &>/dev/null || true
    fi


fi # BUNDLE_GMIC

# NATRON
NATRON_PATH="$INSTALLER/packages/$NATRON_PKG"
mkdir -p "$NATRON_PATH/meta" "$NATRON_PATH/data/docs" "$NATRON_PATH/data/bin" "$NATRON_PATH/data/Resources" "$NATRON_PATH/data/Plugins/PyPlugs" "$NATRON_PATH/data/Resources/stylesheets"
$GSED "s/_VERSION_/${TAG}/;s/_DATE_/${DATE}/" "$XML/natron.xml" > "$NATRON_PATH/meta/package.xml"
cat "$QS/$PKGOS_ORIG/natron.qs" > "$NATRON_PATH/meta/installscript.qs"
cp -a "$TMP_BINARIES_PATH/docs/natron"/* "$NATRON_PATH/data/docs/"
cp "$TMP_BINARIES_PATH/Resources/stylesheets"/mainstyle.qss "$NATRON_PATH/data/Resources/stylesheets/"
cat "$TMP_BINARIES_PATH/docs/natron/LICENSE.txt" > "$NATRON_PATH/meta/natron-license.txt"
cp "$INC_PATH/natron/natron-mime.sh" "$NATRON_PATH/data/bin/"
cp "$TMP_BINARIES_PATH/PyPlugs"/* "$NATRON_PATH/data/Plugins/PyPlugs/"

if [ "${DISABLE_BREAKPAD:-}" != "1" ]; then
    cp "$TMP_BINARIES_PATH/bin/Natron" "$NATRON_PATH/data/bin/Natron-bin"
    cp "$TMP_BINARIES_PATH/bin/NatronRenderer" "$NATRON_PATH/data/bin/NatronRenderer-bin"
    cp "$TMP_BINARIES_PATH/bin/NatronCrashReporter" "$NATRON_PATH/data/bin/Natron"
    cp "$TMP_BINARIES_PATH/bin/NatronRendererCrashReporter" "$NATRON_PATH/data/bin/NatronRenderer"

    "$SDK_HOME/bin"/dump_syms "$NATRON_PATH/data/bin/Natron-bin" > "$TMP_BINARIES_PATH/symbols/Natron-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
    "$SDK_HOME/bin"/dump_syms "$NATRON_PATH/data/bin/NatronRenderer-bin" > "$TMP_BINARIES_PATH/symbols/NatronRenderer-${TAG}${BPAD_TAG:-}-${PKGOS}.sym"
else
    cp "$TMP_BINARIES_PATH/bin/Natron" "$NATRON_PATH/data/bin/Natron"
    cp "$TMP_BINARIES_PATH/bin/NatronRenderer" "$NATRON_PATH/data/bin/NatronRenderer"
fi

if [ -f "$TMP_BINARIES_PATH/bin/NatronProjectConverter" ]; then
    cp "$TMP_BINARIES_PATH/bin/NatronProjectConverter" "$NATRON_PATH/data/bin/"
fi

if [ -f "$TMP_BINARIES_PATH/bin/natron-python" ]; then
    cp "$TMP_BINARIES_PATH/bin/natron-python" "$NATRON_PATH/data/bin/"
fi

if [ "${DEBUG_MODE:-}" != "1" ]; then
    strip -s "$NATRON_PATH/data/bin"/Natron*
fi

rm "$NATRON_PATH/data/docs/TuttleOFX-README.txt" || true
#GLIBCXX 3.4.19 is for GCC 4.8.3, see https://gcc.gnu.org/onlinedocs/libstdc++/manual/abi.html
compat_version_script=3.4.19
compat_version=3.4.19
case "${TC_GCC:-}" in
    4.7.*)
        compat_version=3.4.17
        ;;
    4.8.[012])
        compat_version=3.4.18
        ;;
    4.8.[3456789])
        compat_version=3.4.19
        ;;
    4.9.*|5.0.*)
        compat_version=3.4.20
        ;;
    5.[123456789].*|6.0.*)
        compat_version=3.4.21
        ;;
    6.[123456789].*|7.*)
        compat_version=3.4.22
        ;;
esac

$GSED -e "s#${compat_version_script}#${compat_version}#" "$INC_PATH/scripts/Natron-Linux.sh" > "$NATRON_PATH/data/Natron"
$GSED -e "s#${compat_version_script}#${compat_version}#" -e "s#bin/Natron#bin/NatronRenderer#" "$INC_PATH/scripts/Natron-Linux.sh" > "$NATRON_PATH/data/NatronRenderer"
chmod a+x "$NATRON_PATH/data/Natron" "$NATRON_PATH/data/NatronRenderer"

# Docs
mkdir "$NATRON_PATH/data/Resources/docs"
cp -a "$TMP_BINARIES_PATH/Resources/docs/html" "$NATRON_PATH/data/Resources/docs/"

# OCIO
OCIO_VERSION="$COLOR_PROFILES_VERSION"
OCIO_PATH="$INSTALLER/packages/$PROFILES_PKG"
mkdir -p "$OCIO_PATH/meta" "$OCIO_PATH/data/Resources"
$GSED "s/_VERSION_/${OCIO_VERSION}/;s/_DATE_/${DATE}/" "$XML/ocio.xml" > "$OCIO_PATH/meta/package.xml"
cat "$QS/ocio.qs" > "$OCIO_PATH/meta/installscript.qs"
cp -a "$TMP_BINARIES_PATH/Resources/OpenColorIO-Configs" "$OCIO_PATH/data/Resources/"

# CORE LIBS
CLIBS_VERSION="$CORELIBS_VERSION"
CLIBS_PATH="$INSTALLER/packages/$CORELIBS_PKG"
mkdir -p "$CLIBS_PATH/meta" "$CLIBS_PATH/data/bin" "$CLIBS_PATH/data/lib" "$CLIBS_PATH/data/Resources/pixmaps"
$GSED "s/_VERSION_/${CLIBS_VERSION}/;s/_DATE_/${DATE}/" "$XML/corelibs.xml" > "$CLIBS_PATH/meta/package.xml"
cat "$QS/$PKGOS_ORIG/corelibs.qs" > "$CLIBS_PATH/meta/installscript.qs"
cp "$QTDIR/lib/libQtDBus.so.4" "$CLIBS_PATH/data/lib/"
cp "$TMP_BINARIES_PATH/Resources/pixmaps/natronIcon256_linux.png" "$CLIBS_PATH/data/Resources/pixmaps/"
cp "$TMP_BINARIES_PATH/Resources/pixmaps/natronProjectIcon_linux.png" "$CLIBS_PATH/data/Resources/pixmaps/"
cp -a "$TMP_BINARIES_PATH/Resources/etc"  "$CLIBS_PATH/data/Resources/"
cp -a "$SDK_HOME/share/poppler" "$CLIBS_PATH/data/Resources/"
cp -a "$QTDIR/plugins"/* "$CLIBS_PATH/data/bin/"
CORE_DEPENDS=$(ldd $(find "$NATRON_PATH/data/bin" -maxdepth 1 -type f) | grep /opt | awk '{print $3}'|sort|uniq)
for i in $CORE_DEPENDS; do
    if [ ! -f "$CLIBS_PATH/data/lib/$i" ]; then
        cp -fv "$i" "$CLIBS_PATH/data/lib/"
    fi
done
LIB_DEPENDS=$(env LD_LIBRARY_PATH="$CLIBS_PATH/data/lib:$LD_LIBRARY_PATH" ldd $(find "$CLIBS_PATH/data/lib" -maxdepth 1 -type f -name 'lib*.so*')|grep /opt | awk '{print $3}'|sort|uniq)
for y in $LIB_DEPENDS; do
    if [ ! -f "$CLIBS_PATH/data/lib/$y" ]; then
        cp -fv "$y" "$CLIBS_PATH/data/lib/"
    fi
done
PLUG_DEPENDS=$(env LD_LIBRARY_PATH="$CLIBS_PATH/data/lib:$LD_LIBRARY_PATH" ldd "$CLIBS_PATH/data/bin"/*/*.so* |grep /opt | awk '{print $3}'|sort|uniq)
for z in $PLUG_DEPENDS; do
    if [ ! -f "$CLIBS_PATH/data/lib/$z" ]; then
        cp -fv "$z" "$CLIBS_PATH/data/lib/"
    fi
done

if [ -f "$INC_PATH/misc/compat${BITS}.tgz" ] && [ "$SDK_VERSION" = "CY2015" ]; then
    tar xvf "$INC_PATH/misc/compat${BITS}.tgz" -C "$CLIBS_PATH/data/lib/"
fi

cp "$SDK_HOME/lib"/{libbz2.so.1,liblcms2.so.2,libcairo.so.2} "$CLIBS_PATH/data/lib/"
cp "$SDK_HOME/lib"/{libicudata.so.59,libicui18n.so.59,libicuuc.so.59} "$CLIBS_PATH/data/lib/"

if [ "$BUNDLE_IO" = "1" ]; then
    mv "$IO_LIBS"/{libOpenColor*,libtinyxml*,libgomp*} "$CLIBS_PATH/data/lib/"
    (cd "$IO_LIBS" ;
     ln -sf ../../../../../lib/libOpenColorIO.so.1 .
     ln -sf ../../../../../lib/libtinyxml.so .
     ln -sf ../../../../../lib/libgomp.so.1 .
    )
fi

# done in build-natron.sh
#mkdir -p "$CLIBS_PATH/data/Resources/etc/fonts/conf.d"
#cp "$SDK_HOME/etc/fonts/fonts.conf" "$CLIBS_PATH/data/Resources/etc/fonts/"
#cp "$SDK_HOME/share/fontconfig/conf.avail"/* "$CLIBS_PATH/data/Resources/etc/fonts/conf.d/"
#$GSED -i "s#${SDK_HOME}/#/#;/conf.d/d" "$CLIBS_PATH/data/Resources/etc/fonts/fonts.conf"

if [ "${DEBUG_MODE:-}" != "1" ]; then
    strip -s "$CLIBS_PATH/data/lib"/* &>/dev/null || true
    strip -s "$CLIBS_PATH/data/bin"/*/* &>/dev/null || true
fi

#Copy Python distrib
mkdir -p "$CLIBS_PATH/data/Plugins"

PYSIDE_DIR=PySide
if [ "$SDK_VERSION" = "CY2016" ] || [ "$SDK_VERSION" = "CY2017" ]; then
    PYSIDE_DIR=PySide2
fi
cp -a "$SDK_HOME/lib/python${PYVER}" "$CLIBS_PATH/data/lib/"
mv "$CLIBS_PATH/data/lib/python${PYVER}/site-packages/$PYSIDE_DIR" "$CLIBS_PATH/data/Plugins/"
(cd "$CLIBS_PATH/data/lib/python${PYVER}/site-packages"; ln -sf "../../../Plugins/$PYSIDE_DIR" . )
rm -rf "$CLIBS_PATH/data/lib/python${PYVER}"/{test,config,config-"${PYVER}m"}
PY_DEPENDS=$(env LD_LIBRARY_PATH="$CLIBS_PATH/data/lib:$LD_LIBRARY_PATH" ldd $(find "$CLIBS_PATH/data/Plugins/$PYSIDE_DIR" -maxdepth 1 -type f)| grep /opt | awk '{print $3}'|sort|uniq)
for y in $PY_DEPENDS; do
    cp -fv "$y" "$CLIBS_PATH/data/lib/"
done
(cd "$CLIBS_PATH" ; find . -type d -name __pycache__ -exec rm -rf {} \;)

if [ "${DEBUG_MODE:-}" != "1" ]; then
    strip -s "$CLIBS_PATH/data/Plugins/$PYSIDE_DIR"/* "$CLIBS_PATH/data/lib"/python*/* "$CLIBS_PATH/data/lib"/python*/*/* &>/dev/null || true
fi

# Strip Python
PYDIR="$CLIBS_PATH/data/lib/python${PYVER:-}"
find "${PYDIR}" -type f -name '*.pyo' -exec rm {} \;
(cd "${PYDIR}"; xargs rm -rf || true) < "$INC_PATH/python-exclude.txt"

# let Natron.sh handle gcc libs
#mkdir "$CLIBS_PATH/data/lib/compat"
#mv "$CLIBS_PATH/data/lib"/{libgomp*,libgcc*,libstdc*} "$CLIBS_PATH/data/lib/compat/"
if [ ! -f "$SRC_PATH/strings${BITS}.tgz" ]; then
    $WGET "$THIRD_PARTY_SRC_URL/strings${BITS}.tgz" -O "$SRC_PATH/strings${BITS}.tgz"
fi
tar xvf "$SRC_PATH/strings${BITS}.tgz" -C "$CLIBS_PATH/data/bin/"

# Fix RPATH (we dont want to link against system libraries when deployed)
# some libs (eg libssl, libcrypto, libpython2.7) have 555 right - make sure all are 755 before patching
(cd "$CLIBS_PATH/data/lib";
 chmod 755 *.so* || true
 for i in *.so*; do
     patchelf --set-rpath "\$ORIGIN" "$i" || true
 done;
 (cd "python${PYVER:-}/lib-dynload";
  chmod 755 *.so* || true
  for i in *.so*; do
      patchelf --set-rpath "\$ORIGIN/../.." "$i" || true
  done;
 )
 (cd "python${PYVER:-}/site-packages";
  chmod 755 *.so* || true
  for i in *.so*; do
      patchelf --set-rpath "\$ORIGIN/../.." "$i" || true
  done
 )
)
(cd "$CLIBS_PATH/data/Plugins/$PYSIDE_DIR";
 chmod 755 *.so* || true
 for i in *.so*; do
     patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
 done;
)
(cd "$CLIBS_PATH/data/bin";
 (cd accessible;
  chmod 755 *.so* || true
  for i in *.so*; do
      patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
  done;
 )
 (cd bearer;
  chmod 755 *.so* || true
  for i in *.so*; do
      patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
  done;
 )
 (cd codecs;
  chmod 755 *.so* || true
  for i in *.so*; do
      patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
  done;
 )
 (cd designer;
  chmod 755 *.so* || true
  for i in *.so*; do
      patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
  done;
 )
 (cd graphicssystems;
  chmod 755 *.so* || true
  for i in *.so*; do
      patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
  done;
 )
 (cd iconengines;
  chmod 755 *.so* || true
  for i in *.so*; do
      patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
  done;
 )
 (cd imageformats;
  chmod 755 *.so* || true
  for i in *.so*; do
      patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
  done;
 )
 (cd inputmethods;
  chmod 755 *.so* || true
  for i in *.so*; do
      patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
  done;
 )
 (cd qmltooling;
  chmod 755 *.so* || true
  for i in *.so*; do
      patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
  done;
 )
 (cd script;
  chmod 755 *.so* || true
  for i in *.so*; do
      patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
  done;
 )
 (cd sqldrivers;
  chmod 755 *.so* || true
  for i in *.so*; do
      patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
  done;
 )
 (cd webkit;
  chmod 755 *.so* || true
  for i in *.so*; do
      patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
  done;
 )
 if [ "$SDK_VERSION" = "CY2016" ] || [ "$SDK_VERSION" = "CY2017" ]; then
     (cd generic;
      chmod 755 *.so* || true
      for i in *.so*; do
          patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
      done;
     )
     (cd platforminputcontexts;
      chmod 755 *.so* || true
      for i in *.so*; do
          patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
      done;
     )
     (cd platforms;
      chmod 755 *.so* || true
      for i in *.so*; do
          patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
      done;
     )
     (cd qmltooling;
      chmod 755 *.so* || true
      for i in *.so*; do
          patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
      done;
     )
     (cd xcbglintegrations;
      chmod 755 *.so* || true
      for i in *.so*; do
          patchelf --set-rpath "\$ORIGIN/../../lib" "$i" || true
      done;
     )
 fi
)

# python zip
cd "$PYDIR"
if [ "${USE_QT5:-}" != 1 ]; then
  rm -rf site-packages/shiboken2* site-packages/PySide2 || true
fi
# remove pyc files
find . -type f -name '*.pyc' -exec rm {} \;
# generate pyo files
"$SDK_HOME/bin/python${PYVER:-}" -O -m compileall . || true
PY_ZIP_FILES=(bsddb compiler ctypes curses distutils email encodings hotshot idlelib importlib json logging multiprocessing pydoc_data sqlite3 unittest wsgiref xml)
zip -r "../python${PYVERNODOT}.zip" ./*.py* "${PY_ZIP_FILES[@]}"
rm -rf ./*.py* "${PY_ZIP_FILES[@]}" || true

if [ "$BUNDLE_IO" = "1" ]; then
    IO_BINS="ffmpeg ffprobe iconvert idiff igrep iinfo exrheader tiffinfo"
    mkdir -p "$OFX_IO_PATH/data/bin" || true
    for bin in $IO_BINS; do
        patchelf --set-rpath "\$ORIGIN/../Plugins/OFX/Natron/IO.ofx.bundle/Libraries" "$OFX_IO_PATH/data/$bin"
        mv "$OFX_IO_PATH/data/$bin" "$OFX_IO_PATH/data/bin/"
    done
fi

# Clean and perms
#chown root:root -R "$INSTALLER"/*
(cd "$INSTALLER"; find . -type d -name .git -exec rm -rf {} \;)

# Build repo and packages
if [ "$BUILD_CONFIG" = "SNAPSHOT" ]; then
    SNAPDATE="${NATRON_BRANCH}-${TAG}-"
fi
ONLINE_INSTALL="Natron-${PKGOS}-online-install-$ONLINE_TAG"
BUNDLED_INSTALL="Natron-${SNAPDATE:-}$NATRON_VERSION-${PKGOS}"
#REPO_DIR=$REPO_DIR_PREFIX$ONLINE_TAG

BUILD_ARCHIVE="$CWD/archive"
if [ ! -d "$BUILD_ARCHIVE" ]; then
    mkdir -p "$BUILD_ARCHIVE"
fi

if [ "${DEBUG_MODE:-}" = "1" ]; then
    BUNDLED_INSTALL="${BUNDLED_INSTALL}-debug"
fi

rm -rf "$REPO_DIR/packages" "$REPO_DIR/installers" || true
mkdir -p "$REPO_DIR"/{packages,installers}

if [ "${TAR_BUILD:-}" = "1" ]; then
    TAR_INSTALL="$TMP_PATH/$BUNDLED_INSTALL"
    mkdir -p "$TAR_INSTALL"
    (cd "$TAR_INSTALL" && cp -a "$INSTALLER/packages"/fr.*/data/* .)
    (cd "$TMP_PATH" && tar Jcf "${BUNDLED_INSTALL}-portable.tar.xz" "$BUNDLED_INSTALL" && rm -rf "$BUNDLED_INSTALL" &&  mv "${BUNDLED_INSTALL}-portable.tar.xz" "$REPO_DIR/installers/")
fi

if [ "${NO_INSTALLER:-}" != "1" ]; then
    "$SDK_HOME/installer/bin"/repogen -v --update-new-components -p "$INSTALLER/packages" -c "$INSTALLER/config/config.xml" "$REPO_DIR/packages"
    cd "$REPO_DIR/installers"
    if [ "${OFFLINE:-}" != "0" ]; then
        "$SDK_HOME/installer/bin"/binarycreator -v -f -p "$INSTALLER/packages" -c "$INSTALLER/config/config.xml" -i "$PACKAGES" "$REPO_DIR/installers/$BUNDLED_INSTALL" 
        tar zcf "$BUNDLED_INSTALL.tgz" "$BUNDLED_INSTALL"
        #ln -sf "$BUNDLED_INSTALL.tgz" "Natron-latest-$PKGOS-$ONLINE_TAG.tgz"
    fi
    "$SDK_HOME/installer/bin"/binarycreator -v -n -p "$INSTALLER/packages" -c "$INSTALLER/config/config.xml" "$ONLINE_INSTALL"
    tar zcf "$ONLINE_INSTALL.tgz" "$ONLINE_INSTALL"
fi

# collect debug versions for gdb
#if [ "$BUILD_CONFIG" = "STABLE" ]; then
#    DEBUG_DIR=$INSTALLER/Natron-$NATRON_VERSION-Linux${BITS}-Debug
#    rm -rf "$DEBUG_DIR"
#    mkdir "$DEBUG_DIR"
#    cp -a "$TMP_BINARIES_PATH/bin"/Natron* "$DEBUG_DIR/"
#    cp -a "$TMP_BINARIES_PATH/Plugins"/*.ofx.bundle/Contents/Linux*/*.ofx "$DEBUG_DIR/"
#    ( cd "$INSTALLER"; tar Jcf "Natron-$NATRON_VERSION-Linux${BITS}-Debug.tar.xz" "Natron-$NATRON_VERSION-Linux${BITS}-Debug" )
#    mv "${DEBUG_DIR}.tar.xz" "$BUILD_ARCHIVE"/
#fi

# Build native packages for linux

#if [ "${RPM_BUILD:-}" = "1" ] || [ "${DEB_BUILD:-}" = "1" ]; then
#   echo "#!/bin/bash" > "$INSTALLER/packages/fr.inria.natron/data/bin/postinstall.sh"
#   echo "echo \"Checking GCC compatibility for Natron ...\"" >> "$INSTALLER/packages/fr.inria.natron/data/bin/postinstall.sh"
#   echo "DIR=/opt/Natron2" >> "$INSTALLER/packages/fr.inria.natron/data/bin/postinstall.sh"
#   $GSED '42,82!d' "$INSTALLER/packages/fr.inria.natron/data/Natron" >> "$INSTALLER/packages/fr.inria.natron/data/bin/postinstall.sh"
#
#   cat <<'EOF' >> "$INSTALLER/packages/fr.inria.natron/data/bin/postinstall.sh"
#   if [ -f /usr/bin/update-mime-database ]; then
#       update-mime-database /usr/share/mime
#   fi
#   if [ -f /usr/bin/update-desktop-database ]; then
#       update-desktop-database /usr/share/applications
#   elif [ -f /usr/bin/xdg-desktop-menu ]; then
#       xdg-desktop-menu forceupdate
#   fi
#   EOF
#
#   chmod +x "$INSTALLER/packages/fr.inria.natron/data/bin/postinstall.sh"
#   $GSED -i '42,82d' "$INSTALLER/packages/fr.inria.natron/data/Natron"
#$GSED -i '42,82d' "$INSTALLER/packages/fr.inria.natron/data/NatronRenderer"
#fi

if [ "${RPM_BUILD:-}" = "1" ]; then
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
    $GSED "s/REPLACE_VERSION/$(echo "$NATRON_VERSION" | $GSED 's/-/./g')/;s#__NATRON_INSTALLER__#${INSTALLER}#;s#__INC__#${INC_PATH}#;s#__TMP_BINARIES_PATH__#${TMP_BINARIES_PATH}#" "$INC_PATH/natron/Natron.spec" > "$TMP_PATH/Natron.spec"
    #Only need to build once, so uncomment as default #echo "" | setsid rpmbuild -bb --define="%_gpg_name build@natron.fr" --sign $INC_PATH/natron/Natron-repo.spec
    echo "" | setsid rpmbuild -bb --define="%_gpg_name build@natron.fr" --sign "$TMP_PATH/Natron.spec"
    mv ~/rpmbuild/RPMS/*/Natron*.rpm "$REPO_DIR/installers/"
fi

if [ "${DEB_BUILD:-}" = "1" ]; then
    if [ ! -f "/usr/bin/dpkg-deb" ]; then
        if [ $EUID -ne 0 ]; then
            echo "Error: dpkg-dev not installed, please run: sudo yum install -y dpkg-dev"
            exit 2
        else
            yum install -y dpkg-dev
        fi
    fi

    rm -rf "$INSTALLER/natron"
    mkdir -p "$INSTALLER/natron"
    cd "$INSTALLER/natron"
    mkdir -p opt/Natron2 DEBIAN usr/share/doc/natron usr/share/{applications,pixmaps} usr/share/mime/packages usr/bin

    cp -a "$INSTALLER/packages"/fr.inria.*/data/* opt/Natron2/
    cp "$INC_PATH/debian"/post* DEBIAN/
    chmod +x DEBIAN/post*

    if [ "${BITS}" = "64" ]; then
        DEB_ARCH=amd64
    else
        DEB_ARCH=i386
    fi  
    DEB_VERSION=$(echo "$NATRON_VERSION" | $GSED 's/-/./g')
    DEB_DATE=$(date +"%a, %d %b %Y %T %z")
    DEB_SIZE=$(du -ks opt|cut -f 1)
    DEB_PKG="natron_${DEB_VERSION}_${DEB_ARCH}.deb"
    
    cat "$INC_PATH/debian/copyright" > usr/share/doc/natron/copyright
    $GSED "s/__VERSION__/${DEB_VERSION}/;s/__ARCH__/${DEB_ARCH}/;s/__SIZE__/${DEB_SIZE}/" "$INC_PATH/debian/control" > DEBIAN/control
    $GSED "s/__VERSION__/${DEB_VERSION}/;s/__DATE__/${DEB_DATE}/" "$INC_PATH/debian/changelog.Debian" > changelog.Debian
    gzip changelog.Debian
    mv changelog.Debian.gz usr/share/doc/natron/
    
    cat "$INC_PATH/natron/Natron2.desktop" > usr/share/applications/Natron2.desktop
    cat "$INC_PATH/natron/x-natron.xml" > usr/share/mime/packages/x-natron.xml
    cp "$TMP_BINARIES_PATH/Resources/pixmaps/natronIcon256_linux.png" usr/share/pixmaps/
    cp "$TMP_BINARIES_PATH/Resources/pixmaps/natronProjectIcon_linux.png" usr/share/pixmaps/
    
    (cd usr/bin; ln -sf ../../opt/Natron2/Natron .)
    (cd usr/bin; ln -sf ../../opt/Natron2/NatronRenderer .)

    # why?
    #chown root:root -R "$INSTALLER/natron"

    cd "$INSTALLER"
    dpkg-deb -Zxz -z9 --build natron
    mv natron.deb "$DEB_PKG"
    mv "$DEB_PKG" "$REPO_DIR/installers/"
fi

# waste of space #cp $REPO_DIR/installers/* "$BUILD_ARCHIVE/" || true
rm "$REPO_DIR/installers/$ONLINE_INSTALL" "$REPO_DIR/installers/$BUNDLED_INSTALL" || true

# Unit tests
#if [ "${UNIT_TEST:-}" = "1" ]; then
#  UNIT_LOG="$REPO_DIR/logs/unit_tests.Linux${BITS}.${TAG}.log"
#  if [ ! -d "$CWD/Natron-Tests" ]; then
#    cd $CWD
#    git clone $GIT_UNIT
#    cd Natron-Tests
#  else
#    cd $CWD/Natron-Tests
#    git pull
#  fi
#  echo "Running unit tests ..."
#  sh runOnServer.sh >& "$UNIT_LOG" || true
#fi

echo "All Done!!!"

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
