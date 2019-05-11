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

# common.sh is sourced multiple times across scripts so it may already have set CWD
if [ -z "${CWD:-}" ]; then
    CWD="$(pwd)"
fi

# Set common paths used across scripts
TMP_PATH="$CWD/tmp"
SRC_PATH="$CWD/src"
INC_PATH="$CWD/include"
# posix name for temporary directory
TMPDIR=${TMPDIR:-/tmp}
export TMPDIR

LANG="C"
export LANG
GSED="sed -b"
TIMEOUT="timeout"
WGET="wget --referer https://natron.fr/"


# Get OS
#
CHECK_OS="$(uname -s)"
case "$CHECK_OS" in
Linux)
    PKGOS=Linux
    ;;
Msys|MINGW64_NT-*|MINGW32_NT-*)
    PKGOS=Windows
    ;;
Darwin)
    PKGOS=OSX
    ;;
*)
    echo "$CHECK_OS not supported!"
    exit 1
    ;;
esac

unset LD_LIBRARY_PATH LD_RUN_PATH DYLD_LIBRARY_PATH LIBRARY_PATH CPATH PKG_CONFIG_PATH
# save the default PATH to avoid growing it each time we source this file
DEFAULT_PATH="${DEFAULT_PATH:-$PATH}"
PATH="$DEFAULT_PATH"

# Figure out architecture: 32bit, 64bit or Universal
#
# Default build flags
# backward compat with BIT for older build systems
BITS=${BITS:-${BIT:-}}
#SDK_VERSION=CY2015 # obsolete
SDK_VERSION=sdk


if [ "$PKGOS" = "Linux" ]; then
    if [ -z "${ARCH:-}" ]; then
        case $(uname -m) in
            i?86) ARCH=i686 ;;
            *) ARCH=$(uname -m) ;;
        esac
        export ARCH
    fi
    
    if [ "$ARCH" = "i686" ]; then
        BITS=32
    elif [ "$ARCH" = "x86_64" ]; then
        BITS=64
    else
        exit 1
    fi
    SDK_HOME="/opt/Natron-$SDK_VERSION"
    # Path where GPL builds are stored
    CUSTOM_BUILDS_PATH="$SDK_HOME"

    if [ "${COMPILE_TYPE:-}" = "debug" ] && [ "$PKGOS" = "Linux" ]; then
        SDK_VERSION="${SDK_VERSION}-debug"
    fi
elif [ "$PKGOS" = "OSX" ]; then
    macosx=$(uname -r | sed 's|\([0-9][0-9]*\)\.[0-9][0-9]*\.[0-9][0-9]*|\1|')
    case "$macosx" in
        [1-8])
            BITS=32
            ARCH=i386
           ;;
        9|10)
            BITS=Universal
            ARCH=Universal
            ;;
        *)
            BITS=64
            ARCH=x86_64
            ;;
    esac;
    # always deploy for the same version as the build system, since we use macports (for now)
    MACOSX_DEPLOYMENT_TARGET=$(sw_vers -productVersion | cut -d. -f-2)
    export MACOSX_DEPLOYMENT_TARGET
    MACPORTS="/opt/local"
    SDK_HOME="$MACPORTS"
    # Path where GPL builds are stored
    CUSTOM_BUILDS_PATH="/opt"

    # sed adds a newline on OSX! http://stackoverflow.com/questions/13325138/why-does-sed-add-a-new-line-in-osx
    # let's use gsed in binary mode.
    # gsed is provided by the gsed package on MacPorts or the gnu-sed package on homebrew
    # xhen using this variable, do not double-quote it ("$GSED"), because it contains options
    GSED="${MACPORTS}/bin/gsed -b"
    PATH="${MACPORTS}/bin:$PATH"
    # timeout is available in GNU coreutils:
    # sudo port install coreutils
    # or
    # brew install coreutils
    TIMEOUT="${MACPORTS}/bin/gtimeout"
elif [ "$PKGOS" = "Windows" ]; then
    if [ -z "${BITS:-}" ]; then
        echo "You must select a BITS"
        exit 1
    fi
    if [ "${BITS}" = "32" ]; then
        SDK_HOME="/mingw32"
        ARCH=i386
    elif [ "$BITS" = "64" ]; then
        SDK_HOME="/mingw64"
        ARCH=x86_64
    else
        echo "Unsupported BITS"
        exit 1
    fi
    # define TMP, which is used by QDir::tempPath http://doc.qt.io/qt-4.8/qdir.html#tempPath
    # so that the repogen binary uses different temp directories
    # https://github.com/olear/qtifw/blob/d3f31b3359af8415f9347347bb62e4d4ef94cf0d/src/libs/installer/fileutils.cpp#L503
    TMP="$TMPDIR/mingw${BITS}"
    export TMP
    if [ ! -d "$TMP" ]; then
        mkdir -p "$TMP"
    fi
    MINGW_PACKAGES_PATH="$CWD/../MINGW-packages"

    # Windows build bot can build both 32 and 64 bit so provide 2 different tmp directory depending on the build
    TMP_PATH="$TMP_PATH${BITS}"
    # Path where GPL builds are stored
    CUSTOM_BUILDS_PATH="$SDK_HOME"
else
    echo "Unsupported build platform"
    exit 1
fi

GIT=git
if [ "$PKGOS" = "Windows" ]; then
    # see https://github.com/Alexpux/MSYS2-packages/issues/735
    GIT="env PATH=/usr/bin git"
elif [ -x "$SDK_HOME/bin/git" ]; then
    GIT="env LD_LIBRARY_PATH=$SDK_HOME/lib $SDK_HOME/bin/git"
fi

# The version of Python used by the SDK and to build Natron
# Python 2 or 3, NOTE! v3 is probably broken, been untested for a long while
PYV=2
if [ "$PYV" = "3" ]; then
    PYVER=3.4
else
    PYVER=2.7
fi
PYVERNODOT=$(echo ${PYVER:-}| sed 's/\.//')

QT_VERSION_MAJOR=4


# Keep existing tag, else make a new one
if [ -z "${CURRENT_DATE:-}" ]; then
    CURRENT_DATE=$(date "+%Y%m%d%H%M")
fi

# Repo settings
#
if [ -f repo.sh ]; then
    source repo.sh
elif [ -f local.sh ]; then
    source local.sh
else
    REPO_DEST=buildmaster@downloads.natron.fr:../www/downloads.natron.fr
    REPO_URL=http://downloads.natron.fr
fi

THIRD_PARTY_SRC_URL=https://sourceforge.net/projects/natron/files/Third_Party/Sources
THIRD_PARTY_BIN_URL=https://natrongithub.github.io/files/bin


# Threads
#
# Set build threads to 2 if not exists
if [ -z "${MKJOBS:-}" ]; then
    if [ "$PKGOS" = "Linux" ]; then
        DEFAULT_MKJOBS=$(nproc)
    elif [ "$PKGOS" = "OSX" ]; then
        DEFAULT_MKJOBS=$(sysctl -n hw.ncpu)
    else
        DEFAULT_MKJOBS=2
    fi
    MKJOBS=$DEFAULT_MKJOBS
fi

rsync_remote () {
    local from="$1"
    local to="$2"
    local opts=""
    if [ $# -eq 3 ]; then
        opts="$3"
    fi
    $TIMEOUT 3600 rsync -avz -O --chmod=ug=rwx --no-perms --no-owner --no-group --progress --verbose -e 'ssh -oBatchMode=yes' $opts "$from" "${REMOTE_USER}@${REMOTE_URL}:$to"
}

if ! type -p keychain > /dev/null; then
    echo "Error: keychain not available, install from https://www.funtoo.org/Keychain"
    exit 1
fi

FFMPEG_PATH="$CUSTOM_BUILDS_PATH"
LIBRAW_PATH="$CUSTOM_BUILDS_PATH"
if [ "$PKGOS" = "Windows" ]; then
    if [ "${NATRON_LICENSE:-}" = "GPL" ]; then
        if [ -d "$CUSTOM_BUILDS_PATH/ffmpeg-gpl" ]; then
            FFMPEG_PATH="$CUSTOM_BUILDS_PATH/ffmpeg-gpl"
        elif [ -d "$CUSTOM_BUILDS_PATH/ffmpeg-gpl2" ]; then
            FFMPEG_PATH="$CUSTOM_BUILDS_PATH/ffmpeg-gpl2"
        else
            echo "FFmpeg cannot be fount in $SDK_HOME/ffmpeg-gpl or $SDK_HOME/ffmpeg-gpl2"
            echo "Setting FFMPEG_PATH=$SDK_HOME/ffmpeg-gpl2"
            FFMPEG_PATH="$SDK_HOME/ffmpeg-gpl2"
        fi
        LIBRAW_PATH="$CUSTOM_BUILDS_PATH/libraw-gpl2"        
    elif [ -z "${NATRON_LICENSE+x}" ] || [ "${NATRON_LICENSE:-}" = "COMMERCIAL" ]; then
        FFMPEG_PATH="$CUSTOM_BUILDS_PATH/ffmpeg-lgpl"
        LIBRAW_PATH="$CUSTOM_BUILDS_PATH/libraw-lgpl"
    fi
elif [ "$PKGOS" = "Linux" ]; then
    if [ "${NATRON_LICENSE:-}" = "GPL" ]; then
        if [ -d "$CUSTOM_BUILDS_PATH/ffmpeg-gpl" ]; then
            FFMPEG_PATH="$CUSTOM_BUILDS_PATH/ffmpeg-gpl"
        elif [ -d "$CUSTOM_BUILDS_PATH/ffmpeg-gpl2" ]; then
            FFMPEG_PATH="$CUSTOM_BUILDS_PATH/ffmpeg-gpl2"
        else
            echo "FFmpeg cannot be fount in $SDK_HOME/ffmpeg-gpl or $SDK_HOME/ffmpeg-gpl2"
            echo "Setting FFMPEG_PATH=$SDK_HOME/ffmpeg-gpl2"
            FFMPEG_PATH="$SDK_HOME/ffmpeg-gpl2"
        fi
        LIBRAW_PATH="$CUSTOM_BUILDS_PATH/libraw-gpl2"
    elif [ -z "${NATRON_LICENSE+x}" ] || [ "${NATRON_LICENSE:-}" = "COMMERCIAL" ]; then
        FFMPEG_PATH="$CUSTOM_BUILDS_PATH/ffmpeg-lgpl"
        LIBRAW_PATH="$CUSTOM_BUILDS_PATH/libraw-lgpl"
    fi
elif [ "$PKGOS" = "OSX" ]; then
    LIBRAW_PATH="$SDK_HOME"
    FFMPEG_PATH="$SDK_HOME"
    if [ "${NATRON_LICENSE:-}" = "GPL" ]; then
        if [ -d "$CUSTOM_BUILDS_PATH/ffmpeg-gpl" ]; then
            FFMPEG_PATH="$CUSTOM_BUILDS_PATH/ffmpeg-gpl"
        elif [ -d "$CUSTOM_BUILDS_PATH/ffmpeg-gpl2" ]; then
            FFMPEG_PATH="$CUSTOM_BUILDS_PATH/ffmpeg-gpl2"
        fi
        if [ -d "$CUSTOM_BUILDS_PATH/libraw-gpl2" ]; then
            LIBRAW_PATH="$CUSTOM_BUILDS_PATH/libraw-gpl2"
        fi
    elif [ -z "${NATRON_LICENSE+x}" ] || [ "${NATRON_LICENSE:-}" = "COMMERCIAL" ]; then
        if [ -d "$CUSTOM_BUILDS_PATH/ffmpeg-lgpl" ]; then
            FFMPEG_PATH="$CUSTOM_BUILDS_PATH/ffmpeg-lgpl"
        fi
        if [ -d "$CUSTOM_BUILDS_PATH/libraw-lgpl" ]; then
            LIBRAW_PATH="$CUSTOM_BUILDS_PATH/libraw-lgpl"
        fi
    fi
fi

if [ -d "$SDK_HOME/osmesa" ]; then
    OSMESA_PATH="$SDK_HOME/osmesa"
elif [ -d "/opt/osmesa" ]; then
    OSMESA_PATH="/opt/osmesa"
else
    echo "OSMesa cannot be found in $SDK_HOME/osmesa or /opt/osmesa"
    echo "Setting OSMESA_PATH=$SDK_HOME/osmesa"
    OSMESA_PATH="$SDK_HOME/osmesa"
fi

if [ -d "$SDK_HOME/llvm" ]; then
    LLVM_PATH="$SDK_HOME/llvm"
elif [ -d "/opt/llvm" ]; then
    LLVM_PATH="/opt/llvm"
else
    echo "LLVM cannot be found in $SDK_HOME/llvm or /opt/llvm"
    echo "Setting LLVM_PATH=$SDK_HOME/llvm"
    LLVM_PATH="$SDK_HOME/llvm"
fi


if [ -d "$SDK_HOME/qt${QT_VERSION_MAJOR}" ]; then
    QTDIR="$SDK_HOME/qt${QT_VERSION_MAJOR}"
elif [ -d "$SDK_HOME/libexec/qt${QT_VERSION_MAJOR}" ]; then
    QTDIR="$SDK_HOME/libexec/qt${QT_VERSION_MAJOR}"
elif [ -x "$SDK_HOME/bin/qmake" ] || [ -x "$SDK_HOME/bin/qmake.exe" ]; then
    QTDIR="$SDK_HOME"
else
    echo "Qt cannot be found in $SDK_HOME or $SDK_HOME/qt${QT_VERSION_MAJOR}"
    echo "setting QTDIR=$SDK_HOME/qt${QT_VERSION_MAJOR}"
    QTDIR="$SDK_HOME/qt${QT_VERSION_MAJOR}"
fi
export QTDIR

PKG_CONFIG_PATH=
if [ "$PKGOS" = "Linux" ]; then
    PATH="$SDK_HOME/bin:$QTDIR/bin:$SDK_HOME/gcc/bin:$FFMPEG_PATH/bin:$LIBRAW_PATH/bin:$PATH"
    LIBRARY_PATH="$SDK_HOME/lib:$QTDIR/lib:$SDK_HOME/gcc/lib64:$SDK_HOME/gcc/lib:$FFMPEG_PATH/lib:$LIBRAW_PATH/lib"
    LD_LIBRARY_PATH="$SDK_HOME/lib:$QTDIR/lib:$SDK_HOME/gcc/lib64:$SDK_HOME/gcc/lib:$FFMPEG_PATH/lib:$LIBRAW_PATH/lib"
    #LD_RUN_PATH="$SDK_HOME/lib:$QTDIR/lib:$SDK_HOME/gcc/lib:$FFMPEG_PATH/lib:$LIBRAW_PATH/lib"
    PKG_CONFIG_PATH="$SDK_HOME/lib/pkgconfig:$SDK_HOME/share/pkgconfig:$SDK_HOME/libdata/pkgconfig:$FFMPEG_PATH/lib/pkgconfig:$LIBRAW_PATH/lib/pkgconfig:$OSMESA_PATH/lib/pkgconfig:$QTDIR/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
elif [ "$PKGOS" = "Windows" ]; then
    PKG_CONFIG_PATH="$FFMPEG_PATH/lib/pkgconfig:$LIBRAW_PATH/lib/pkgconfig:$OSMESA_PATH/lib/pkgconfig:$QTDIR/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
elif [ "$PKGOS" = "OSX" ]; then
    PKG_CONFIG_PATH="$FFMPEG_PATH/lib/pkgconfig:$LIBRAW_PATH/lib/pkgconfig:$OSMESA_PATH/lib/pkgconfig:$QTDIR/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
fi
export PKG_CONFIG_PATH

# Load compiler related stuff
source $CWD/compiler-common.sh


# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
