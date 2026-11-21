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

# see https://stackoverflow.com/a/24067243
function version_gt() { test "$(printf '%s\n' "$@" | $GSORT -V | head -n 1)" != "$1"; }

# common.sh is sourced multiple times across scripts so it may already have set CWD
if [ -z "${CWD:-}" ]; then
    CWD="$(pwd)"
fi

# Set common paths used across scripts
if [ -z "${WORKSPACE+x}" ] && [ -z "${GEN_DOCKERFILE+x}" ]; then
    (>&2 echo "Error: 'WORKSPACE' env is not defined -- required for build")
    exit 1
fi
TMP_PATH="${WORKSPACE:-}/tmp"
SRC_PATH="${WORKSPACE:-}/src"
INC_PATH="$CWD/include"
# posix name for temporary directory
TMPDIR=${TMPDIR:-/tmp}
export TMPDIR

LANG="C"
export LANG
GSED="sed -b"
GSORT="sort"
TIMEOUT="timeout"
#WGET="wget --referer https://natron.fr/"

# tell curl to continue downloads and follow redirects
curlopts="--location --continue-at -"
CURL="curl $curlopts"

# Get OS
#
system="$(uname -s)"
case "$system" in
Linux)
    PKGOS=${PKGOS:-Linux}
    ;;
Msys|MSYS_NT-*|MINGW64_NT-*|MINGW32_NT-*|UCRT64_NT-*|CLANG64_NT-*|CLANGARM64_NT-*)
    # MSYS2: uname -s is "<MSYSTEM>_NT-<Windows version>"
    PKGOS=${PKGOS:-Windows}
    ;;
Darwin)
    PKGOS=${PKGOS:-OSX}
    if [ "$(uname -m)" = "arm64" ]; then
        # No Qt4 on apple Silicon
        QT_VERSION_MAJOR=${QT_VERSION_MAJOR:-5}
    fi
    ;;
*)
    (>&2 echo "$system not supported!")
    exit 1
    ;;
esac

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
            arm64) ARCH=aarch64 ;; # e.g. when generating a Dockerfile on macOS
            *) ARCH=$(uname -m) ;;
        esac
        export ARCH
    fi
    
    if [ "$ARCH" = "i686" ]; then
        BITS=32
    elif [ "$ARCH" = "x86_64" ] || [ "$ARCH" = "aarch64" ]; then
        BITS=64
    else
        (>&2 echo "Error: unsupported Linux architecture: $ARCH")
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
            ARCH=$(uname -m)
            ;;
    esac;
    # Deployment target. Release builds set NATRON_MACOS_MIN to the oldest macOS supported by the
    # release (NATRON_MACOS_RELEASE_MIN in macos-release.sh, 14.0; tools/tart/provision.sh sets it):
    # the MacPorts libraries that are bundled must be built for it too (macosx_deployment_target
    # in macports.conf, see INSTALL_MACOS.md), and launchBuildMain.sh checks the DMG with
    # check-minos.sh. Otherwise (development builds), build for this Mac's macOS, so that the
    # plug-ins, which refuse to bundle a library newer than the target (OFX_DEPLOY_STRICT_MINOS),
    # can bundle MacPorts' ordinary binary archives. An explicit MACOSX_DEPLOYMENT_TARGET wins.
    # shellcheck source=macos-release.sh
    . "$(dirname "${BASH_SOURCE[0]}")/macos-release.sh"
    if [ -z "${MACOSX_DEPLOYMENT_TARGET:-}" ]; then
        if [ -n "${NATRON_MACOS_MIN:-}" ]; then
            MACOSX_DEPLOYMENT_TARGET=$NATRON_MACOS_MIN
        else
            MACOSX_DEPLOYMENT_TARGET=$(sw_vers -productVersion)
            case "$MACOSX_DEPLOYMENT_TARGET" in
                10.*) MACOSX_DEPLOYMENT_TARGET=$(echo "$MACOSX_DEPLOYMENT_TARGET" | cut -d. -f-2) ;;
                *) MACOSX_DEPLOYMENT_TARGET=$(echo "$MACOSX_DEPLOYMENT_TARGET" | cut -d. -f1).0 ;;
            esac
        fi
    fi
    export MACOSX_DEPLOYMENT_TARGET
    MACPORTS="/opt/local"
    LOCAL="/usr/local"
    HOMEBREW="${LOCAL}"
    if [ -e "/brew2/local" ]; then
        HOMEBREW="/brew2/local"
    elif [ -e "/opt/homebrew" ]; then
        HOMEBREW="/opt/homebrew"
    fi
    if [ -e "${MACPORTS}" ]; then
        SDK_HOME="${SDK_HOME:-${MACPORTS}}"
    elif [ -e "${HOMEBREW}" ]; then
        SDK_HOME="${SDK_HOME:-${HOMEBREW}}"
    else
        SDK_HOME="${SDK_HOME:-${LOCAL}}"
    fi

    # Path where GPL builds are stored
    CUSTOM_BUILDS_PATH="/opt"

    # sed adds a newline on OSX! http://stackoverflow.com/questions/13325138/why-does-sed-add-a-new-line-in-osx
    # let's use gsed in binary mode.
    # gsed is provided by the gsed package on MacPorts or the gnu-sed package on homebrew
    # when using this variable, do not double-quote it ("$GSED"), because it contains options
    GSED="${SDK_HOME}/bin/gsed -b"
    GSORT="${SDK_HOME}/bin/gsort"
    PATH="${SDK_HOME}/bin:$PATH"
    # timeout is available in GNU coreutils:
    # sudo port install coreutils
    # or
    # brew install coreutils
    TIMEOUT="${SDK_HOME}/bin/gtimeout"
elif [ "$PKGOS" = "Windows" ]; then
    # MSYS2 MinGW environment (UCRT64, CLANG64, CLANGARM64): everything comes from the environment
    # (MINGW_PREFIX, MINGW_PACKAGE_PREFIX, MSYSTEM_CARCH), never from uname -m, which reports
    # the x86_64 MSYS runtime when it runs emulated on ARM64 (#1494).
    if [ -z "${MINGW_PREFIX:-}" ] || [ -z "${MSYSTEM_CARCH:-}" ]; then
        (>&2 echo "Error: run this in an MSYS2 UCRT64, CLANG64 or CLANGARM64 shell (MINGW_PREFIX and MSYSTEM_CARCH are not set; MSYSTEM=${MSYSTEM:-})")
        exit 1
    fi
    SDK_HOME="$MINGW_PREFIX"
    ARCH="$MSYSTEM_CARCH"
    case "$ARCH" in
        x86_64)
            WIN_ARCH=x86_64           # installer names (Natron-<version>-Windows-x86_64)
            WIN_MANIFEST_ARCH=amd64   # processorArchitecture of the plug-in manifests
            OFX_ARCHDIR=Win64         # OFX plug-in binary directory (ofxhArchDirs.h)
            ;;
        aarch64)
            WIN_ARCH=arm64
            WIN_MANIFEST_ARCH=arm64
            OFX_ARCHDIR=Win-arm64
            ;;
        *)
            (>&2 echo "Error: unsupported Windows architecture MSYSTEM_CARCH=$ARCH")
            exit 1
            ;;
    esac
    # BITS: only 64-bit Windows is supported (MSYS2 has no 32-bit UCRT or clang environment)
    if [ -n "${BITS:-}" ] && [ "$BITS" != "64" ]; then
        (>&2 echo "Error: Unsupported BITS=$BITS (only 64-bit Windows builds are supported)")
        exit 1
    fi
    BITS=64
    # the compiler of the environment: gcc on UCRT64, clang on CLANG64 and CLANGARM64
    case "${MINGW_PACKAGE_PREFIX:-}" in
        *-clang-*) MINGW_TOOLCHAIN=clang ;;
        *) MINGW_TOOLCHAIN=gcc ;;
    esac
    # Inno Setup's Setup.exe for this environment (${MINGW_PACKAGE_PREFIX}-natron-setup, built for
    # x64 or arm64): in the environment's prefix since pkgrel 6. Older packages, which were only
    # built for UCRT64 (x64), installed it as /Setup.exe.
    NATRON_SETUP_EXE="${MINGW_PREFIX}/share/natron-setup/Setup.exe"
    if [ ! -f "$NATRON_SETUP_EXE" ] && [ "$ARCH" = "x86_64" ] && [ -f /Setup.exe ]; then
        NATRON_SETUP_EXE=/Setup.exe
    fi
    # define TMP, which is used by QDir::tempPath http://doc.qt.io/qt-4.8/qdir.html#tempPath,
    # so that the Qt programs of each MSYS2 environment use their own temporary directory
    TMP="$TMPDIR/${MSYSTEM:-mingw64}"
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
    (>&2 echo "Error: Unsupported build platform PKGOS=$PKGOS")
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
# To default to Python 2, move the Python 3 section below the Python 2 section
PYTHON_ABIFLAGS=
if [[ $(type -P python3) ]]; then
    PY3VER="$(python3 -c "import platform; print('.'.join(platform.python_version_tuple()[:2]))")"
    PY3VERNODOT=$(echo ${PY3VER:-}| sed 's/\.//')
    if [ -z "${PYV:-}" ]; then
        PYV=3
    fi
fi
if [[ $(type -P python2) ]]; then
    PY2VER="$(python2 -c "import platform; print('.'.join(platform.python_version_tuple()[:2]))")"
    PY2VERNODOT=$(echo ${PY2VER:-}| sed 's/\.//')
    if [ -z "${PYV:-}" ]; then
        # default to Python 2 if present
        PYV=2
    fi
    case "${RELEASE_TAG:-}" in
    1.*|2.[01234].*)
        PYV=2
    esac
    case "${GIT_BRANCH:-}" in
    RB-1|RB-1.*|RB-2.[01234])
        PYV=2
    esac
fi
if [ "${PYV:-}" = 3 ]; then
    PYVER="${PY3VER}"
    PYVERNODOT="${PY3VERNODOT}"
    case "${PYVER}" in
    3.[01234567])
        PYTHON_ABIFLAGS="m"
        ;;
    esac
    PYTHON_HOME="$(python${PYV} -c "import sys; print(sys.prefix)")"
fi
if [ "${PYV:-}" = 2 ]; then
    PYVER="${PY2VER}"
    PYVERNODOT="${PY2VERNODOT}"
    PYTHON_ABIFLAGS=""
    PYTHON_HOME="$(python${PYV} -c "import sys; print(sys.prefix)")"
fi
PY_EXE="$SDK_HOME/bin/python${PYV}"
# PY_LIB="$SDK_HOME/lib/libpython${PYVER}.so"
# PY_INC="$SDK_HOME/include/python${PYVER}"
# PYTHON_HOME="$SDK_HOME"
# PYTHON_PATH="$SDK_HOME/lib/python${PYVER}"
# PYTHON_INCLUDE="$SDK_HOME/include/python${PYVER}"
# export PYTHON_PATH PYTHON_INCLUDE


QT_VERSION_MAJOR=${QT_VERSION_MAJOR:-5}
if [[ ${QT_VERSION_MAJOR} -lt 5 ]]; then
    echo "QT_VERSION_MAJOR ${QT_VERSION_MAJOR} not supported."
    exit 1
fi

unset LD_LIBRARY_PATH LD_RUN_PATH DYLD_LIBRARY_PATH LIBRARY_PATH CPATH PKG_CONFIG_PATH
# save the default PATH to avoid growing it each time we source this file
DEFAULT_PATH="${DEFAULT_PATH:-$PATH}"
PATH="$DEFAULT_PATH"

# Keep existing tag, else make a new one
if [ -z "${CURRENT_DATE:-}" ]; then
    CURRENT_DATE=$(date -u "+%Y%m%d%H%M")
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
    case "$system" in
    Linux)
        DEFAULT_MKJOBS=$(nproc)
        ;;
    Darwin)
        DEFAULT_MKJOBS=$(sysctl -n hw.ncpu)
        ;;
    *)
        DEFAULT_MKJOBS=2
        ;;
    esac
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

if [ -f "$HOME/.ssh/id_rsa_build_master" ] && ! type -p keychain > /dev/null; then
    (>&2 echo "Error: keychain not available, install from https://www.funtoo.org/Keychain")
    exit 1
fi

FFMPEG_PATH="$CUSTOM_BUILDS_PATH"
LIBRAW_PATH="$CUSTOM_BUILDS_PATH"
# macOS COMMERCIAL builds only: the prefix of the libheif without x264/x265 that replaces the
# bundled libheif (build-plugins.sh)
LIBHEIF_PATH=""
if [ "$PKGOS" = "Windows" ]; then
    if [ "${NATRON_LICENSE:-}" = "GPL" ]; then
        if [ -d "$CUSTOM_BUILDS_PATH/ffmpeg-gpl" ]; then
            FFMPEG_PATH="$CUSTOM_BUILDS_PATH/ffmpeg-gpl"
        elif [ -d "$CUSTOM_BUILDS_PATH/ffmpeg-gpl2" ]; then
            FFMPEG_PATH="$CUSTOM_BUILDS_PATH/ffmpeg-gpl2"
        else
            (>&2 echo "Warning: FFmpeg cannot be found in $SDK_HOME/ffmpeg-gpl or $SDK_HOME/ffmpeg-gpl2")
            (>&2 echo "Info: Setting FFMPEG_PATH=$SDK_HOME/ffmpeg-gpl2")
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
            (>&2 echo "FFmpeg cannot be found in $SDK_HOME/ffmpeg-gpl or $SDK_HOME/ffmpeg-gpl2")
            (>&2 echo "Info: Setting FFMPEG_PATH=$SDK_HOME/ffmpeg-gpl2")
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
        # the MacPorts overlay's subports ffmpeg-lgpl, libraw-lgpl and libheif-lgpl
        # (tools/MacPorts), installed in $SDK_HOME/libexec next to the GPL ffmpeg and libraw
        # and to libheif (which links the GPL x264 and x265), or a custom build (#1302, #1177)
        for _lgpl_prefix in "$SDK_HOME/libexec/ffmpeg-lgpl" "$CUSTOM_BUILDS_PATH/ffmpeg-lgpl"; do
            if [ -d "$_lgpl_prefix" ]; then
                FFMPEG_PATH="$_lgpl_prefix"
                break
            fi
        done
        for _lgpl_prefix in "$SDK_HOME/libexec/libraw-lgpl" "$CUSTOM_BUILDS_PATH/libraw-lgpl"; do
            if [ -d "$_lgpl_prefix" ]; then
                LIBRAW_PATH="$_lgpl_prefix"
                break
            fi
        done
        for _lgpl_prefix in "$SDK_HOME/libexec/libheif-lgpl" "$CUSTOM_BUILDS_PATH/libheif-lgpl"; do
            if [ -d "$_lgpl_prefix" ]; then
                LIBHEIF_PATH="$_lgpl_prefix"
                break
            fi
        done
        unset _lgpl_prefix
    fi
fi

# Windows COMMERCIAL builds: the libheif without x264/x265 and the libtiff without JBIG-KIT
# (natron_libheif-lgpl and natron_libtiff-lgpl, #1449, #1177), whose bin/ come first in the DLL
# search directories of genDllVersions.sh. Empty if they are not installed (genDllVersions.sh skips
# empty entries, and fails on a COMMERCIAL build without them).
# shellcheck disable=SC2034 # (LIBTIFF_PATH is used by build-Windows-installer.sh)
if [ "$PKGOS" = "Windows" ]; then
    LIBHEIF_PATH=""
    LIBTIFF_PATH=""
    if [ "${NATRON_LICENSE:-}" = "COMMERCIAL" ]; then
        if [ -d "$SDK_HOME/libheif-lgpl" ]; then
            LIBHEIF_PATH="$SDK_HOME/libheif-lgpl"
        fi
        if [ -d "$SDK_HOME/libtiff-lgpl" ]; then
            LIBTIFF_PATH="$SDK_HOME/libtiff-lgpl"
        fi
    fi
fi

# ImageMagick 7, used by openfx-arena (build-plugins.sh). On Linux and macOS, ImageMagick 6
# stays in SDK_HOME (on macOS, MacPorts' ImageMagick, which other ports use).
# - Linux: the SDK's $SDK_HOME/magick7 (include/scripts/pkg/imagemagick7.sh);
# - macOS: MacPorts' ImageMagick7 +natron (tools/MacPorts/graphics/ImageMagick7), installed in
#   $SDK_HOME/lib/ImageMagick7;
# - Windows: natron_imagemagick (tools/MINGW-packages/mingw-w64-imagemagick), which replaces
#   ImageMagick 6 in the MSYS2 prefix.
if [ -z "${MAGICK7_PREFIX:-}" ]; then
    if [ "$PKGOS" = "Linux" ]; then
        MAGICK7_PREFIX="$SDK_HOME/magick7"
    elif [ "$PKGOS" = "OSX" ]; then
        MAGICK7_PREFIX="$SDK_HOME/lib/ImageMagick7"
    elif [ "$PKGOS" = "Windows" ]; then
        MAGICK7_PREFIX="${MINGW_PREFIX:-$SDK_HOME}"
    fi
fi

if [ -n "${OSMESA_PREFIX:-}" ] && [ -z "${LLVM_PREFIX:-}" ]; then
    # an OSMesa install needs the LLVM it was built with (e.g. Mesa 25.0.7 needs LLVM >= 15)
    (>&2 echo "Error: OSMESA_PREFIX=$OSMESA_PREFIX is set, so LLVM_PREFIX must be set too (e.g. /opt/llvm-19.1.7)")
    exit 1
fi
if [ -n "${OSMESA_PREFIX:-}" ]; then
    # e.g. OSMESA_PREFIX=/opt/osmesa-25.0.7 (with LLVM_PREFIX=/opt/llvm-19.1.7)
    OSMESA_PATH="${OSMESA_PREFIX}"
elif [ -d "$SDK_HOME/osmesa" ]; then
    OSMESA_PATH="$SDK_HOME/osmesa"
elif [ -d "/opt/osmesa" ]; then
    OSMESA_PATH="/opt/osmesa"
else
    (>&2 echo "Warning: OSMesa cannot be found in $SDK_HOME/osmesa or /opt/osmesa")
    (>&2 echo "Info: Setting OSMESA_PATH=$SDK_HOME/osmesa")
    OSMESA_PATH="$SDK_HOME/osmesa"
fi

if [ -n "${LLVM_PREFIX:-}" ]; then
    LLVM_PATH="${LLVM_PREFIX}"
elif [ "$PKGOS" = "Windows" ] && [ -x "$SDK_HOME/bin/llvm-config.exe" ]; then
    # MSYS2's llvm, which the osmesa package was built with
    LLVM_PATH="$SDK_HOME"
elif [ -d "$SDK_HOME/llvm" ]; then
    LLVM_PATH="$SDK_HOME/llvm"
elif [ -d "/opt/llvm" ]; then
    LLVM_PATH="/opt/llvm"
else
    (>&2 echo "Warning: LLVM cannot be found in $SDK_HOME/llvm or /opt/llvm")
    (>&2 echo "Info: Setting LLVM_PATH=$SDK_HOME/llvm")
    LLVM_PATH="$SDK_HOME/llvm"
fi


# Print the qmake binary of a Qt prefix, if there is one.
# MSYS2 renamed Qt5's qmake to qmake-qt5 when it made Qt6 the default Qt
# (msys2/MINGW-packages 389f888c), so an unsuffixed qmake is now either absent
# or belongs to a different Qt major version. Probe the suffixed names first.
find_qmake() {
    local prefix="$1"
    local name exe
    for name in "qmake-qt${QT_VERSION_MAJOR}" "qmake${QT_VERSION_MAJOR}" qmake; do
        for exe in "$prefix/bin/$name" "$prefix/bin/$name.exe"; do
            if [ -x "$exe" ]; then
                echo "$exe"
                return 0
            fi
        done
    done
    return 1
}

QMAKE_IN_SDK_HOME=
if [ -d "$SDK_HOME/qt${QT_VERSION_MAJOR}" ]; then
    QTDIR="$SDK_HOME/qt${QT_VERSION_MAJOR}"
elif [ -d "$SDK_HOME/libexec/qt${QT_VERSION_MAJOR}" ]; then
    QTDIR="$SDK_HOME/libexec/qt${QT_VERSION_MAJOR}"
elif QMAKE_IN_SDK_HOME=$(find_qmake "$SDK_HOME"); then
    QTDIR="$SDK_HOME"
else
    (>&2 echo "Warning: Qt cannot be found in $SDK_HOME or $SDK_HOME/qt${QT_VERSION_MAJOR}")
    (>&2 echo "Info: setting QTDIR=$SDK_HOME/qt${QT_VERSION_MAJOR}")
    QTDIR="$SDK_HOME/qt${QT_VERSION_MAJOR}"
fi

# The qmake binary can be overridden explicitly with NATRON_QMAKE, e.g. to build
# against a Qt that does not follow any of the layouts probed above (QTDIR is
# still the probed one). A QMAKE inherited from the environment is not trusted:
# it may come from an unrelated tool or shell profile, and would silently take
# precedence over the probe. It is replaced by the probe result, with a warning
# when it differs (common.sh is sourced again by child scripts, which inherit
# the QMAKE exported below, so an identical value is expected and silent).
if [ -n "$QMAKE_IN_SDK_HOME" ]; then
    QMAKE_PROBED="$QMAKE_IN_SDK_HOME"
else
    QMAKE_PROBED=$(find_qmake "$QTDIR") || QMAKE_PROBED="$QTDIR/bin/qmake"
fi
if [ -n "${NATRON_QMAKE:-}" ]; then
    if [ "${NATRON_QMAKE}" != "${QMAKE:-}" ]; then
        (>&2 echo "Info: using NATRON_QMAKE=${NATRON_QMAKE} instead of the probed ${QMAKE_PROBED}")
    fi
    QMAKE="${NATRON_QMAKE}"
else
    if [ -n "${QMAKE:-}" ] && [ "${QMAKE}" != "${QMAKE_PROBED}" ]; then
        (>&2 echo "Warning: ignoring QMAKE=${QMAKE} from the environment, using ${QMAKE_PROBED}")
        (>&2 echo "Info: set NATRON_QMAKE to override the qmake binary")
    fi
    QMAKE="${QMAKE_PROBED}"
fi
unset QMAKE_PROBED
export QTDIR QMAKE

BOOST_ROOT="$SDK_HOME"

PKG_CONFIG_PATH=
if [ "$PKGOS" = "Linux" ]; then
    # The compiler is gcc-toolset (Rocky Linux 9), and its C++ runtime is the system one:
    # don't use the SDK's old self-built GCC ($SDK_HOME/gcc).
    PATH="$SDK_HOME/bin:$QTDIR/bin:$FFMPEG_PATH/bin:$LIBRAW_PATH/bin:$PATH"
    LIBRARY_PATH="$SDK_HOME/lib:$QTDIR/lib:$FFMPEG_PATH/lib:$LIBRAW_PATH/lib"
    LD_LIBRARY_PATH="$SDK_HOME/lib:$QTDIR/lib:$FFMPEG_PATH/lib:$LIBRAW_PATH/lib"
    #LD_RUN_PATH="$SDK_HOME/lib:$QTDIR/lib:$FFMPEG_PATH/lib:$LIBRAW_PATH/lib"
    PKG_CONFIG_PATH="$SDK_HOME/lib/pkgconfig:$SDK_HOME/share/pkgconfig:$SDK_HOME/libdata/pkgconfig:$FFMPEG_PATH/lib/pkgconfig:$LIBRAW_PATH/lib/pkgconfig:$OSMESA_PATH/lib/pkgconfig:$PYTHON_HOME/lib/pkgconfig:$QTDIR/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
elif [ "$PKGOS" = "Windows" ]; then
    PKG_CONFIG_PATH="$FFMPEG_PATH/lib/pkgconfig:$LIBRAW_PATH/lib/pkgconfig:$OSMESA_PATH/lib/pkgconfig:$(cygpath -u ${PYTHON_HOME})/lib/pkgconfig:$QTDIR/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
elif [ "$PKGOS" = "OSX" ]; then
    PKG_CONFIG_PATH="$FFMPEG_PATH/lib/pkgconfig:$LIBRAW_PATH/lib/pkgconfig:$OSMESA_PATH/lib/pkgconfig:$PYTHON_HOME/lib/pkgconfig:$QTDIR/lib/pkgconfig${PKG_CONFIG_PATH:+:$PKG_CONFIG_PATH}"
fi
export LD_LIBRARY_PATH LD_RUN_PATH DYLD_LIBRARY_PATH LIBRARY_PATH CPATH PKG_CONFIG_PATH C_INCLUDE_PATH CPLUS_INCLUDE_PATH BOOST_ROOT

# Qt may not be installed yet, e.g. when building the SDK or generating its Dockerfile
if [ "${QT_VERSION_MAJOR}" = 4 ]; then
    QT_VERSION=$(pkg-config --modversion QtCore 2>/dev/null || true)
else
    QT_VERSION=$(pkg-config --modversion Qt${QT_VERSION_MAJOR}Core 2>/dev/null || true)
fi
QT_VERSION_MINOR=$(echo $QT_VERSION | cut -d. -f2)

# Load compiler related stuff
source $CWD/compiler-common.sh


# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:
