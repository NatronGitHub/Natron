#!/bin/bash
# ***** BEGIN LICENSE BLOCK *****
# This file is part of Natron <http://www.natron.fr/>,
# Copyright (C) 2013-2018 INRIA and Alexandre Gauthier
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
# Build Natron SDK (CY2015+) for Linux
#
# By default, running this script will rebuild all packages that need to be updated.
# Some dependencies are automatically handled, e.g. if package B depends on A, B
# will be rebuilt when A is rebuilt.
#
# Options available by setting the following environment variables:
#
# GEN_DOCKERFILE=1 : Output a Dockerfile rather than building the SDK.
# GEN_DOCKERFILE32=1 : Output a 32-bit Dockerfile rather than building the SDK.
# LIST_STEPS=1 : Output an ordered list of steps to build the SDK
# LAST_STEP=step_name : Only build until this step (be careful, as dependent packages are not rebuilt).
# FORCE_BUILD=1 : Force build, even if up-to-date. If LAST_STEP is specified, only force-build this step.
#
# Only available when GEN_DOCKERFILE, LIST_STEPS and LAST_STEPS are not set:
#
# TAR_SDK=1 : Make an archive of the SDK when building is done and store it in $SRC_PATH
# UPLOAD_SDK=1 : Upload the SDK tar archive to $REPO_DEST if TAR_SDK=1

set -e # Exit immediately if a command exits with a non-zero status
set -u # Treat unset variables as an error when substituting.
#set -x # Print commands and their arguments as they are executed.

if false; then
    # to update all linux SDKs, run the following
    alias ssh-natron-linux32='ssh -A -p 3232 natron@natron.inrialpes.fr' # centos32
    alias ssh-natron-linux64='ssh -A -p 6464 natron@natron.inrialpes.fr' # centos64
    alias ssh-natron-linux64-ci='ssh -A ci@natron-centos64-64.ci' # ci
    alias ssh-natron-linux32-ci='ssh -A ci@natron-centos64-32.ci' # ci
    ssh-natron-linux32 "cd natron-support-jenkins/buildmaster;git pull;include/scripts/build-Linux-sdk.sh"
    ssh-natron-linux64 "cd natron-support-jenkins/buildmaster;git pull;include/scripts/build-Linux-sdk.sh"
    ssh-natron-linux32-ci "cd data/natron-support/buildmaster;git pull;include/scripts/build-Linux-sdk.sh"
    ssh-natron-linux64-ci "cd data/natron-support/buildmaster;git pull;include/scripts/build-Linux-sdk.sh"
fi

scriptdir=`dirname "$0"`

if [ "${GEN_DOCKERFILE32:-}" = "1" -a -z "${GEN_DOCKERFILE+x}" ]; then
    GEN_DOCKERFILE=1
fi
if [ "${GEN_DOCKERFILE:-}" = "1" -o "${GEN_DOCKERFILE:-}" = "2" ]; then
    cat <<EOF
# Natron-SDK dockerfile.
#
# The natron-sdk docker image should be created using the following commands:
# cd tools/docker/natron-sdk/
# ./build.sh
# SDK is installed in /opt/Natron-sdk
EOF
    if [ "${GEN_DOCKERFILE32:-}" = "1" ]; then
        # - base on i386/centos:6
        # - set the yum archs
        # - install util-linux to get the "setarch" executable
        DOCKER_BASE="i386/centos:6"
        LINUX32="setarch i686"
        DOCKERFILE_I386='RUN echo "i686" > /etc/yum/vars/arch && echo "i386" > /etc/yum/vars/basearch'
    else
        DOCKER_BASE="centos:6"
        LINUX32=
        DOCKERFILE_I386=
    fi
    cat <<EOF
FROM $DOCKER_BASE as intermediate
MAINTAINER https://github.com/NatronGitHub/Natron
WORKDIR /home
$DOCKERFILE_I386
RUN yum install -y util-linux git gcc gcc-c++ make tar wget patch libX11-devel mesa-libGL-devel libXcursor-devel libXrender-devel libXrandr-devel libXinerama-devel libSM-devel libICE-devel libXi-devel libXv-devel libXfixes-devel libXvMC-devel libXxf86vm-devel libxkbfile-devel libXdamage-devel libXp-devel libXScrnSaver-devel libXcomposite-devel libXp-devel libXevie-devel libXres-devel xorg-x11-proto-devel libXxf86dga-devel libdmx-devel libXpm-devel
COPY include/patches/ include/patches/
COPY build-Linux-sdk.sh common.sh compiler-common.sh ./
EOF
    BUILD_LINUX_SDK=./build-Linux-sdk.sh
    ## older version:
    #RUN git clone https://github.com/NatronGitHub/Natron.git
    #WORKDIR /home/Natron/tools/jenkins
    #BUILD_LINUX_SDK=./include/scripts/build-Linux-sdk.sh
fi

function dobuild ()
{
    if [ "${GEN_DOCKERFILE:-}" = "1" -o "${GEN_DOCKERFILE:-}" = "2" ] || [ "${LIST_STEPS:-}" = "1" ]; then
        return 1
    fi
    return 0 # must return a status
}

function build()
{
    step="$1"
    if [ -f "${scriptdir}/pkg/${step}.sh" ]; then
        . "${scriptdir}/pkg/${step}.sh"
    else
        if [ "${GEN_DOCKERFILE:-}" = "1" -o "${GEN_DOCKERFILE:-}" = "2" ] || [ "${LIST_STEPS:-}" = "1" ] || [ "${step}" = "${LAST_STEP:-}" ]; then
            (>&2 echo "Error: package file ${scriptdir}/pkg/${step}.sh not available")
            exit 1
        fi
    fi
}

function checkpoint()
{
    if [ "${GEN_DOCKERFILE:-}" = "1" -o "${GEN_DOCKERFILE:-}" = "2" ]; then
        if [ "$step" = "$checkpointstep" ]; then
            # checkpoint was walled twice in a row
            return 0
        fi
        copyline="COPY "
        for s in in "${pkgs[@]}"; do
            if [ -f "$scriptdir/pkg/${s}.sh" ]; then
                copyline="$copyline pkg/${s}.sh "
            fi
            checkpointstep="$s"
        done
        echo "$copyline pkg/"
        echo "RUN $LINUX32 env LAST_STEP=$checkpointstep $BUILD_LINUX_SDK || (cd /opt/Natron-sdk/var/log/Natron-Linux-x86_64-SDK/ && cat "'`ls -t |grep -e '"'\.log$'"'|head -1`'" && false)"
        pkgs=()
    fi
    return 0
}

# should we build this step?
# Also prints output if GEN_DOCKERFILE=1 or LIST_STEPS=1
function build_step ()
{
    # no-build cases (we avoid printing the same step twice)
    if [ "${GEN_DOCKERFILE:-}" = "1" -o "${GEN_DOCKERFILE:-}" = "2" ]; then
        if [ "$step" != "$prevstep" ]; then
            # push to the list of packages
            pkgs+=("$step")
        fi
        prevstep="$step"
        if [ "${GEN_DOCKERFILE:-}" = "2" ]; then
            # special value of GEN_DOCKERFILE that prints all the steps
            checkpoint
        fi
        return 1 # do not build
    fi
    if [ "${LIST_STEPS:-}" = "1" ]; then
        if [ "$step" != "$prevstep" ]; then
            echo "$step"
        fi
        prevstep="$step"
        return 1
    fi
    # maybe-build cases
    if dobuild && [ -z ${LAST_STEP+x} ]; then # LAST_STEP is not set, so build
        prevstep="$step"
        return 0
    fi
    if dobuild && [ "$reachedstep" = "0" ] ; then # build this step
        if [ "${LAST_STEP:-}" = "$step" ]; then # this is the last step to build
            reachedstep=1
        fi
        prevstep="$step"
        return 0
    fi
    if dobuild && [ "$prevstep" = "$step" ]; then
        # build_step is called a second time on LAST_STEP, this is OK
        return 0
    fi
    prevstep="$step"
    return 1 # must return a status
}

# is build forced?
function force_build()
{
    # for build if:
    # - FORCE_BUILD is 1 and LAST_STEP is not set
    # or
    # - FORCE_BUILD is 1 and LAST_STEP is the current step
    if [ "${FORCE_BUILD:-}" = "1" ] && [ -z ${LAST_STEP+x} -o "${LAST_STEP:-}" = "$step" ]; then
        return 0
    fi
    return 1 # must return a status
}

if dobuild; then
    source common.sh

    if [ "${DEBUG:-}" = "1" ]; then
        CMAKE_BUILD_TYPE="Debug"
    else
        CMAKE_BUILD_TYPE="Release"
    fi

    error=false
    # Check that mandatory utilities are present
    for e in gcc g++ make wget tar patch find gzip; do
        if ! type -p "$e" > /dev/null; then
            (>&2 echo "Error: $e not available")
            error=true
        fi
    done

    if [ ! -f /usr/include/X11/Xlib.h ] && [ ! -f /usr/X11R6/include/X11/Xlib.h ]; then
        (>&2 echo "Error: X11/Xlib.h not available (on CentOS, do 'yum install libICE-devel libSM-devel libX11-devel libXScrnSaver-devel libXcomposite-devel libXcursor-devel libXdamage-devel libXevie-devel libXfixes-devel libXi-devel libXinerama-devel libXp-devel libXp-devel libXpm-devel libXrandr-devel libXrender-devel libXres-devel libXv-devel libXvMC-devel libXxf86dga-devel libXxf86vm-devel libdmx-devel libxkbfile-devel mesa-libGL-devel')")
        error=true
    fi

    if $error; then
        (>&2 echo "Error: build cannot proceed, exiting.")
        exit 1
    fi

    # Check distro and version. CentOS/RHEL 6.10 only!

    if [ "$PKGOS" = "Linux" ]; then
        if [ ! -s /etc/redhat-release ]; then
            (>&2 echo "Warning: Build system has been designed for CentOS/RHEL, use at OWN risk!")
            sleep 5
        else
            RHEL_MAJOR=$(cut -d" " -f3 < /etc/redhat-release | cut -d "." -f1)
            RHEL_MINOR=$(cut -d" " -f3 < /etc/redhat-release | cut -d "." -f2)
            if [ "$RHEL_MAJOR" != "6" ] || [ "$RHEL_MINOR" != "10" ]; then
                (>&2 echo "Warning: Wrong CentOS/RHEL version (${RHEL_MAJOR}.${RHEL_MINOR}): only CentOS 6.10 is officially supported.")
            fi
        fi
    fi


    BINARIES_URL="$REPO_DEST/Third_Party_Binaries"


    SDK="Linux-$ARCH-SDK"

    if [ -z "${MKJOBS:-}" ]; then
        #Default to 4 threads
        MKJOBS=$DEFAULT_MKJOBS
    fi
fi # dobuild

# see https://stackoverflow.com/a/24067243
function version_gt() { test "$(printf '%s\n' "$@" | sort -V | head -n 1)" != "$1"; }

function download() {
    if dobuild; then
        local master_site=$1
        local package=$2
        if [ ! -s "$SRC_PATH/$package" ]; then
            echo "*** downloading $package"
            wget "$THIRD_PARTY_SRC_URL/$package" -O "$SRC_PATH/$package" || \
                wget --no-check-certificate "$master_site/$package" -O "$SRC_PATH/$package" || \
                rm "$SRC_PATH/$package" || true
        fi
        if [ ! -s "$SRC_PATH/$package" ]; then
            (>&2 echo "Error: unable to download $package.")
            exit 1
        fi
    fi
}

function download_github() {
    if dobuild; then
        # e.g.:
        #download_github uclouvain openjpeg 2.3.0 v openjpeg-2.3.0.tar.gz
        #download_github OpenImageIO oiio 1.6.18 tags/Release- oiio-1.6.18.tar.gz
        local user=$1
        local repo=$2
        local version=$3
        local tagprefix=$4
        local package=$5
        if [ ! -s "$SRC_PATH/$package" ]; then
            echo "*** downloading $package"
            wget "$THIRD_PARTY_SRC_URL/$package" -O "$SRC_PATH/$package" || \
                wget --no-check-certificate --content-disposition "https://github.com/${user}/${repo}/archive/${tagprefix}${version}.tar.gz" -O "$SRC_PATH/$package" || \
                rm "$SRC_PATH/$package" || true
        fi
        if [ ! -s "$SRC_PATH/$package" ]; then
            (>&2 echo "Error: unable to download $package")
            exit 1
        fi
    fi
}

# see https://stackoverflow.com/a/43136160/2607517
# $1: git repo
# $2: commit SHA1
function git_clone_commit() {
    if dobuild; then
        local repo=${1##*/}
        repo=${repo%.git}
        git clone -n "$1" && ( cd "$repo" && git checkout "$2" )
        ## the following requires git 2.5.0 with uploadpack.allowReachableSHA1InWant=true
        #git init
        #git remote add origin "$1"
        #git fetch --depth 1 origin "$2"
        #git checkout FETCH_HEAD
    fi
}

function untar() {
    if dobuild; then
        echo "*** extracting $1"
        tar xf "$1"
    fi
}

function start_build() {
    if dobuild; then
        echo "*** Building $step..."
        touch "$LOGDIR/$step".stamp
        exec 3>&1 4>&2 1>"$LOGDIR/$step".log 2>&1
        pushd "$TMP_PATH"
    fi
}

function end_build() {
    if dobuild; then
        popd #"$TMP_PATH"
        find "$SDK_HOME" -newer "$LOGDIR/$step".stamp > "$LOGDIR/$step"-files.txt
        rm "$LOGDIR/$step".stamp
        exec 1>&3 2>&4
        echo "*** Done"
    fi
}

if dobuild; then
    echo 
    echo "Building Natron-$SDK with $MKJOBS threads ..."
    echo

    if [ -d "$TMP_PATH" ]; then
        rm -rf "$TMP_PATH"
    fi
    mkdir -p "$TMP_PATH"
    if [ ! -d "$SRC_PATH" ]; then
        mkdir -p "$SRC_PATH"
    fi

    # check for dirs
    if [ ! -d "$SDK_HOME/include" ]; then
        # see http://www.linuxfromscratch.org/lfs/view/development/chapter06/creatingdirs.html
        mkdir -pv "$SDK_HOME"/{bin,include,lib,sbin,src}
        mkdir -pv "$SDK_HOME"/share/{color,dict,doc,info,locale,man}
        mkdir -v  "$SDK_HOME"/share/{misc,terminfo,zoneinfo}
        mkdir -v  "$SDK_HOME"/libexec
        mkdir -pv "$SDK_HOME"/share/man/man{1..8}
        case $(uname -m) in
            x86_64) ln -sfv lib "$SDK_HOME"/lib64 ;;
        esac
    fi

    LOGDIR="$SDK_HOME/var/log/Natron-$SDK"
    if [ ! -d "$LOGDIR" ]; then
        mkdir -p "$LOGDIR"
    fi
    echo "*** Info: Build logs are in $LOGDIR"
fi

checkpointstep=""
prevstep=""
reachedstep=0 # becomes 1 when LAST_STEP is reached
pkgs=()

build openssl-installer

checkpoint

build qt-installer
build qtifw

checkpoint

# Setup env

if dobuild; then
    # now that the installer is built, let's set failsafe values for variables that are either
    # understood by configure/cmake scripts or by GCC itself
    export CMAKE_PREFIX_PATH="$SDK_HOME"
    #export CPPFLAGS="-I$SDK_HOME/include" # configure/cmake
    #export LDFLAGS="-L$SDK_HOME/lib" # configure/cmake
    export CPATH="$SDK_HOME/include" # gcc/g++'s include path
    export LIBRARY_PATH="$SDK_HOME/lib" # gcc/g++'s library path

    QT4PREFIX="$SDK_HOME/qt4"
    QT5PREFIX="$SDK_HOME/qt5"

    export PATH="$SDK_HOME/gcc/bin:$SDK_HOME/bin:$PATH"
    export LD_LIBRARY_PATH="$SDK_HOME/lib:$QT4PREFIX/lib:$QT5PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    if [ "$ARCH" = "x86_64" ]; then
        LD_LIBRARY_PATH="$SDK_HOME/gcc/lib64${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    else
        LD_LIBRARY_PATH="$SDK_HOME/gcc/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    fi
    #export LD_RUN_PATH="$LD_LIBRARY_PATH"
    #PKG_CONFIG_PATH="$SDK_HOME/lib/pkgconfig:$SDK_HOME/share/pkgconfig"
    BOOST_ROOT="$SDK_HOME"
    OPENJPEG_HOME="$SDK_HOME"
    THIRD_PARTY_TOOLS_HOME="$SDK_HOME"
    PYTHON_HOME="$SDK_HOME"
    PYTHON_PATH="$SDK_HOME/lib/python${PYVER}"
    PYTHON_INCLUDE="$SDK_HOME/include/python${PYVER}"
    export PKG_CONFIG_PATH LD_LIBRARY_PATH PATH BOOST_ROOT OPENJPEG_HOME THIRD_PARTY_TOOLS_HOME PYTHON_HOME PYTHON_PATH PYTHON_INCLUDE
fi

build gcc

checkpoint

if dobuild; then
    export CC="${SDK_HOME}/gcc/bin/gcc"
    export CXX="${SDK_HOME}/gcc/bin/g++"
fi

build bzip2
build xz # (required to uncompress source tarballs)
build m4
build ncurses # (required for gdb and readline)
build bison # (for SeExpr)
build flex # (for SeExpr)
build pkg-config
build libtool
build gperf # (used by fontconfig) as well as assemblers (yasm and nasm)
build autoconf
build automake
build yasm
build nasm # (for x264, lame, ans others)
build gmp # (used by ruby)
build openssl
build patchelf
build gettext
build expat
build perl # (required to install XML:Parser perl module used by intltool)
build intltool
build zlib

checkpoint

build readline
build nettle # (for gnutls)
build libtasn1 # (for gnutls)
build libunistring # (for gnutls)
build libffi
build p11-kit # (for gnutls)
build gnutls # (for ffmpeg)
build curl
build libarchive # (for cmake)
#build libuv # (for cmake but we use the one bundled with cmake)
build cmake
build libzip # (requires cmake)
#build berkeleydb # (optional for python2 and python3)
build sqlite # (required for webkit and QtSql SQLite module, optional for python2)
build pcre # (required by glib)

checkpoint

build git # (requires curl and pcre)
build mariadb # (for the Qt mariadb plugin and the python mariadb adapter)
build postgresql # (for the Qt postgresql plugin and the python postgresql adapter)

checkpoint

build python2

checkpoint

build python3

checkpoint

build ninja # (requires python3, required for building glib > 2.59.0)
build meson # (requires python3, required for building glib > 2.59.0)

checkpoint

build mysqlclient # (MySQL/MariaDB connector for python 2/3)
build psycopg2 # (PostgreSQL connector for python 2/3)

checkpoint

build icu # requires python

checkpoint

build osmesa

checkpoint

build libpng
build freetype
build libmount # (required by glib)
build fontconfig
build texinfo # (for gdb)

checkpoint

build libxml2
build libxslt # (required by glib)
build dbus # (for QtDBus and glib)
build glib
build boost

checkpoint

build cppunit
build libjpeg-turbo
build giflib
build tiff
build jasper
build lcms2
build librevenge
build libcdr
build openjpeg
build libraw
build ilmbase
build openexr
build pixman
build cairo
build harfbuzz
build fribidi # (for libass and ffmpeg)
build pango
build libcroco # (requires glib and libxml2)
build shared-mime-info # (required by gdk-pixbuf)
build gdk-pixbuf
build librsvg # (without vala support)
build fftw # (GPLv2, for openfx-gmic)
build imagemagick6
build imagemagick7
build glew
build poppler-glib # (without curl, nss3, qt4, qt5)
build poppler-data
build tinyxml # (used by OpenColorIO)
build yaml-cpp # (0.5.3 requires boost, 0.6+ requires C++11, used by OpenColorIO)
build opencolorio # (uses yaml-cpp, tinyxml, lcms)
build webp # (for OIIO)
build openimageio
build seexpr
#build eigen
#build opencv

checkpoint

build lame
build libogg
build libvorbis
build libtheora
build libmodplug
build libvpx
build speex # (EOL, use opus)
build opus
build orc
#build dirac # (obsolete since ffmpeg-3.4)
build x264
build x265
build xvid
build soxr
build libass # (for ffmpeg)
build libbluray # (for ffmpeg)
build openh264 # (for ffmpeg)
build snappy # (for ffmpeg)
build ffmpeg
build ruby # (necessary for qtwebkit)
build breakpad
build valgrind
build gdb # (requires xz, zlib ncurses, python2, texinfo)

checkpoint

# build qt5 # <--- Disabled Qt 5.6 (temporary)

# pysetup
if dobuild; then
    if [ "$PYV" = "3" ]; then
        export PYTHON_PATH=$SDK_HOME/lib/python${PYVER}
        export PYTHON_INCLUDE=$SDK_HOME/include/python${PYVER}
        PY_EXE=$SDK_HOME/bin/python${PYV}
        PY_LIB=$SDK_HOME/lib/libpython${PYVER}.so
        PY_INC=$SDK_HOME/include/python${PYVER}
        USE_PY3=true
    else
        PY_EXE=$SDK_HOME/bin/python${PYV}
        PY_LIB=$SDK_HOME/lib/libpython${PYVER}.so
        PY_INC=$SDK_HOME/include/python${PYVER}
        USE_PY3=false
    fi


    # add qt5 to lib path to build shiboken2 and pyside2
    LD_LIBRARY_PATH="$QT5PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    #LD_RUN_PATH="$LD_LIBRARY_PATH"
fi

# build shiboken2 # only useful for qt5
# build pyside2 # only useful for qt5

checkpoint

build qt4

checkpoint

build qt4webkit

if dobuild; then
    # add qt4 to lib path to build shiboken and pyside
    LD_LIBRARY_PATH="$QT4PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    #LD_RUN_PATH="$LD_LIBRARY_PATH"
fi

build shiboken
build pyside

if dobuild; then
    # make sure all libs are user-writable and executable
    for subdir in . ffmpeg-gpl2 ffmpeg-lgpl libraw-gpl2 libraw-lgpl qt4; do # removed qt5 (temporary) to prevent script error
        if compgen -G "$SDK_HOME/$subdir/lib*/lib*.so*" > /dev/null; then
            chmod 755 "$SDK_HOME"/$subdir/lib*/lib*.so*
        fi
    done

    if [ ! -z "${TAR_SDK:-}" ]; then
        # Done, make a tarball
        pushd "$SDK_HOME/.."
        tar cJf "$SRC_PATH/Natron-$SDK.tar.xz" "Natron-sdk"
        echo "*** SDK available at $SRC_PATH/Natron-$SDK.tar.xz"

        if [ ! -z "${UPLOAD_SDK:-}" ]; then
            rsync -avz -O --progress --verbose -e 'ssh -oBatchMode=yes' "$SRC_PATH/Natron-$SDK.tar.xz" "$BINARIES_URL"
        fi
        popd
    fi

    echo
    echo "Check for broken libraries and binaries... (everything is OK if nothing is printed)"
    # SDK_HOME=/opt/Natron-sdk; LD_LIBRARY_PATH=$SDK_HOME/qt4/lib:$SDK_HOME/gcc/lib:$SDK_HOME/lib:$SDK_HOME/qt5/lib
    for subdir in . ffmpeg-gpl2 ffmpeg-lgpl libraw-gpl2 libraw-lgpl qt4; do # removed qt5 (temporary) to prevent script error
        [ -d "$SDK_HOME/$subdir/lib" ] && LD_LIBRARY_PATH="$SDK_HOME/$subdir/lib:${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" find "$SDK_HOME/$subdir/lib" -name '*.so' -exec bash -c 'ldd {} 2>&1 | fgrep "not found" &>/dev/null' \; -print
        [ -d "$SDK_HOME/$subdir/bin" ] && LD_LIBRARY_PATH="$SDK_HOME/$subdir/lib:${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" find "$SDK_HOME/$subdir/bin" -maxdepth 1 -exec bash -c 'ldd {} 2>&1 | fgrep "not found" &>/dev/null' \; -print
    done
    echo "Check for broken libraries and binaries... done!"

    echo
    echo "Natron SDK Done"
    echo
fi

checkpoint

if [ "${GEN_DOCKERFILE:-}" = "1" -o "${GEN_DOCKERFILE:-}" = "2" ]; then
    cat <<EOF
RUN rm -rf /opt/Natron-sdk/var/log/Natron-Linux-x86_64-SDK
FROM $DOCKER_BASE
COPY --from=intermediate /opt/Natron-sdk /opt/Natron-sdk
#COPY --from=intermediate /home/src /opt/Natron-sdk/src
## retrieve sources using:
## docker run natrongithub/natron-sdk:latest tar -C /opt/Natron-sdk -cf - src | tar xvf -
EOF
    if [ "${GEN_DOCKERFILE32:-}" = "1" ]; then
        cat <<EOF
COPY --from=intermediate /usr/bin/setarch /usr/bin/setarch
ENTRYPOINT ["setarch", "i686"]
EOF
    fi
fi

exit 0

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:

