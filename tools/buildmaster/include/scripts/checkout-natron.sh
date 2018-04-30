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

# PYV=3 Use Python 3 otherwise use Python 2
# MKJOBS=X: Nb threads, 4 is default as defined in common
# BUILD_CONFIG=(SNAPSHOT,ALPHA,BETA,RC,STABLE,CUSTOM)
# CUSTOM_BUILD_USER_NAME="Toto" : to be set if BUILD_CONFIG=CUSTOM
# BUILD_NUMBER=X: To be set to indicate the revision number of the build. For example RC1,RC2, RC3 etc...
# DISABLE_BREAKPAD=1: Disable automatic crash report
# GIT_BRANCH=xxxx if not defined git tags from common will be used
# GIT_COMMIT=xxxx if not defined latest commit from GIT_BRANCH will be built

source common.sh
source common-buildmaster.sh

QMAKE_FLAGS_EXTRA=""

QMAKE_FLAGS_EXTRA="$QMAKE_FLAGS_EXTRA CONFIG+=silent" # comment for a verbose / non-silent build


#PID=$$
#if [ "${PIDFILE:-}" != "" ]; then
#  echo $PID > "$PIDFILE"
#fi

# We need REPO
if [ -z "${REPO:-}" ]; then
    echo "Please define REPO"
    exit 1
fi

# we need $BUILD_CONFIG
if [ -z "${BUILD_CONFIG:-}" ]; then
    echo "Please define BUILD_CONFIG"
    exit 1
fi

# Assume that $GIT_BRANCH is the branch to build, otherwise if empty use the NATRON_GIT_TAG in common.sh, but if BUILD_CONFIG=SNAPSHOT use MASTER_BRANCH
NATRON_BRANCH="${GIT_BRANCH:-}"
if [ -z "${NATRON_BRANCH:-}" ]; then
    if [ "$BUILD_CONFIG" = "SNAPSHOT" ]; then
        NATRON_BRANCH="${MASTER_BRANCH:-}"
        COMMITS_HASH="$CWD/commits-hash-$NATRON_BRANCH.sh"
    else
        NATRON_BRANCH="${NATRON_GIT_TAG:-}"
        COMMITS_HASH="$CWD/commits-hash.sh"
    fi
else
    COMMITS_HASH="$CWD/commits-hash-$NATRON_BRANCH.sh"
fi

echo "===> Using branch $NATRON_BRANCH"

# Setup tmp
if [ ! -d "$TMP_PATH" ]; then
    mkdir -p "$TMP_PATH"
fi
if [ -d "$TMP_PATH/Natron" ]; then
    rm -rf "$TMP_PATH/Natron"
fi

if [ ! -d "$TMP_BINARIES_PATH" ]; then
    mkdir -p "$TMP_BINARIES_PATH"
fi

# Setup env
if [ "$PKGOS" = "Linux" ]; then
    export PKG_CONFIG_PATH="$SDK_HOME/lib/pkgconfig:$SDK_HOME/libdata/pkgconfig"
    if [ -x "$SDK_HOME/bin/qmake" ]; then
        QTDIR="$SDK_HOME"
    elif [ -x "$SDK_HOME/qt4/bin/qmake" ]; then
        QTDIR="$SDK_HOME/qt4"
    elif [ -x "$SDK_HOME/qt5/bin/qmake" ]; then
        QTDIR="$SDK_HOME/qt5"
    else
        echo "Qt cannot be found in $SDK_HOME or $SDK_HOME/qt4 or $SDK_HOME/qt5"
        exit 1
    fi
    export QTDIR
    export BOOST_ROOT="$SDK_HOME"
    #export PYTHONHOME="$SDK_HOME"
    export LD_LIBRARY_PATH="$SDK_HOME/lib"
    export PATH="$SDK_HOME/gcc/bin:$SDK_HOME/bin:$HOME/.cabal/bin:$PATH"
    if [ "$ARCH" = "x86_64" ]; then
        export LD_LIBRARY_PATH="$SDK_HOME/gcc/lib64:$LD_LIBRARY_PATH"
    else
        export LD_LIBRARY_PATH="$SDK_HOME/gcc/lib:$LD_LIBRARY_PATH"
    fi
elif [ "$PKGOS" = "Windows" ]; then
#    export PKG_CONFIG_PATH="$SDK_HOME/fontconfig/lib/pkgconfig:$PKG_CONFIG_PATH"
    export QTDIR="$SDK_HOME" 
elif [ "$PKGOS" = "OSX" ]; then
    export QTDIR="$MACPORTS/libexec/qt4"
    export BOOST_ROOT="$SDK_HOME"
fi

# python
#if [ "$PYV" = "3" ]; then
#    export PYTHONPATH="$SDK_HOME/lib/python${PYVER}"
    #export PYTHON_INCLUDE="$SDK_HOME/include/python${PYVER}"
#else
#    export PYTHONPATH="$SDK_HOME/lib/python${PYVER}"
    #export PYTHON_INCLUDE="$SDK_HOME/include/python${PYVER}"
#fi

cd "$TMP_PATH"

# kill git if idle for too long
#"$KILLSCRIPT" $PID &
#KILLBOT=$! 


# OpenColorIO-Configs setup
OCIO_CONFIGS_VERSION=""
if [ "$BUILD_CONFIG" = "ALPHA" ] || [ "$BUILD_CONFIG" = "BETA" ] || [ "$BUILD_CONFIG" = "RC" ] || [ "$BUILD_CONFIG" = "STABLE" ]; then
    OCIO_CONFIGS_VERSION=$(echo $NATRON_RELEASE_BRANCH|$GSED 's/RB-//')
else
    case "$NATRON_BRANCH" in
        RB-*)
            OCIO_CONFIGS_VERSION=$(echo $NATRON_BRANCH | sed 's#RB-##;')
            ;;
    esac
fi

if [ "$OCIO_CONFIGS_VERSION" = "" ]; then
    OCIO_CONFIGS_VERSION="3.0"
    echo "Warnning: No OCIO config version found, setting to $OCIO_CONFIGS_VERSION"
fi

OCIO_CONFIGS_URL=https://github.com/NatronGitHub/OpenColorIO-Configs/archive/Natron-v${OCIO_CONFIGS_VERSION}.tar.gz

OCIO_CONFIGS_DIR="$SRC_PATH/OpenColorIO-Configs-Natron-v${OCIO_CONFIGS_VERSION}"
if [ ! -d "$OCIO_CONFIGS_DIR" ]; then
    mkdir -p "$SRC_PATH" || true
    $WGET $OCIO_CONFIGS_URL -O "$SRC_PATH/OpenColorIO-Configs-Natron-v${OCIO_CONFIGS_VERSION}.tar.gz"
    tar xf "$SRC_PATH/OpenColorIO-Configs-Natron-v${OCIO_CONFIGS_VERSION}.tar.gz" -C "$SRC_PATH"/
    rm -rf "$OCIO_CONFIGS_DIR/aces_1.0.1/baked" || true
    rm -rf "$OCIO_CONFIGS_DIR/aces_1.0.1/python" || true
fi
# set OCIO for Tests
export OCIO="$OCIO_CONFIGS_DIR/nuke-default/config.ocio"

# clone natron
$TIMEOUT 1800 $GIT clone "$REPO"
cd Natron

# checkout branch
git checkout "$NATRON_BRANCH"

# if we have a predefined commit use that, else use latest commit from branch
if [ -z "${GIT_COMMIT:-}" ]; then
    $TIMEOUT 1800 $GIT pull origin "$NATRON_BRANCH"
else
    $GIT checkout "$GIT_COMMIT"
fi

# Update submodule
$TIMEOUT 1800 $GIT submodule update -i --recursive

# the snapshot are always built with latest version
if [ "$NATRON_BRANCH" = "$MASTER_BRANCH" ]; then
    $TIMEOUT 1800 $GIT submodule update -i --recursive --remote
    #$TIMEOUT 1800 $GIT submodule foreach git pull origin master
fi

# kill idle checker
#kill -9 $KILLBOT

# color
ln -sf "$OCIO_CONFIGS_DIR" OpenColorIO-Configs

# add breakpad
rm -rf CrashReporter* BreakpadClient google-breakpad || true
cp -a "$CWD/../Breakpad/CrashReporter"* "$CWD/../Breakpad/BreakpadClient" "$CWD/../Breakpad/google-breakpad" "$CWD/../Breakpad/breakpadclient.pri" "$CWD/../Breakpad/breakpadpro.pri" .

# Get commit
REL_GIT_VERSION=$(git log|head -1|awk '{print $2}')

#Always bump NATRON_DEVEL_GIT, it is only used to version-stamp binaries
NATRON_REL_V="$REL_GIT_VERSION"

NATRON_MAJOR=$(echo "NATRON_VERSION_MAJOR" | $CC -E -P -include "$TMP_PATH/Natron/Global/Macros.h" - | tail -1)
NATRON_MINOR=$(echo "NATRON_VERSION_MINOR" | $CC -E -P -include "$TMP_PATH/Natron/Global/Macros.h" - | tail -1)
NATRON_REVISION=$(echo "NATRON_VERSION_REVISION" | $CC -E -P -include "$TMP_PATH/Natron/Global/Macros.h" - | tail -1)

if [ ! -f "$COMMITS_HASH" ]; then
    cat <<EOF > "$COMMITS_HASH"
#!/bin/sh
NATRON_DEVEL_GIT=#
IOPLUG_DEVEL_GIT=#
MISCPLUG_DEVEL_GIT=#
ARENAPLUG_DEVEL_GIT=#
GMICPLUG_DEVEL_GIT=#
CVPLUG_DEVEL_GIT=#
NATRON_VERSION_NUMBER=#
EOF
fi

$GSED -e "s/NATRON_DEVEL_GIT=.*/NATRON_DEVEL_GIT=${NATRON_REL_V}/" -e "s/NATRON_VERSION_NUMBER=.*/NATRON_VERSION_NUMBER=${NATRON_MAJOR}.${NATRON_MINOR}.${NATRON_REVISION}/" --in-place "$COMMITS_HASH"


echo "checkout-natron Done!"

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
