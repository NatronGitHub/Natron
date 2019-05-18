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
#### not yet implemented BEGIN
# GEN_DOCKERFILE=1 : Output a Dockerfile rather than building the SDK.
# LIST_STEPS=1 : Output an ordered list of steps to build the SDK
# BUILD_STEP=step_name : Only build this step (dependent packages are not rebuilt).
# FORCE_BUILD=1 : Force build, even if up-to-date.
# Only available when GEN_DOCKERFILE, LIST_STEPS and BUILD_STEPS are not set:
#### not yet implemented END
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

if [ "${GEN_DOCKERFILE:-}" = "1" ]; then
    cat <<EOF
# Natron-SDK dockerfile.
#
# The natron-sdk docker image should be created using the following commands:
# cd tools/docker/natron-sdk/
# env GEN_DOCKERFILE=1 ../../jenkins/include/scripts/build-Linux-sdk.sh > Dockerfile
# cp  ../../jenkins/include/scripts/build-Linux-sdk.sh ../../jenkins/common.sh ../../jenkins/compiler-common.sh .
# docker build -t natronsdk:latest .
# SDK is installed in /opt/Natron-sdk

FROM centos:6
MAINTAINER https://github.com/NatronGitHub/Natron
WORKDIR /home
RUN yum install -y git gcc gcc-c++ make tar wget patch libX11-devel mesa-libGL-devel libXcursor-devel libXrender-devel libXrandr-devel libXinerama-devel libSM-devel libICE-devel libXi-devel libXv-devel libXfixes-devel libXvMC-devel libXxf86vm-devel libxkbfile-devel libXdamage-devel libXp-devel libXScrnSaver-devel libXcomposite-devel libXp-devel libXevie-devel libXres-devel xorg-x11-proto-devel libXxf86dga-devel libdmx-devel libXpm-devel
RUN mkdir /home/Natron-sdk-build
WORKDIR /home/Natron-sdk-build
COPY include/patches/ /home/Natron-sdk-build/include/patches/
COPY build-Linux-sdk.sh /home/Natron-sdk-build/build-Linux-sdk.sh
COPY common.sh /home/Natron-sdk-build/common.sh
COPY compiler-common.sh /home/Natron-sdk-build/compiler-common.sh
EOF
    BUILD_LINUX_SDK=./build-Linux-sdk.sh
    ## older version:
    #RUN git clone https://github.com/NatronGitHub/Natron.git
    #WORKDIR /home/Natron/tools/jenkins
    #BUILD_LINUX_SDK=./include/scripts/build-Linux-sdk.sh
fi

function dobuild ()
{
    if [ "${GEN_DOCKERFILE:-}" = "1" ] || [ "${LIST_STEPS:-}" = "1" ]; then
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
        if [ "${GEN_DOCKERFILE:-}" = "1" ] || [ "${LIST_STEPS:-}" = "1" ] || [ "${step}" = "${BUILD_STEP:-}" ]; then
            (>&2 echo "Error: package file ${scriptdir}/pkg/${step}.sh not available")
            exit 1
        fi
    fi
}

# should we build this step?
# Also prints output if GEN_DOCKERFILE=1 or LIST_STEPS=1
function build_step ()
{
    # no-build cases (we avoid printing the same step twice)
    if [ "${GEN_DOCKERFILE:-}" = "1" ]; then
        if [ "$step" != "$prevstep" ]; then
            if [ -f "$scriptdir/pkg/${step}.sh" ]; then
                echo "COPY pkg/${step}.sh /home/Natron-sdk-build/pkg/${step}.sh"
            fi
            echo "RUN env BUILD_STEP=$step $BUILD_LINUX_SDK || (cd /opt/Natron-sdk/var/log/Natron-Linux-x86_64-SDK/ && cat "'`ls -t|head -1`'")"
        fi
        prevstep="$step"
        return 1
    fi
    if [ "${LIST_STEPS:-}" = "1" ]; then
        if [ "$step" != "$prevstep" ]; then
            echo "$step"
        fi
        prevstep="$step"
        return 1
    fi
    # maybe-build cases
    if dobuild && [ -z ${BUILD_STEP+x} ]; then # BUILD_STEP is not set, so build
        prevstep="$step"
        return 0
    fi
    if dobuild && [ "${BUILD_STEP:-}" = "$step" ]; then # this is the step to build
        prevstep="$step"
        return 0
    fi
    prevstep="$step"
    return 1 # must return a status
}

# is build forced?
function force_build()
{
    if [ "${FORCE_BUILD:-}" = "1" ]; then
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

    # Check distro and version. CentOS/RHEL 6.4 only!

    if [ "$PKGOS" = "Linux" ]; then
        if [ ! -s /etc/redhat-release ]; then
            (>&2 echo "Warning: Build system has been designed for CentOS/RHEL, use at OWN risk!")
            sleep 5
        else
            RHEL_MAJOR=$(cut -d" " -f3 < /etc/redhat-release | cut -d "." -f1)
            RHEL_MINOR=$(cut -d" " -f3 < /etc/redhat-release | cut -d "." -f2)
            if [ "$RHEL_MAJOR" != "6" ] || [ "$RHEL_MINOR" != "4" ]; then
                (>&2 echo "Warning: Wrong CentOS/RHEL version (${RHEL_MAJOR}.{$RHEL_MINOR}): only CentOS 6.4 is officially supported.")
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
            echo "*** Error: unable to download $package"
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
            echo "*** Error: unable to download $package"
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

prevstep=""

build openssl-installer
build qt-installer
build qtifw

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

if dobuild; then
    export CC="${SDK_HOME}/gcc/bin/gcc"
    export CXX="${SDK_HOME}/gcc/bin/g++"
fi

build bzip2
build xz
build m4
build ncurses
build bison
build flex
build pkg-config
build libtool
build gperf
build autoconf
build automake
build yasm
build nasm
build gmp
build openssl
build patchelf
build gettext
build expat
build perl
build intltool
build zlib
build readline
build nettle
build libtasn1
build libunistring
build libffi
build p11-kit
build gnutls
build curl
build libarchive
#build libuv
build cmake
build libzip
build icu
#build berkeleydb
build sqlite
build pcre
build git
build mariadb
build postgresql
build python2
build mysqlclient
build psycopg2
build python3
build osmesa
build libpng
build freetype
build libmount
build fontconfig
build texinfo
build glib
build libxml2
build libxslt
build boost
build cppunit
build libjpeg-turbo
build giflib
build tiff
build jasper
build lcms2
build librevenge
build libcdr
build openjpeg
build ilmbase
build openexr
build pixman
build cairo

# Install harbuzz
# see http://www.linuxfromscratch.org/blfs/view/svn/general/harfbuzz.html
HARFBUZZ_VERSION=1.8.6
HARFBUZZ_TAR="harfbuzz-${HARFBUZZ_VERSION}.tar.bz2"
HARFBUZZ_SITE="https://www.freedesktop.org/software/harfbuzz/release"
step="$HARFBUZZ_TAR"
if build_step && { force_build || { [ "${REBUILD_HARFBUZZ:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME/lib/libharfbuzz*" "$SDK_HOME/include/harfbuzz" "$SDK_HOME/lib/pkgconfig/"*harfbuzz* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/harfbuzz.pc" ] || [ "$(pkg-config --modversion harfbuzz)" != "$HARFBUZZ_VERSION" ]; }; }; then
    REBUILD_PANGO=1
    start_build
    download "$HARFBUZZ_SITE" "$HARFBUZZ_TAR"
    untar "$SRC_PATH/$HARFBUZZ_TAR"
    pushd "harfbuzz-${HARFBUZZ_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --disable-docs --disable-static --enable-shared --with-freetype --with-cairo --with-gobject --with-glib --with-icu
    make -j${MKJOBS}
    make install
    popd
    rm -rf "harfbuzz-${HARFBUZZ_VERSION}"
    end_build
fi

# Install fribidi (for libass and ffmpeg)
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/fribidi.html
FRIBIDI_VERSION=1.0.4
FRIBIDI_TAR="fribidi-${FRIBIDI_VERSION}.tar.bz2"
FRIBIDI_SITE="https://github.com/fribidi/fribidi/releases/download/v${FRIBIDI_VERSION}"
step="$FRIBIDI_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/fribidi.pc" ] || [ "$(pkg-config --modversion fribidi)" != "$FRIBIDI_VERSION" ]; }; }; then
    REBUILD_FFMPEG=1
    start_build
    download "$FRIBIDI_SITE" "$FRIBIDI_TAR"
    untar "$SRC_PATH/$FRIBIDI_TAR"
    pushd "fribidi-${FRIBIDI_VERSION}"
    # git.mk seems to trigger a ./config.status --recheck, which is unnecessary
    # and additionally fails due to quoting
    # (git.mk was removed after 0.19.7)
    if [ -f git.mk ]; then
        rm git.mk
    fi
    if [ ! -f configure ]; then
        autoreconf -i -f
    fi
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared --disable-docs
    make #  -j${MKJOBS}
    make install
    popd
    rm -rf "fribidi-${FRIBIDI_VERSION}"
    end_build
fi

# Install pango
# see http://www.linuxfromscratch.org/blfs/view/svn/x/pango.html
PANGO_VERSION=1.42.2
PANGO_VERSION_SHORT=${PANGO_VERSION%.*}
PANGO_TAR="pango-${PANGO_VERSION}.tar.xz"
PANGO_SITE="http://ftp.gnome.org/pub/GNOME/sources/pango/${PANGO_VERSION_SHORT}"
step="$PANGO_TAR"
if build_step && { force_build || { [ "${REBUILD_PANGO:-}" = "1" ]; }; }; then
    rm -rf $SDK_HOME/include/pango* $SDK_HOME/lib/libpango* $SDK_HOME/lib/pkgconfig/pango* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/pango.pc" ] || [ "$(pkg-config --modversion pango)" != "$PANGO_VERSION" ]; }; }; then
    REBUILD_SVG=1
    start_build
    download "$PANGO_SITE" "$PANGO_TAR"
    untar "$SRC_PATH/$PANGO_TAR"
    pushd "pango-${PANGO_VERSION}"
    env FONTCONFIG_LIBS="-lfontconfig" CFLAGS="$BF -g" CXXFLAGS="$BF -g" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --with-included-modules=basic-fc
    make -j${MKJOBS}
    make install
    popd
    rm -rf "pango-${PANGO_VERSION}"
    end_build
fi

# Install libcroco (requires glib and libxml2)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libcroco.html
LIBCROCO_VERSION=0.6.12
LIBCROCO_VERSION_SHORT=${LIBCROCO_VERSION%.*}
LIBCROCO_TAR="libcroco-${LIBCROCO_VERSION}.tar.xz"
LIBCROCO_SITE="http://ftp.gnome.org/pub/gnome/sources/libcroco/${LIBCROCO_VERSION_SHORT}"
step="$LIBCROCO_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libcroco-0.6.so" ]; }; }; then
    start_build
    download "$LIBCROCO_SITE" "$LIBCROCO_TAR"
    untar "$SRC_PATH/$LIBCROCO_TAR"
    pushd "libcroco-${LIBCROCO_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libcroco-${LIBCROCO_VERSION}"
    end_build
fi

# Install shared-mime-info (required by gdk-pixbuf)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/shared-mime-info.html
SHAREDMIMEINFO_VERSION=1.9
SHAREDMIMEINFO_TAR="shared-mime-info-${SHAREDMIMEINFO_VERSION}.tar.xz"
SHAREDMIMEINFO_SITE="http://freedesktop.org/~hadess"
step="$SHAREDMIMEINFO_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/share/pkgconfig/shared-mime-info.pc" ] || [ "$(pkg-config --modversion shared-mime-info)" != "$SHAREDMIMEINFO_VERSION" ]; }; }; then
    start_build
    download "$SHAREDMIMEINFO_SITE" "$SHAREDMIMEINFO_TAR"
    untar "$SRC_PATH/$SHAREDMIMEINFO_TAR"
    pushd "shared-mime-info-${SHAREDMIMEINFO_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --without-libtiff
    #make -j${MKJOBS}
    make
    make install
    popd
    rm -rf "shared-mime-info-${SHAREDMIMEINFO_VERSION}"
    end_build
fi

# Install gdk-pixbuf
# see http://www.linuxfromscratch.org/blfs/view/svn/x/gdk-pixbuf.html
GDKPIXBUF_VERSION=2.36.11
GDKPIXBUF_VERSION_SHORT=${GDKPIXBUF_VERSION%.*}
GDKPIXBUF_TAR="gdk-pixbuf-${GDKPIXBUF_VERSION}.tar.xz"
GDKPIXBUF_SITE="http://ftp.gnome.org/pub/gnome/sources/gdk-pixbuf/${GDKPIXBUF_VERSION_SHORT}"
step="$GDKPIXBUF_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/gdk-pixbuf-2.0.pc" ] || [ "$(pkg-config --modversion gdk-pixbuf-2.0)" != "$GDKPIXBUF_VERSION" ]; }; }; then
    start_build
    download "$GDKPIXBUF_SITE" "$GDKPIXBUF_TAR"
    untar "$SRC_PATH/$GDKPIXBUF_TAR"
    pushd "gdk-pixbuf-${GDKPIXBUF_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --without-libtiff
    make -j${MKJOBS}
    make install
    popd
    rm -rf "gdk-pixbuf-${GDKPIXBUF_VERSION}"
    end_build
fi

# Install librsvg (without vala support)
# see http://www.linuxfromscratch.org/blfs/view/systemd/general/librsvg.html
LIBRSVG_VERSION=2.40.20 # 2.41 requires rust
LIBRSVG_VERSION_SHORT=${LIBRSVG_VERSION%.*}
LIBRSVG_TAR="librsvg-${LIBRSVG_VERSION}.tar.xz"
LIBRSVG_SITE="https://download.gnome.org/sources/librsvg/${LIBRSVG_VERSION_SHORT}"
step="$LIBRSVG_TAR"
if build_step && { force_build || { [ "${REBUILD_SVG:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/include/librsvg* "$SDK_HOME"/lib/librsvg* "$SDK_HOME"/lib/pkgconfig/librsvg* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/librsvg-2.0.pc" ] || [ "$(pkg-config --modversion librsvg-2.0)" != "$LIBRSVG_VERSION" ]; }; }; then
    start_build
    if [ "$LIBRSVG_VERSION_SHORT" = 2.41 ]; then
        # librsvg 2.41 requires rust
        if [ ! -s "$HOME/.cargo/env" ]; then
            (>&2 echo "Error: librsvg requires rust. Please install rust by executing:")
            (>&2 echo "$SDK_HOME/bin/curl https://sh.rustup.rs -sSf | sh")
            exit 1
        fi
        source "$HOME/.cargo/env"
        if [ "$ARCH" = "x86_64" ]; then
            RUSTFLAGS="-C target-cpu=x86-64"
        else
            RUSTFLAGS="-C target-cpu=i686"
        fi
        export RUSTFLAGS
    fi
    pushd "$TMP_PATH"
    download "$LIBRSVG_SITE" "$LIBRSVG_TAR"
    untar "$SRC_PATH/$LIBRSVG_TAR"
    pushd "librsvg-${LIBRSVG_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --disable-introspection --disable-vala --disable-pixbuf-loader
    make -j${MKJOBS}
    make install
    popd
    rm -rf "librsvg-${LIBRSVG_VERSION}"
    end_build
fi

# Install fftw (GPLv2, for openfx-gmic)
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/fftw.html
FFTW_VERSION=3.3.8
FFTW_TAR="fftw-${FFTW_VERSION}.tar.gz"
FFTW_SITE="http://www.fftw.org/"
step="$FFTW_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/fftw3.pc" ] || [ "$(pkg-config --modversion fftw3)" != "$FFTW_VERSION" ]; }; }; then
    start_build
    download "$FFTW_SITE" "$FFTW_TAR"
    untar "$SRC_PATH/$FFTW_TAR"
    pushd "fftw-${FFTW_VERSION}"
    # to compile fftw3f, add --enable-float --enable-sse
    # --enable-avx512 is 64-bit only and requires a modern assembler (Support in binutils (gas/objdump) available from v2.24, CentOS 6.4 has 2.20)
    # /tmp/ccQxpb5e.s:25: Error: bad register name `%zmm14'
    #enableavx512="--enable-avx512"
    #if [ "$BITS" = "32" ]; then
        enableavx512=
    #fi
    # --enable-avx2 requires a modern assembler (Support in binutils (gas/objdump) available from v2.21, CentOS 6.4 has 2.20):
    # /tmp/ccX8Quod.s:86: Error: no such instruction: `vpermpd $177,%ymm0,%ymm0'
    env CFLAGS="$BF -DFFTWCORE_EXCLUDE_DEPRECATED=1" CXXFLAGS="$BF -DFFTWCORE_EXCLUDE_DEPRECATED=1" ./configure --prefix="$SDK_HOME" --enable-shared --enable-threads --enable-sse2 --enable-avx $enableavx512 -enable-avx-128-fma
    make -j${MKJOBS}
    make install
    popd
    rm -rf "fftw-${FFTW_VERSION}"
    end_build
fi


# Install ImageMagick6
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/imagemagick6.html
MAGICK_VERSION=6.9.10-9
MAGICK_VERSION_SHORT=${MAGICK_VERSION%-*}
MAGICK_TAR="ImageMagick6-${MAGICK_VERSION}.tar.gz"
MAGICK_SITE="https://gitlab.com/ImageMagick/ImageMagick6/-/archive/${MAGICK_VERSION}"
step="$MAGICK_TAR"
if build_step && { force_build || { [ "${REBUILD_MAGICK:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/include/ImageMagick-6/ "$SDK_HOME"/lib/libMagick* "$SDK_HOME"/share/ImageMagick-6/ "$SDK_HOME"/lib/pkgconfig/{Image,Magick}* "$SDK_HOME"/magick7 || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/Magick++.pc" ] || [ "$(pkg-config --modversion Magick++)" != "$MAGICK_VERSION_SHORT" ]; }; }; then
    start_build
    download "$MAGICK_SITE" "$MAGICK_TAR"
    untar "$SRC_PATH/$MAGICK_TAR"
    pushd "ImageMagick6-${MAGICK_VERSION}"
    #if [ "${MAGICK_CL:-}" = "1" ]; then
    #  MAGICK_CL_OPT="--with-x --enable-opencl"
    #else
    MAGICK_CL_OPT="--without-x"
    #fi
    patch -p0 -i "$INC_PATH/patches/ImageMagick/pango-align-hack.diff"
    env CFLAGS="$BF -DMAGICKCORE_EXCLUDE_DEPRECATED=1" CXXFLAGS="$BF -DMAGICKCORE_EXCLUDE_DEPRECATED=1" ./configure --prefix="$SDK_HOME" --with-magick-plus-plus=yes --with-quantum-depth=32 --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --with-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --with-pango --with-png --with-rsvg --without-tiff --without-webp --with-xml --with-zlib --without-bzlib --disable-static --enable-shared --enable-hdri --with-freetype --with-fontconfig --without-modules ${MAGICK_CL_OPT:-}
    make -j${MKJOBS}
    make install
    popd
    rm -rf "ImageMagick6-${MAGICK_VERSION}"
    end_build
fi
# install ImageMagick7
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/imagemagick.html
MAGICK7_VERSION=7.0.8-9
MAGICK7_VERSION_SHORT=${MAGICK7_VERSION%-*}
MAGICK7_TAR="ImageMagick-${MAGICK7_VERSION}.tar.gz"
MAGICK7_SITE="https://gitlab.com/ImageMagick/ImageMagick/-/archive/${MAGICK7_VERSION}"
step="$MAGICK7_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/magick7/lib/pkgconfig/Magick++.pc" ] || [ "$(env PKG_CONFIG_PATH=$SDK_HOME/magick7/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion Magick++)" != "$MAGICK7_VERSION_SHORT" ]; }; }; then
    start_build
    download "$MAGICK7_SITE" "$MAGICK7_TAR"
    untar "$SRC_PATH/$MAGICK7_TAR"
    pushd "ImageMagick-${MAGICK7_VERSION}"
    #if [ "${MAGICK_CL:-}" = "1" ]; then
    #  MAGICK_CL_OPT="--with-x --enable-opencl"
    #else
    MAGICK_CL_OPT="--without-x"
    #fi
    env CFLAGS="$BF -DMAGICKCORE_EXCLUDE_DEPRECATED=1" CXXFLAGS="$BF -DMAGICKCORE_EXCLUDE_DEPRECATED=1" ./configure --prefix=$SDK_HOME/magick7 --with-magick-plus-plus=yes --with-quantum-depth=32 --without-dps --without-djvu --without-fftw --without-fpx --without-gslib --without-gvc --without-jbig --without-jpeg --with-lcms --without-openjp2 --without-lqr --without-lzma --without-openexr --without-pango --with-png --without-rsvg --without-tiff --without-webp --without-xml --with-zlib --without-bzlib --disable-static --enable-shared --enable-hdri --with-freetype --with-fontconfig --without-modules ${MAGICK_CL_OPT:-}
    make -j${MKJOBS}
    make install
    popd
    rm -rf "ImageMagick-${MAGICK7_VERSION}"
    end_build
fi

# Install glew
GLEW_VERSION=2.1.0
GLEW_TAR="glew-${GLEW_VERSION}.tgz"
GLEW_SITE="https://sourceforge.net/projects/glew/files/glew/${GLEW_VERSION}"
step="$GLEW_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/glew.pc" ] || [ "$(pkg-config --modversion glew)" != "$GLEW_VERSION" ]; }; }; then
    start_build
    download "$GLEW_SITE" "$GLEW_TAR"
    untar "$SRC_PATH/$GLEW_TAR"
    pushd "glew-${GLEW_VERSION}"
    if [ "$ARCH" = "i686" ]; then
        make -j${MKJOBS} 'CFLAGS.EXTRA=-O2 -g -march=i686 -mtune=i686' includedir=/usr/include GLEW_DEST= libdir=/usr/lib bindir=/usr/bin
    else
        make -j${MKJOBS} 'CFLAGS.EXTRA=-O2 -g -m64 -fPIC -mtune=generic' includedir=/usr/include GLEW_DEST= libdir=/usr/lib64 bindir=/usr/bin
    fi
    make install GLEW_DEST="$SDK_HOME" libdir=/lib bindir=/bin includedir=/include
    chmod +x "$SDK_HOME/lib/libGLEW.so.$GLEW_VERSION"
    popd
    rm -rf "glew-${GLEW_VERSION}"
    end_build
fi

# Install poppler-glib (without curl, nss3, qt4, qt5)
# see http://www.linuxfromscratch.org/blfs/view/stable/general/poppler.html
POPPLER_VERSION=0.67.0
POPPLER_TAR="poppler-${POPPLER_VERSION}.tar.xz"
POPPLER_SITE="https://poppler.freedesktop.org"
step="$POPPLER_TAR"
if build_step && { force_build || { [ "${REBUILD_POPPLER:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME/include/poppler" "$SDK_HOME/lib/"libpoppler* "$SDK_HOME/lib/pkgconfig/"*poppler* || true 
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/poppler-glib.pc" ] || [ "$(pkg-config --modversion poppler-glib)" != "$POPPLER_VERSION" ]; }; }; then
    start_build
    download "$POPPLER_SITE" "$POPPLER_TAR"
    untar "$SRC_PATH/$POPPLER_TAR"
    pushd "poppler-${POPPLER_VERSION}"
    #env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --enable-cairo-output --enable-xpdf-headers --enable-libjpeg --enable-zlib --enable-poppler-glib --disable-poppler-qt4 --disable-poppler-qt5 --enable-build-type=release --enable-cmyk --enable-xpdf-headers
    #make -j${MKJOBS}
    #make install
    mkdir build
    pushd build
    # Natron-specific options: disable curl, nss3, qt5, qt4
    # ENABLE_NSS3=OFF does not disable NSS3 if include/library are found,
    # thus NSS3_FOUND=OFF
    natron_options=( -DENABLE_LIBCURL=OFF
                     -DNSS3_FOUND=OFF
                     -DENABLE_NSS3=OFF
                     -DENABLE_QT5=OFF
                     -DENABLE_QT4=OFF
                     -DENABLE_GTK_DOC=OFF )
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DENABLE_XPDF_HEADERS=ON "${natron_options[@]}"
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "poppler-${POPPLER_VERSION}"
    end_build
fi

# Install poppler-data
POPPLERDATA_VERSION=0.4.9
POPPLERDATA_TAR="poppler-data-${POPPLERDATA_VERSION}.tar.gz"
step="$POPPLERDATA_TAR"
if build_step && { force_build || { [ ! -d "$SDK_HOME/share/poppler" ]; }; }; then
    start_build
    download "$POPPLER_SITE" "$POPPLERDATA_TAR"
    untar "$SRC_PATH/$POPPLERDATA_TAR"
    pushd "poppler-data-${POPPLERDATA_VERSION}"
    make install datadir="${SDK_HOME}/share"
    popd
    rm -rf "poppler-data-${POPPLERDATA_VERSION}"
    end_build
fi

# Install tinyxml (used by OpenColorIO)
TINYXML_VERSION=2.6.2
TINYXML_TAR="tinyxml_${TINYXML_VERSION//./_}.tar.gz"
TINYXML_SITE="http://sourceforge.net/projects/tinyxml/files/tinyxml/${TINYXML_VERSION}"
step="$TINYXML_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/tinyxml.pc" ] || [ "$(pkg-config --modversion tinyxml)" != "$TINYXML_VERSION" ]; }; }; then
    start_build
    download "$TINYXML_SITE" "$TINYXML_TAR"
    untar "$SRC_PATH/$TINYXML_TAR"
    pushd "tinyxml"
    # The first two patches are taken from the debian packaging of tinyxml.
    #   The first patch enforces use of stl strings, rather than a custom string type.
    #   The second patch is a fix for incorrect encoding of elements with special characters
    #   originally posted at https://sourceforge.net/p/tinyxml/patches/51/
    # The third patch adds a CMakeLists.txt file to build a shared library and provide an install target
    #   submitted upstream as https://sourceforge.net/p/tinyxml/patches/66/
    patch -Np1 -i "$INC_PATH/patches/tinyxml/enforce-use-stl.patch"
    patch -Np1 -i "$INC_PATH/patches/tinyxml/entity-encoding.patch"
    patch -Np1 -i "$INC_PATH/patches/tinyxml/tinyxml_CMakeLists.patch"
    mkdir build
    pushd build
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    make install
    if [ ! -d "$SDK_HOME/lib/pkgconfig" ]; then
        mkdir -p "$SDK_HOME/lib/pkgconfig"
    fi
    cat > "$SDK_HOME/lib/pkgconfig/tinyxml.pc" <<EOF
prefix=${SDK_HOME}
exec_prefix=\${prefix}
libdir=\${exec_prefix}/lib
includedir=\${prefix}/include

Name: TinyXml
Description: Simple, small, C++ XML parser
Version: ${TINYXML_VERSION}
Libs: -L\${libdir} -ltinyxml
Cflags: -I\${includedir}
EOF
    popd
    popd
    rm -rf "tinyxml"
    end_build
fi

# Install yaml-cpp (0.5.3 requires boost, 0.6+ requires C++11, used by OpenColorIO)
YAMLCPP_VERSION=0.5.3
if [[ ! "$GCC_VERSION" =~ ^4\. ]]; then
    YAMLCPP_VERSION=0.6.2 # 0.6.0 is the first version to require C++11
fi
YAMLCPP_VERSION_SHORT=${YAMLCPP_VERSION%.*}
YAMLCPP_TAR="yaml-cpp-${YAMLCPP_VERSION}.tar.gz"
step="$YAMLCPP_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/yaml-cpp.pc" ] || [ "$(pkg-config --modversion yaml-cpp)" != "$YAMLCPP_VERSION" ]; }; }; then
    start_build
    download_github jbeder yaml-cpp "${YAMLCPP_VERSION}" yaml-cpp- "${YAMLCPP_TAR}"
    untar "$SRC_PATH/$YAMLCPP_TAR"
    pushd "yaml-cpp-yaml-cpp-${YAMLCPP_VERSION}"
    mkdir build
    pushd build
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "yaml-cpp-yaml-cpp-${YAMLCPP_VERSION}"
    end_build
fi

# Install OpenColorIO (uses yaml-cpp, tinyxml, lcms)
# We build a version more recent than 1.0.9, because 1.0.9 only supports yaml-cpp 0.3 and we
# installed yaml-cpp 0.5.
OCIO_BUILD_GIT=0 # set to 1 to build the git version instead of the release
# non-GIT version:
OCIO_VERSION=1.1.0
OCIO_TAR="OpenColorIO-${OCIO_VERSION}.tar.gz"
if [ "$OCIO_BUILD_GIT" = 1 ]; then
    ## GIT version:
    #OCIO_GIT="https://github.com/imageworks/OpenColorIO.git"
    ## 7bd4b1e556e6c98c0aa353d5ecdf711bb272c4fa is October 25, 2017
    #OCIO_COMMIT="7bd4b1e556e6c98c0aa353d5ecdf711bb272c4fa"
    step="OpenColorIO-$OCIO_COMMIT"
else
    step="$OCIO_TAR"
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/OpenColorIO.pc" ] || [ "$(pkg-config --modversion OpenColorIO)" != "$OCIO_VERSION" ]; }; }; then
    start_build
    if [ "$OCIO_BUILD_GIT" = 1 ]; then
        git_clone_commit "$OCIO_GIT" "$OCIO_COMMIT"
        pushd OpenColorIO
    else
        download_github imageworks OpenColorIO "$OCIO_VERSION" v "$OCIO_TAR"
        untar "$SRC_PATH/$OCIO_TAR"
        pushd "OpenColorIO-${OCIO_VERSION}"
    fi
    # don't treat warnings as error (this should be for OCIO developers only)
    find . -name CMakeLists.txt -exec sed -e s/-Werror// -i {} \;
    mkdir build
    pushd build
    cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"   -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DOCIO_BUILD_JNIGLUE=OFF -DOCIO_BUILD_NUKE=OFF -DOCIO_BUILD_SHARED=ON -DOCIO_BUILD_STATIC=OFF -DOCIO_STATIC_JNIGLUE=OFF -DOCIO_BUILD_TRUELIGHT=OFF -DUSE_EXTERNAL_LCMS=ON -DUSE_EXTERNAL_TINYXML=ON -DUSE_EXTERNAL_YAML=ON -DOCIO_BUILD_APPS=OFF -DOCIO_USE_BOOST_PTR=ON
    make -j${MKJOBS}
    make install
    popd
    popd
    if [ "$OCIO_BUILD_GIT" = 1 ]; then
        rm -rf OpenColorIO
    else
        rm -rf "OpenColorIO-${OCIO_VERSION}"
    fi
    end_build
fi

# Install webp (for OIIO)
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/libwebp.html
WEBP_VERSION=1.0.0
WEBP_TAR="libwebp-${WEBP_VERSION}.tar.gz"
WEBP_SITE="https://storage.googleapis.com/downloads.webmproject.org/releases/webp"
step="$WEBP_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libwebp.pc" ] || [ "$(pkg-config --modversion libwebp)" != "$WEBP_VERSION" ]; }; }; then
    start_build
    download "$WEBP_SITE" "$WEBP_TAR"
    untar "$SRC_PATH/$WEBP_TAR"
    pushd "libwebp-${WEBP_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --enable-libwebpmux --enable-libwebpdemux --enable-libwebpdecoder --enable-libwebpextras --disable-silent-rules --enable-swap-16bit-csp
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libwebp-${WEBP_VERSION}"
    end_build
fi

# Install oiio
#OIIO_VERSION=1.7.19
OIIO_VERSION=1.8.13
OIIO_VERSION_SHORT=${OIIO_VERSION%.*}
OIIO_TAR="oiio-Release-${OIIO_VERSION}.tar.gz"
step="openimageio-${OIIO_VERSION}"
if build_step && { force_build || { [ "${REBUILD_OIIO:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME/lib/libOpenImage"* "$SDK_HOME/include/OpenImage"* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libOpenImageIO.so" ]; }; }; then
    start_build
    download_github OpenImageIO oiio "$OIIO_VERSION" Release- "$OIIO_TAR"
    untar "$SRC_PATH/$OIIO_TAR"
    pushd "oiio-Release-${OIIO_VERSION}"

    patches=$(find  "$INC_PATH/patches/OpenImageIO/$OIIO_VERSION_SHORT" -type f)
    for p in $patches; do
        if [[ "$p" = *-mingw-* ]] && [ "$OS" != "MINGW64_NT-6.1" ]; then
            continue
        fi
        patch -p1 -i "$p"
    done

    if [ "${DEBUG:-}" = "1" ]; then
        OIIOMAKE="debug"
    fi
    OIIO_CMAKE_FLAGS=( "-DCMAKE_BUILD_TYPE=$CMAKE_BUILD_TYPE" "-DCMAKE_C_FLAGS=$BF" "-DCMAKE_CXX_FLAGS=$BF" "-DCMAKE_LIBRARY_PATH=${SDK_HOME}/lib" "-DUSE_OPENCV:BOOL=FALSE" "-DUSE_OPENSSL:BOOL=FALSE" "-DOPENEXR_HOME=$SDK_HOME" "-DILMBASE_HOME=$SDK_HOME" "-DTHIRD_PARTY_TOOLS_HOME=$SDK_HOME" "-DUSE_QT:BOOL=FALSE" "-DUSE_PYTHON:BOOL=FALSE" "-DUSE_FIELD3D:BOOL=FALSE" "-DOIIO_BUILD_TESTS=0" "-DOIIO_BUILD_TOOLS=1" "-DBOOST_ROOT=$SDK_HOME" "-DSTOP_ON_WARNING:BOOL=FALSE" "-DUSE_GIF:BOOL=TRUE" "-DUSE_FREETYPE:BOOL=TRUE" "-DFREETYPE_INCLUDE_PATH=$SDK_HOME/include" "-DUSE_FFMPEG:BOOL=FALSE" "-DLINKSTATIC=0" "-DBUILDSTATIC=0" "-DOPENJPEG_HOME=$SDK_HOME" "-DOPENJPEG_INCLUDE_DIR=$SDK_HOME/include/openjpeg-$OPENJPEG2_VERSION_SHORT" "-DUSE_OPENJPEG:BOOL=TRUE" )
    if [[ ! "$GCC_VERSION" =~ ^4\. ]]; then
        OIIO_CMAKE_FLAGS+=( "-DUSE_CPP11:BOOL=FALSE" "-DUSE_CPP14:BOOL=FALSE" "-DUSE_CPP17:BOOL=FALSE" )
    fi
    OIIO_CMAKE_FLAGS+=( "-DUSE_LIBRAW:BOOL=TRUE" "-DCMAKE_INSTALL_PREFIX=$SDK_HOME" )

    mkdir build
    pushd build

    cmake "${OIIO_CMAKE_FLAGS[@]}" ..
    make -j${MKJOBS} ${OIIOMAKE:-}
    make install
    popd
    popd
    rm -rf "oiio-Release-${OIIO_VERSION}"
    end_build
fi

# Install SeExpr
SEEXPR_VERSION=2.11
SEEXPR_TAR="SeExpr-${SEEXPR_VERSION}.tar.gz"
step="$SEEXPR_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libSeExpr.so" ]; }; }; then
    start_build
    download_github wdas SeExpr "$SEEXPR_VERSION" v "$SEEXPR_TAR"
    untar "$SRC_PATH/$SEEXPR_TAR"
    pushd "SeExpr-${SEEXPR_VERSION}"
    patch -Np1 -i "$INC_PATH/patches/SeExpr/0001-seexpr-2.11-fix-cxxflags.patch"
    patch -Np1 -i "$INC_PATH/patches/SeExpr/0002-seexpr-2.11-fix-inline-error.patch"
    patch -Np1 -i "$INC_PATH/patches/SeExpr/0003-seexpr-2.11-fix-dll-installation.patch"
    patch -Np1 -i "$INC_PATH/patches/SeExpr/0004-seexpr-2.11-c++98.patch"
    patch -Np1 -i "$INC_PATH/patches/SeExpr/0005-seexpr-2.11-noeditordemos.patch"
    #patch -p0 -i "$INC_PATH/patches/SeExpr/seexpr2-cmake.diff"
    mkdir build
    pushd build
    cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    make
    make install
    #rm -f $SDK_HOME/lib/libSeExpr.so
    popd
    popd
    rm -rf "SeExpr-${SEEXPR_VERSION}"
    end_build
fi

# Install eigen
#EIGEN_TAR=eigen-eigen-bdd17ee3b1b3.tar.gz
#if [ ! -s $SDK_HOME/lib/pkgconfig/eigen3.pc ]; then
#    pushd "$TMP_PATH"
#    if [ ! -s $CWD/src/$EIGEN_TAR ]; then
#        $WGET $THIRD_PARTY_SRC_URL/$EIGEN_TAR -O $CWD/src/$EIGEN_TAR
#    fi
#    untar $CWD/src/$EIGEN_TAR
#    cd eigen-*
#    rm -rf build
#    mkdir build
#    cd build
#    env CFLAGS="$BF" CXXFLAGS="$BF" cmake .. -DCMAKE_INSTALL_PREFIX=$SDK_HOME -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
#    make -j${MKJOBS}
#    make install
#    mv $SDK_HOME/share/pkgconfig/* $SDK_HOME/lib/pkgconfig
#fi

# Install opencv
#CV_TAR=opencv-2.4.11.zip
#if [ ! -s $SDK_HOME/lib/pkgconfig/opencv.pc ]; then
#    pushd "$TMP_PATH"
#    if [ ! -s $CWD/src/$CV_TAR ]; then
#        $WGET $THIRD_PARTY_SRC_URL/$CV_TAR -O $CWD/src/$CV_TAR
#    fi
#    unzip $CWD/src/$CV_TAR
#    cd opencv*
#    mkdir build
#    cd build
#    env CFLAGS="$BF" CXXFLAGS="$BF" CMAKE_INCLUDE_PATH=$SDK_HOME/include CMAKE_LIBRARY_PATH=$SDK_HOME/lib cmake .. -DWITH_GTK=OFF -DWITH_GSTREAMER=OFF -DWITH_FFMPEG=OFF -DWITH_OPENEXR=OFF -DWITH_OPENCL=OFF -DWITH_OPENGL=ON -DBUILD_WITH_DEBUG_INFO=OFF -DBUILD_TESTS=OFF -DBUILD_PERF_TESTS=OFF -DBUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DENABLE_SSE3=OFF -DCMAKE_INSTALL_PREFIX=$SDK_HOME -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
#    make -j${MKJOBS}
#    make install
#    mkdir -p $SDK_HOME/docs/opencv
#    cp ../LIC* ../COP* ../README ../AUTH* ../CONT* $SDK_HOME/docs/opencv/
#fi


# Install lame
# see http://www.linuxfromscratch.org/blfs/view/cvs/multimedia/lame.html
LAME_VERSION=3.100
#LAME_VERSION_SHORT=${LAME_VERSION%.*}
LAME_TAR="lame-${LAME_VERSION}.tar.gz"
LAME_SITE="https://sourceforge.net/projects/lame/files/lame/${LAME_VERSION}"
step="$LAME_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libmp3lame.so" ] || [ "$("$SDK_HOME/bin/lame" --version |head -1 | sed -e 's/^.*[Vv]ersion \([^ ]*\) .*$/\1/')" != "$LAME_VERSION" ]; }; }; then
    start_build
    download "$LAME_SITE" "$LAME_TAR"
    untar "$SRC_PATH/$LAME_TAR"
    pushd "lame-${LAME_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-mp3rtp --enable-shared --disable-static --enable-nasm
    make -j${MKJOBS}
    make install
    popd
    rm -rf "lame-${LAME_VERSION}"
    end_build
fi

# Install libogg
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/libogg.html
LIBOGG_VERSION=1.3.3
LIBOGG_TAR="libogg-${LIBOGG_VERSION}.tar.gz"
LIBOGG_SITE="http://downloads.xiph.org/releases/ogg"
step="$LIBOGG_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/ogg.pc" ] || [ "$(pkg-config --modversion ogg)" != "$LIBOGG_VERSION" ]; }; }; then
    start_build
    download "$LIBOGG_SITE" "$LIBOGG_TAR"
    untar "$SRC_PATH/$LIBOGG_TAR"
    pushd "libogg-${LIBOGG_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libogg-${LIBOGG_VERSION}"
    end_build
fi

# Install libvorbis
# http://www.linuxfromscratch.org/blfs/view/cvs/multimedia/libvorbis.html
LIBVORBIS_VERSION=1.3.6
LIBVORBIS_TAR="libvorbis-${LIBVORBIS_VERSION}.tar.gz"
LIBVORBIS_SITE="http://downloads.xiph.org/releases/vorbis"
step="$LIBVORBIS_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/vorbis.pc" ] || [ "$(pkg-config --modversion vorbis)" != "$LIBVORBIS_VERSION" ]; }; }; then
    start_build
    download "$LIBVORBIS_SITE" "$LIBVORBIS_TAR"
    untar "$SRC_PATH/$LIBVORBIS_TAR"
    pushd "libvorbis-${LIBVORBIS_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libvorbis-${LIBVORBIS_VERSION}"
    end_build
fi

# Install libtheora
# see http://www.linuxfromscratch.org/blfs/view/cvs/multimedia/libtheora.html
LIBTHEORA_VERSION=1.1.1
LIBTHEORA_TAR="libtheora-${LIBTHEORA_VERSION}.tar.bz2"
LIBTHEORA_SITE="http://downloads.xiph.org/releases/theora"
step="$LIBTHEORA_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/theora.pc" ] || [ "$(pkg-config --modversion theora)" != "$LIBTHEORA_VERSION" ]; }; }; then
    start_build
    download "$LIBTHEORA_SITE" "$LIBTHEORA_TAR"
    untar "$SRC_PATH/$LIBTHEORA_TAR"
    pushd "libtheora-${LIBTHEORA_VERSION}"
    $GSED -i 's/png_\(sizeof\)/\1/g' examples/png2theora.c # fix libpng16
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libtheora-${LIBTHEORA_VERSION}"
    end_build
fi

# Install libmodplug
LIBMODPLUG_VERSION=0.8.9.0
LIBMODPLUG_TAR="libmodplug-${LIBMODPLUG_VERSION}.tar.gz"
LIBMODPLUG_SITE="https://sourceforge.net/projects/modplug-xmms/files/libmodplug/${LIBMODPLUG_VERSION}"
step="$LIBMODPLUG_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libmodplug.so" ]; }; }; then
    start_build
    download "$LIBMODPLUG_SITE" "$LIBMODPLUG_TAR"
    untar "$SRC_PATH/$LIBMODPLUG_TAR"
    pushd "libmodplug-$LIBMODPLUG_VERSION"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libmodplug-$LIBMODPLUG_VERSION"
    end_build
fi

# Install libvpx
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/libvpx.html
LIBVPX_VERSION=1.7.0
LIBVPX_TAR="libvpx-${LIBVPX_VERSION}.tar.gz"
#LIBVPX_SITE=http://storage.googleapis.com/downloads.webmproject.org/releases/webm
step="$LIBVPX_TAR"
if build_step && { force_build || { [ "${REBUILD_LIBVPX:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/lib/libvpx* || true
    rm -rf "$SDK_HOME"/lib/pkgconfig/vpx.pc || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/vpx.pc" ] || [ "$(pkg-config --modversion vpx)" != "$LIBVPX_VERSION" ]; }; }; then
    start_build
    #download "$LIBVPX_SITE" "$LIBVPX_TAR"
    download_github webmproject libvpx "${LIBVPX_VERSION}" v "${LIBVPX_TAR}"
    untar "$SRC_PATH/$LIBVPX_TAR"
    pushd "libvpx-$LIBVPX_VERSION"
    # This command corrects ownership and permissions of installed files. 
    sed -i 's/cp -p/cp/' build/make/Makefile
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static --enable-vp8 --enable-vp9 --enable-vp9-highbitdepth --enable-multithread --enable-runtime-cpu-detect --enable-postproc --enable-pic --disable-avx --disable-avx2 --disable-examples
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libvpx-$LIBVPX_VERSION"
    end_build
fi

# Install speex (EOL, use opus)
# see http://www.linuxfromscratch.org/blfs/view/stable/multimedia/speex.html
SPEEX_VERSION=1.2.0
SPEEX_TAR="speex-${SPEEX_VERSION}.tar.gz"
SPEEX_SITE="http://downloads.us.xiph.org/releases/speex"
step="$SPEEX_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/speex.pc" ] || [ "$(pkg-config --modversion speex)" != "$SPEEX_VERSION" ]; }; }; then
    start_build
    download "$SPEEX_SITE" "$SPEEX_TAR"
    untar "$SRC_PATH/$SPEEX_TAR"
    pushd "speex-$SPEEX_VERSION"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "speex-$SPEEX_VERSION"
    end_build
fi

# Install opus
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/opus.html
OPUS_VERSION=1.2.1
OPUS_TAR="opus-${OPUS_VERSION}.tar.gz"
OPUS_SITE="https://archive.mozilla.org/pub/opus"
step="$OPUS_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/opus.pc" ] || [ "$(pkg-config --modversion opus)" != "$OPUS_VERSION" ]; }; }; then
    start_build
    download "$OPUS_SITE" "$OPUS_TAR"
    untar "$SRC_PATH/$OPUS_TAR"
    pushd "opus-$OPUS_VERSION"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static --enable-custom-modes
    make -j${MKJOBS}
    make install
    popd
    rm -rf "opus-$OPUS_VERSION"
    end_build
fi

# Install orc
ORC_VERSION=0.4.28
ORC_TAR="orc-${ORC_VERSION}.tar.xz"
ORC_SITE="http://gstreamer.freedesktop.org/src/orc"
step="$ORC_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/orc-0.4.pc" ] || [ "$(pkg-config --modversion orc-0.4)" != "$ORC_VERSION" ]; }; }; then
    start_build
    download "$ORC_SITE" "$ORC_TAR"
    untar "$SRC_PATH/$ORC_TAR"
    pushd "orc-$ORC_VERSION"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "orc-$ORC_VERSION"
    end_build
fi

# Install dirac (obsolete since ffmpeg-3.4)
if false; then
DIRAC_VERSION=1.0.11
DIRAC_TAR=schroedinger-${DIRAC_VERSION}.tar.gz
DIRAC_SITE="http://diracvideo.org/download/schroedinger"
step="$DIRAC_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/schroedinger-1.0.pc" ] || [ "$(pkg-config --modversion schroedinger-1.0)" != "$DIRAC_VERSION" ]; }; }; then
    start_build
    download "$DIRAC_SITE" "$DIRAC_TAR"
    untar "$SRC_PATH/$DIRAC_TAR"
    pushd "schroedinger-${DIRAC_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    #rm -f $SDK_HOME/lib/libschro*.so*
    $GSED -i "s/-lschroedinger-1.0/-lschroedinger-1.0 -lorc-0.4/" $SDK_HOME/lib/pkgconfig/schroedinger-1.0.pc
    popd
    rm -rf "schroedinger-${DIRAC_VERSION}"
    end_build
fi
fi

# x264
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/x264.html
X264_VERSION=20180212-2245-stable
X264_VERSION_PKG=0.152.x
X264_TAR="x264-snapshot-${X264_VERSION}.tar.bz2"
X264_SITE="https://download.videolan.org/x264/snapshots"
step="$X264_TAR"
if build_step && { force_build || { [ "${REBUILD_X264:-}" = "1" ]; }; }; then
    REBUILD_FFMPEG=1
    rm -rf "$SDK_HOME"/lib/libx264* || true
    rm -rf "$SDK_HOME"/lib/pkgconfig/x264* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/x264.pc" ] || [ "$(pkg-config --modversion x264)" != "$X264_VERSION_PKG" ]; }; }; then
    start_build
    download "$X264_SITE" "$X264_TAR"
    untar "$SRC_PATH/$X264_TAR"
    pushd "x264-snapshot-${X264_VERSION}"
    # add --bit-depth=10 to build a x264 that can only produce 10bit videos
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static --enable-pic --disable-cli
    make -j${MKJOBS}
    make install
    popd
    rm -rf "x264-snapshot-${X264_VERSION}"
    end_build
fi

# x265
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/x265.html
# and https://git.archlinux.org/svntogit/packages.git/tree/trunk/PKGBUILD?h=packages/x265
X265_VERSION=2.8
X265_TAR="x265_${X265_VERSION}.tar.gz"
X265_SITE="https://bitbucket.org/multicoreware/x265/downloads"
step="$X265_TAR"
if build_step && { force_build || { [ "${REBUILD_X265:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/lib/libx265* || true
    rm -rf "$SDK_HOME"/lib/pkgconfig/x265* || true
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/x265.pc" ] || [ "$(pkg-config --modversion x265)" != "$X265_VERSION" ]; }; }; then
    start_build
    download "$X265_SITE" "$X265_TAR"
    untar "$SRC_PATH/$X265_TAR"
    pushd "x265_$X265_VERSION"
    for d in 8 $([[ "$ARCH" == "x86_64" ]] && echo "10 12"); do
        if [[ -d build-$d ]]; then
            rm -rf build-$d
        fi
        mkdir build-$d
    done
    if [[ "$ARCH" == "x86_64" ]]; then
        pushd build-12
        cmake ../source \
              -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
              -DHIGH_BIT_DEPTH='TRUE' \
              -DMAIN12='TRUE' \
              -DEXPORT_C_API='FALSE' \
              -DENABLE_CLI='FALSE' \
              -DENABLE_SHARED='FALSE'
        make -j${MKJOBS}
        popd
        
        pushd build-10
        cmake ../source \
              -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
              -DHIGH_BIT_DEPTH='TRUE' \
              -DEXPORT_C_API='FALSE' \
              -DENABLE_CLI='FALSE' \
              -DENABLE_SHARED='FALSE'
        make -j${MKJOBS}
        popd
        
        pushd build-8

        ln -sfv ../build-10/libx265.a libx265_main10.a
        ln -sfv ../build-12/libx265.a libx265_main12.a

        cmake ../source \
              -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
              -DENABLE_SHARED='TRUE' \
              -DENABLE_HDR10_PLUS='TRUE' \
              -DEXTRA_LIB='x265_main10.a;x265_main12.a' \
              -DEXTRA_LINK_FLAGS='-L.' \
              -DLINKED_10BIT='TRUE' \
              -DLINKED_12BIT='TRUE'
        make -j${MKJOBS}
        # rename the 8bit library, then combine all three into libx265.a
        mv libx265.a libx265_main.a
        # On Linux, we use GNU ar to combine the static libraries together
        ar -M <<EOF
CREATE libx265.a
ADDLIB libx265_main.a
ADDLIB libx265_main10.a
ADDLIB libx265_main12.a
SAVE
END
EOF
        # Mac/BSD libtool
        #libtool -static -o libx265.a libx265_main.a libx265_main10.a libx265_main12.a 2>/dev/null
        popd
    else
        pushd build-8
        cmake ../source \
              -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
              -DENABLE_SHARED='TRUE'
        make -j${MKJOBS}
        popd
    fi
    pushd build-8
    make install
    popd
    popd
    rm -rf "x265_$X265_VERSION"
    end_build
fi

# xvid
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/xvid.html
XVID_VERSION=1.3.5
XVID_TAR="xvidcore-${XVID_VERSION}.tar.gz"
XVID_SITE="http://downloads.xvid.org/downloads"
step="$XVID_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libxvidcore.so" ]; }; }; then
    start_build
    download "$XVID_SITE" "$XVID_TAR"
    untar "$SRC_PATH/$XVID_TAR"
    pushd xvidcore/build/generic
    # Fix error during make install if reintalling or upgrading. 
    sed -i 's/^LN_S=@LN_S@/& -f -v/' platform.inc.in
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    # This command disables installing the static library. 
    sed -i '/libdir.*STATIC_LIB/ s/^/#/' Makefile
    make install
    chmod +x "$SDK_HOME/lib/libxvidcore.so".*.*
    popd
    rm -rf xvidcore
    end_build
fi

# install soxr
SOXR_VERSION=0.1.2
SOXR_TAR="soxr-${SOXR_VERSION}-Source.tar.xz"
SOXR_SITE="https://sourceforge.net/projects/soxr/files"
step="$SOXR_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/soxr.pc" ] || [ "$(pkg-config --modversion soxr)" != "$SOXR_VERSION" ]; }; }; then
    start_build
    download "$SOXR_SITE" "$SOXR_TAR"
    untar "$SRC_PATH/$SOXR_TAR"
    pushd "soxr-${SOXR_VERSION}-Source"
    mkdir tmp_build
    pushd tmp_build
    cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DBUILD_TESTS=no -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "soxr-${SOXR_VERSION}-Source"
    end_build
fi

# Install libass (for ffmpeg)
# see http://www.linuxfromscratch.org/blfs/view/cvs/multimedia/libass.html
LIBASS_VERSION=0.14.0
LIBASS_TAR="libass-${LIBASS_VERSION}.tar.xz"
LIBASS_SITE="https://github.com/libass/libass/releases/download/${LIBASS_VERSION}"
step="$LIBASS_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libass.pc" ] || [ "$(pkg-config --modversion libass)" != "$LIBASS_VERSION" ]; }; }; then
    REBUILD_FFMPEG=1
    start_build
    download "$LIBASS_SITE" "$LIBASS_TAR"
    untar "$SRC_PATH/$LIBASS_TAR"
    pushd "libass-${LIBASS_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libass-${LIBASS_VERSION}"
    end_build
fi

# Install libbluray (for ffmpeg)
# BD-Java support is disabled, because it requires apache-ant and a JDK for building
LIBBLURAY_VERSION=1.0.2
LIBBLURAY_TAR="libbluray-${LIBBLURAY_VERSION}.tar.bz2"
LIBBLURAY_SITE="ftp://ftp.videolan.org/pub/videolan/libbluray/${LIBBLURAY_VERSION}"
step="$LIBBLURAY_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/libbluray.pc" ] || [ "$(pkg-config --modversion libbluray)" != "$LIBBLURAY_VERSION" ]; }; }; then
    REBUILD_FFMPEG=1
    start_build
    download "$LIBBLURAY_SITE" "$LIBBLURAY_TAR"
    untar "$SRC_PATH/$LIBBLURAY_TAR"
    pushd "libbluray-${LIBBLURAY_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared --disable-bdjava --disable-bdjava-jar
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libbluray-${LIBBLURAY_VERSION}"
    end_build
fi

# Install openh264 (for ffmpeg)
OPENH264_VERSION=1.7.0
OPENH264_TAR="openh264-${OPENH264_VERSION}.tar.gz"
#OPENH264_SITE="https://github.com/cisco/openh264/archive"
step="$OPENH264_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/openh264.pc" ] || [ "$(pkg-config --modversion openh264)" != "$OPENH264_VERSION" ]; }; }; then
    REBUILD_FFMPEG=1
    start_build
    download_github cisco openh264 "${OPENH264_VERSION}" v "${OPENH264_TAR}"
    #download "$OPENH264_SITE" "$OPENH264_TAR"
    untar "$SRC_PATH/$OPENH264_TAR"
    pushd "openh264-${OPENH264_VERSION}"
    # AVX2 is only supported from Kaby Lake on
    # see https://en.wikipedia.org/wiki/Intel_Core
    make HAVE_AVX2=No PREFIX="$SDK_HOME" install
    popd
    rm -rf "openh264-${OPENH264_VERSION}"
    end_build
fi

# Install snappy (for ffmpeg)
SNAPPY_VERSION=1.1.7
SNAPPY_TAR="snappy-${SNAPPY_VERSION}.tar.gz"
SNAPPY_SITE="https://github.com/google/snappy/releases/download/${SNAPPY_VERSION}"
step="$SNAPPY_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/libsnappy.so" ]; }; }; then
    REBUILD_FFMPEG=1
    start_build
    #download "$SNAPPY_SITE" "$SNAPPY_TAR"
    download_github google snappy "${SNAPPY_VERSION}" "" "${SNAPPY_TAR}"
    untar "$SRC_PATH/$SNAPPY_TAR"
    pushd "snappy-${SNAPPY_VERSION}"
    mkdir tmp_build
    pushd tmp_build
    cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF -DNDEBUG"  -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DBUILD_SHARED_LIBS=ON -DSNAPPY_BUILD_TESTS=OFF -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "snappy-${SNAPPY_VERSION}"
    end_build
fi

# Install FFmpeg
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/ffmpeg.html
FFMPEG_VERSION=4.0.2
FFMPEG_VERSION_LIBAVCODEC=58.18.100
FFMPEG_TAR="ffmpeg-${FFMPEG_VERSION}.tar.bz2"
FFMPEG_SITE="http://www.ffmpeg.org/releases"
step="$FFMPEG_TAR"
if build_step && { force_build || { [ "${REBUILD_FFMPEG:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME"/ffmpeg-* || true
fi
if build_step && { force_build || { [ ! -d "$SDK_HOME/ffmpeg-gpl2" ] || [ ! -d "$SDK_HOME/ffmpeg-lgpl" ] || [ "$(env PKG_CONFIG_PATH=$SDK_HOME/ffmpeg-gpl2/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion libavcodec)" != "$FFMPEG_VERSION_LIBAVCODEC" ]; }; }; then
    start_build
    download "$FFMPEG_SITE" "$FFMPEG_TAR"
    untar "$SRC_PATH/$FFMPEG_TAR"
    pushd "ffmpeg-${FFMPEG_VERSION}"
    LGPL_SETTINGS=( "--enable-x86asm" "--enable-swscale" "--enable-avfilter" "--enable-avresample" "--enable-libmp3lame" "--enable-libvorbis" "--enable-libopus" "--enable-librsvg" "--enable-libtheora" "--enable-libopenh264" "--enable-libopenjpeg" "--enable-libsnappy" "--enable-libmodplug" "--enable-libvpx" "--enable-libsoxr" "--enable-libspeex" "--enable-libass" "--enable-libbluray" "--enable-lzma" "--enable-gnutls" "--enable-fontconfig" "--enable-libfreetype" "--enable-libfribidi" "--disable-libxcb" "--disable-libxcb-shm" "--disable-libxcb-xfixes" "--disable-indev=jack" "--disable-outdev=xv" "--disable-xlib" )
    GPL_SETTINGS=( "${LGPL_SETTINGS[@]}" "--enable-gpl" "--enable-libx264" "--enable-libx265" "--enable-libxvid" "--enable-version3" )

    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME/ffmpeg-gpl2" --libdir="$SDK_HOME/ffmpeg-gpl2/lib" --enable-shared --disable-static "${GPL_SETTINGS[@]}"
    make -j${MKJOBS}
    make install
    make distclean
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME/ffmpeg-lgpl" --libdir="$SDK_HOME/ffmpeg-lgpl/lib" --enable-shared --disable-static "${LGPL_SETTINGS[@]}"
    make install
    popd
    rm -rf "ffmpeg-${FFMPEG_VERSION}"
    end_build
fi

# Install dbus (for QtDBus)
# see http://www.linuxfromscratch.org/lfs/view/systemd/chapter06/dbus.html
DBUS_VERSION=1.12.8
DBUS_TAR="dbus-${DBUS_VERSION}.tar.gz"
DBUS_SITE="https://dbus.freedesktop.org/releases/dbus"
step="$DBUS_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/dbus-1.pc" ] || [ "$(pkg-config --modversion dbus-1)" != "$DBUS_VERSION" ]; }; }; then
    start_build
    download "$DBUS_SITE" "$DBUS_TAR"
    untar "$SRC_PATH/$DBUS_TAR"
    pushd "dbus-${DBUS_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-doxygen-docs --disable-xml-docs --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "dbus-${DBUS_VERSION}"
    end_build
fi


# Install ruby (necessary for qtwebkit)
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/ruby.html
RUBY_VERSION=2.4.3
RUBY_VERSION_SHORT=${RUBY_VERSION%.*}
RUBY_VERSION_PKG=${RUBY_VERSION_SHORT}.0
RUBY_TAR="ruby-${RUBY_VERSION}.tar.xz"
RUBY_SITE="http://cache.ruby-lang.org/pub/ruby/${RUBY_VERSION_SHORT}"
step="$RUBY_TAR"
if build_step && { force_build || { [ "${REBUILD_RUBY:-}" = "1" ]; }; }; then
    rm -rf "$SDK_HOME/lib/pkgconfig/ruby"*.pc
fi
if build_step && { force_build || { [ ! -s "$SDK_HOME/lib/pkgconfig/ruby-${RUBY_VERSION_SHORT}.pc" ] || [ "$(pkg-config --modversion "ruby-${RUBY_VERSION_SHORT}")" != "$RUBY_VERSION_PKG" ]; }; }; then
    start_build
    download "$RUBY_SITE" "$RUBY_TAR"
    untar "$SRC_PATH/$RUBY_TAR"
    pushd "ruby-${RUBY_VERSION}"
    env CFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-install-doc --disable-install-rdoc --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "ruby-${RUBY_VERSION}"
    end_build
fi

# Install breakpad tools
BREAKPAD_GIT=https://github.com/NatronGitHub/breakpad.git # breakpad for server-side or linux dump_syms, not for Natron!
BREAKPAD_GIT_COMMIT=f264b48eb0ed8f0893b08f4f9c7ae9685090ccb8
step="breakpad-${BREAKPAD_GIT_COMMIT}-natron"
if build_step && { force_build || { [ ! -s "${SDK_HOME}/bin/dump_syms" ]; }; }; then

    start_build
    rm -f breakpad || true
    git_clone_commit "$BREAKPAD_GIT" "$BREAKPAD_GIT_COMMIT"
    pushd breakpad
    git submodule update -i

    # Patch bug due to fix of glibc
    patch -Np1 -i "$INC_PATH"/patches/breakpad/0002-Replace-remaining-references-to-struct-ucontext-with.patch

    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure -prefix "$SDK_HOME"
    make
    cp src/tools/linux/dump_syms/dump_syms "$SDK_HOME/bin/"
    popd
    rm -rf breakpad
    end_build
fi

# Install valgrind
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/valgrind.html
VALGRIND_VERSION=3.13.0
VALGRIND_TAR="valgrind-${VALGRIND_VERSION}.tar.bz2"
VALGRIND_SITE="https://sourceware.org/ftp/valgrind"
step="$VALGRIND_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/valgrind" ]; }; }; then
    start_build
    download "$VALGRIND_SITE" "$VALGRIND_TAR"
    untar "$SRC_PATH/$VALGRIND_TAR"
    pushd "valgrind-${VALGRIND_VERSION}"
    sed -i '1904s/4/5/' coregrind/m_syswrap/syswrap-linux.c
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-only"${BITS}"bit
    make -j${MKJOBS}
    make install
    popd
    rm -rf "valgrind-${VALGRIND_VERSION}"
    end_build
fi

# install gdb (requires xz, zlib ncurses, python2, texinfo)
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/gdb.html
GDB_VERSION=8.1
GDB_TAR="gdb-${GDB_VERSION}.tar.xz" # when using the sdk during debug we get a conflict with native gdb, so bundle our own
GDB_SITE="ftp://ftp.gnu.org/gnu/gdb"
step="$GDB_TAR"
if build_step && { force_build || { [ ! -s "$SDK_HOME/bin/gdb" ]; }; }; then
    start_build
    download "$GDB_SITE" "$GDB_TAR"
    untar "$SRC_PATH/$GDB_TAR"
    pushd "gdb-${GDB_VERSION}"

    # Patch issue with header ordering for TRAP_HWBKPT
    patch -Np1 -i "$INC_PATH"/patches/gdb/gdb-Fix-ia64-defining-TRAP_HWBKPT-before-including-g.patch

    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-tui --with-system-readline --without-guile
    make -j${MKJOBS}
    make install
    popd
    rm -rf "gdb-${GDB_VERSION}"
    end_build
fi

if false; then # <--- Disabled Qt 5.6 (temporary)
if build_step && { force_build || { [ "${REBUILD_QT5:-}" = "1" ]; }; }; then
    rm -rf "$QT5PREFIX"
fi
# Qt5
QT5_VERSION=5.6.1
QT5_VERSION_SHORT=${QT5_VERSION%.*}

# we use vfxplatform fork
QTBASE_GIT=https://github.com/autodesk-forks/qtbase

# qtdeclarative and qtx11extras from autodesk-forks don't compile: we extract the originals and apply the necessary patches instead.
QT_DECLARATIVE_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtdeclarative-opensource-src-${QT5_VERSION}-1.tar.gz"
#QT_DECLARATIVE_URL="https://github.com/autodesk-forks/qtdeclarative/archive/Maya2018.tar.gz"
QT_X11EXTRAS_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtx11extras-opensource-src-${QT5_VERSION}-1.tar.gz"
#QT_X11EXTRAS_URL="https://github.com/autodesk-forks/qtx11extras/archive/Maya2017Update3.tar.gz"
QT_XMLPATTERNS_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtxmlpatterns-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_WEBVIEW_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtwebview-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_WEBSOCKETS_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtwebsockets-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_WEBENGINE_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtwebengine-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_WEBCHANNEL_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtwebchannel-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_TRANSLATIONS_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qttranslations-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_SVG_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtsvg-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_TOOLS_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qttools-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_SCRIPT_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtscript-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_QUICKCONTROLS_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtquickcontrols-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_QUICKCONTROLS2_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtquickcontrols2-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_MULTIMEDIA_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtmultimedia-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_IMAGEFORMATS_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtimageformats-opensource-src-${QT5_VERSION}-1.tar.gz"
QT_GRAPHICALEFFECTS_URL="http://download.qt.io/official_releases/qt/${QT5_VERSION_SHORT}/${QT5_VERSION}-1/submodules/qtgraphicaleffects-opensource-src-${QT5_VERSION}-1.tar.gz"

QT_MODULES=( qtxmlpatterns qtwebview qtwebsockets qtwebengine qtwebchannel qttools qttranslations qtsvg qtscript qtquickcontrols qtquickcontrols2 qtmultimedia qtimageformats qtgraphicaleffects qtdeclarative qtx11extras )
QT_MODULES_URL=( "$QT_XMLPATTERNS_URL" "$QT_WEBVIEW_URL" "$QT_WEBSOCKETS_URL" "$QT_WEBENGINE_URL" "$QT_WEBCHANNEL_URL" "$QT_TOOLS_URL" "$QT_TRANSLATIONS_URL" "$QT_SVG_URL" "$QT_SCRIPT_URL" "$QT_QUICKCONTROLS_URL" "$QT_QUICKCONTROLS2_URL" "$QT_MULTIMEDIA_URL" "$QT_IMAGEFORMATS_URL" "$QT_GRAPHICALEFFECTS_URL" "$QT_DECLARATIVE_URL" "$QT_X11EXTRAS_URL" )

export QTDIR="$QT5PREFIX"
step="qt5-${QT5_VERSION}"
if build_step && { force_build || { [ ! -s "$QT5PREFIX/lib/pkgconfig/Qt5Core.pc" ] || [ "$(env PKG_CONFIG_PATH=$QT5PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion Qt5Core)" != "$QT5_VERSION" ]; }; }; then
    start_build

    # Install QtBase
    QT_CONF=( "-v" "-openssl-linked" "-opengl" "desktop" "-opensource" "-nomake" "examples" "-nomake" "tests" "-release" "-no-gtkstyle" "-confirm-license" "-no-c++11" "-shared" "-qt-xcb" )

    if [ ! -d "qtbase" ]; then
        git_clone_commit "$QTBASE_GIT" tags/Maya2018
        pushd qtbase
        git submodule update -i --recursive --remote
        patch -p0 -i "$INC_PATH/patches/Qt/qtbase-revert-1b58d9acc493111390b31f0bffd6b2a76baca91b.diff"
        patch -p0 -i "$INC_PATH/patches/Qt/qtbase-threadpool.diff"
        popd
    fi
    pushd qtbase

    QT_LD_FLAGS="-L${SDK_HOME}/lib"
    if [ "$BITS" = "32" ]; then
        QT_LD_FLAGS="$QT_LD_FLAGS -L${SDK_HOME}/gcc/lib"
    else
        QT_LD_FLAGS="$QT_LD_FLAGS -L${SDK_HOME}/gcc/lib64"
    fi
    env CFLAGS="$BF" CXXFLAGS="$BF" LDFLAGS="$QT_LD_FLAGS" ./configure -prefix "$QT5PREFIX" "${QT_CONF[@]}"
    LD_LIBRARY_PATH="$(pwd)/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" make -j${MKJOBS}
    make install
    QTBASE_PRIV=( QtCore QtDBus QtGui QtNetwork QtOpenGL QtPlatformSupport QtPrintSupport QtSql QtTest QtWidgets QtXml )
    for i in "${QTBASE_PRIV[@]}"; do
        (cd "$QT5PREFIX/include/$i"; ln -sfv "$QT5_VERSION/$i/private" .)
    done

    popd
    rm -rf qtbase
    end_build

    for (( i=0; i<${#QT_MODULES[@]}; i++ )); do
        module=${QT_MODULES[i]}
        moduleurl=${QT_MODULES_URL[i]}
        step="${module}-${QT5_VERSION}"
        start_build
        package="${module}-${QT5_VERSION}.tar.gz"
        if [ ! -s "$SRC_PATH/$package" ]; then
            echo "*** downloading $package"
            $WGET --no-check-certificate "$moduleurl" -O "$SRC_PATH/$package" || rm "$SRC_PATH/$package"
        fi
        if [ ! -s "$SRC_PATH/$package" ]; then
            echo "*** Error: unable to download $package"
            exit 1
        fi
        untar "$SRC_PATH/$package"
        dirs=(${module}-*)
        if [ ${#dirs[@]} -ne 1 ]; then
            (>&2 echo "Error: Qt5: more than one match for ${module}-*: ${dirs[*]}")
            exit 1
        fi
        dir=${dirs[0]}
        pushd "$dir"
        if [ "$module" = "qtdeclarative" ]; then
            patch -p1 -i "$INC_PATH/patches/Qt/qtdeclarative-5.6.1-backport_core_profile_qtdeclarative_5.6.1.patch"
        fi
        if [ "$module" = "qtx11extras" ]; then
            patch -p1 -i "$INC_PATH/patches/Qt/qtx11extras-5.6.1-isCompositingManagerRunning.patch"
            patch -p1 -i "$INC_PATH/patches/Qt/qtx11extras-5.6.1-qtc-wip-add-peekeventqueue-api.patch"
        fi
        "$QT5PREFIX/bin/qmake"
        make -j${MKJOBS}
        make install
        popd
        rm -rf "$dir"

        # Copy private headers to includes because Pyside2 needs them
        ALL_MODULES_INCLUDE=("$QT5PREFIX"/include/*)
        for m in "${ALL_MODULES_INCLUDE[@]}"; do
            (cd "$m"; ln -sfv "$QT5_VERSION"/*/private .)
        done

        end_build
    done
fi
fi # ---> Disabled Qt 5.6 (temporary)

# pysetup
# If PYV is not set, set it to 2
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


# Install shiboken2. Requires clang. Not required for PySide2 5.6, which bundles shiboken2
SHIBOKEN2_VERSION=2
SHIBOKEN2_GIT="https://code.qt.io/pyside/shiboken"
SHIBOKEN2_COMMIT="5.9"
step="shiboken2-$SHIBOKEN2_COMMIT"
# SKIP: not required when using pyside-setup.git, see below
if false; then #[[ build_step && ( force_build || ( [ ! -s "$QT5PREFIX/lib/pkgconfig/shiboken2.pc" ] || [ "$(env PKG_CONFIG_PATH=$QT5PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion "shiboken2")" != "$SHIBOKEN2_VERSION" ]; }; }; then
    start_build
    git_clone_commit "$SHIBOKEN2_GIT" "$SHIBOKEN2_COMMIT"
    pushd shiboken

    mkdir -p build
    pushd build
    env PATH="$SDK_HOME/llvm/bin:$PATH" cmake ../ -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$QT5PREFIX"  \
        -DCMAKE_PREFIX_PATH="$SDK_HOME;$QT5PREFIX" \
        -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF -I${SDK_HOME}/include/libxml2" \
        -DCMAKE_SHARED_LINKER_FLAGS="-L${QT5PREFIX}/lib" \
        -DCMAKE_EXE_LINKER_FLAGS="-L${QT5PREFIX}/lib" \
        -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
        -DBUILD_TESTS=OFF            \
        -DPYTHON_EXECUTABLE="$PY_EXE" \
        -DPYTHON_LIBRARY="$PY_LIB" \
        -DPYTHON_INCLUDE_DIR="$PY_INC" \
        -DUSE_PYTHON3="$USE_PY3" \
        -DQT_QMAKE_EXECUTABLE="$QT5PREFIX/bin/qmake"
    make -j${MKJOBS} 
    make install
    popd
    popd
    rm -rf shiboken
    end_build
fi

if false; then # <--- Disabled pyside 2 (temporary)
# Install pyside2
PYSIDE2_VERSION="5.6.0"
PYSIDE2_GIT="https://code.qt.io/pyside/pyside-setup"
PYSIDE2_COMMIT="5.6" # 5.9 is unstable, see https://github.com/fredrikaverpil/pyside2-wheels/issues/55
step="pyside2-${PYSIDE2_COMMIT}"
if build_step && { force_build || { [ ! -x "$SDK_HOME/lib/python${PY2_VERSION_SHORT}/site-packages/PySide2/shiboken2" ] || [ "$($SDK_HOME/lib/python${PY2_VERSION_SHORT}/site-packages/PySide2/shiboken2 --version|head -1 | sed -e 's/^.* v\([^ ]*\)/\1/')" != "$PYSIDE2_VERSION" ]; }; }; then
    start_build
    git_clone_commit "$PYSIDE2_GIT" "$PYSIDE2_COMMIT"
    pushd pyside-setup

    # mkdir -p build
    # pushd build
    # cmake .. -DCMAKE_INSTALL_PREFIX="$QT5PREFIX" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DBUILD_TESTS=OFF \
    #       -DCMAKE_PREFIX_PATH="$SDK_HOME;$QT5PREFIX" \
    #       -DQT_QMAKE_EXECUTABLE="$QT5PREFIX/bin/qmake" \
    #       -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" \
    #       -DCMAKE_SHARED_LINKER_FLAGS="-L$QT5PREFIX/lib" \
    #       -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
    #       -DPYTHON_EXECUTABLE="$PY_EXE" \
    #       -DPYTHON_LIBRARY="$PY_LIB" \
    #       -DPYTHON_INCLUDE_DIR="$PY_INC"
    # make -j${MKJOBS}
    # make install
    # popd

    # it really installs in SDK_HOME, whatever option is given to --prefix
    env CC="$CC" CXX="$CXX" CMAKE_PREFIX_PATH="$SDK_HOME;$QT5PREFIX" python${PYV} setup.py install --qmake "$QT5PREFIX/bin/qmake" --prefix "$QT5PREFIX" --openssl "$SDK_HOME/bin/openssl" --record files.txt
    cat files.txt
    ln -sf "$SDK_HOME/lib/python${PY2_VERSION_SHORT}/site-packages/PySide2/shiboken2" "$QT5PREFIX/bin/shiboken2"
    popd
    rm -rf pyside-setup
    end_build
fi
fi # ---> Disabled pyside 2 (temporary)


if build_step && { force_build || { [ "${REBUILD_QT4:-}" = "1" ]; }; }; then
    rm -rf "$QT4PREFIX" || true
fi
# Qt4
if dobuild; then
    export QTDIR="$QT4PREFIX"
fi
step="$QT4_TAR"
if build_step && { force_build || { [ ! -s "$QT4PREFIX/lib/pkgconfig/QtCore.pc" ] || [ "$(env PKG_CONFIG_PATH=$QT4PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion QtCore)" != "$QT4_VERSION" ]; }; }; then
    start_build
    QT_CONF=( "-v" "-system-zlib" "-system-libtiff" "-system-libpng" "-no-libmng" "-system-libjpeg" "-no-gtkstyle" "-glib" "-xrender" "-xrandr" "-xcursor" "-xfixes" "-xinerama" "-fontconfig" "-xinput" "-sm" "-no-multimedia" "-confirm-license" "-release" "-opensource" "-dbus-linked" "-opengl" "desktop" "-nomake" "demos" "-nomake" "docs" "-nomake" "examples" "-optimized-qmake" )
    # qtwebkit is installed separately (see below)
    # also install the sqlite plugin.
    # link with the SDK's openssl
    QT_CONF+=( "-no-webkit" "-plugin-sql-sqlite" "-system-sqlite" "-openssl-linked" )
    # Add MariaDB/MySQL plugin
    QT_CONF+=( "-plugin-sql-mysql" )
    # Add PostgresSQL plugin
    QT_CONF+=( "-plugin-sql-psql" )
    # icu must be enabled in Qt, since it is enabled in libxml2
    # see https://bugs.webkit.org/show_bug.cgi?id=82824#c1
    if [ "$LIBXML2_ICU" -eq 1 ]; then
        QT_CONF+=( "-icu" )
    else
        QT_CONF+=( "-no-icu" )
    fi
    # disable phonon (can be provided by http://www.linuxfromscratch.org/blfs/view/7.9/kde/phonon.html)
    QT_CONF+=( "-no-phonon" "-no-phonon-backend" )
    # disable openvg
    QT_CONF+=( "-no-openvg" )
    
    download "$QT4_SITE" "$QT4_TAR"
    untar "$SRC_PATH/$QT4_TAR"
    pushd "qt-everywhere-opensource-src-${QT4_VERSION}"

    ############################################################
    # Fedora patches

    ## Patches from https://download.fedoraproject.org/pub/fedora/linux/development/rawhide/Everything/source/tree/Packages/q/qt-4.8.7-33.fc28.src.rpm

    # set default QMAKE_CFLAGS_RELEASE
    Patch2=qt-everywhere-opensource-src-4.8.0-tp-multilib-optflags.patch

    # get rid of timestamp which causes multilib problem
    Patch4=qt-everywhere-opensource-src-4.8.5-uic_multilib.patch

    # reduce debuginfo in qtwebkit (webcore)
    Patch5=qt-everywhere-opensource-src-4.8.5-webcore_debuginfo.patch

    # cups16 printer discovery
    Patch6=qt-cupsEnumDests.patch

    # prefer adwaita over gtk+ on DE_GNOME
    # https://bugzilla.redhat.com/show_bug.cgi?id=1192453
    Patch10=qt-prefer_adwaita_on_gnome.patch

    # enable ft lcdfilter
    Patch15=qt-x11-opensource-src-4.5.1-enable_ft_lcdfilter.patch

    # may be upstreamable, not sure yet
    # workaround for gdal/grass crashers wrt glib_eventloop null deref's
    Patch23=qt-everywhere-opensource-src-4.6.3-glib_eventloop_nullcheck.patch

    # hack out largely useless (to users) warnings about qdbusconnection
    # (often in kde apps), keep an eye on https://git.reviewboard.kde.org/r/103699/
    Patch25=qt-everywhere-opensource-src-4.8.3-qdbusconnection_no_debug.patch

    # lrelease-qt4 tries to run qmake not qmake-qt4 (http://bugzilla.redhat.com/820767)
    Patch26=qt-everywhere-opensource-src-4.8.1-linguist_qmake-qt4.patch

    # enable debuginfo in libQt3Support
    Patch27=qt-everywhere-opensource-src-4.8.1-qt3support_debuginfo.patch

    # kde4/multilib QT_PLUGIN_PATH
    Patch28=qt-everywhere-opensource-src-4.8.5-qt_plugin_path.patch

    ## upstreamable bits
    # add support for pkgconfig's Requires.private to qmake
    Patch50=qt-everywhere-opensource-src-4.8.4-qmake_pkgconfig_requires_private.patch

    # FTBFS against newer firebird
    Patch51=qt-everywhere-opensource-src-4.8.7-firebird.patch

    # workaround major/minor macros possibly being defined already
    Patch52=qt-everywhere-opensource-src-4.8.7-QT_VERSION_CHECK.patch

    # fix invalid inline assembly in qatomic_{i386,x86_64}.h (de)ref implementations
    Patch53=qt-x11-opensource-src-4.5.0-fix-qatomic-inline-asm.patch

    # fix invalid assumptions about mysql_config --libs
    # http://bugzilla.redhat.com/440673
    Patch54=qt-everywhere-opensource-src-4.8.5-mysql_config.patch

    # http://bugs.kde.org/show_bug.cgi?id=180051#c22
    Patch55=qt-everywhere-opensource-src-4.6.2-cups.patch

    # backport https://codereview.qt-project.org/#/c/205874/
    Patch56=qt-everywhere-opensource-src-4.8.7-mariadb.patch

    # Fails to create debug build of Qt projects on mingw (rhbz#653674)
    Patch64=qt-everywhere-opensource-src-4.8.5-QTBUG-14467.patch

    # fix QTreeView crash triggered by KPackageKit (patch by David Faure)
    Patch65=qt-everywhere-opensource-src-4.8.0-tp-qtreeview-kpackagekit-crash.patch

    # fix the outdated standalone copy of JavaScriptCore
    Patch67=qt-everywhere-opensource-src-4.8.6-s390.patch

    # https://bugs.webkit.org/show_bug.cgi?id=63941
    # -Wall + -Werror = fail
    Patch68=qt-everywhere-opensource-src-4.8.3-no_Werror.patch

    # revert qlist.h commit that seems to induce crashes in qDeleteAll<QList (QTBUG-22037)
    Patch69=qt-everywhere-opensource-src-4.8.0-QTBUG-22037.patch

    # Buttons in Qt applications not clickable when run under gnome-shell (#742658, QTBUG-21900)
    Patch71=qt-everywhere-opensource-src-4.8.5-QTBUG-21900.patch

    # workaround
    # sql/drivers/tds/qsql_tds.cpp:341:49: warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
    Patch74=qt-everywhere-opensource-src-4.8.5-tds_no_strict_aliasing.patch

    # add missing method for QBasicAtomicPointer on s390(x)
    Patch76=qt-everywhere-opensource-src-4.8.0-s390-atomic.patch

    # don't spam in release/no_debug mode if libicu is not present at runtime
    Patch77=qt-everywhere-opensource-src-4.8.3-icu_no_debug.patch

    # https://bugzilla.redhat.com/show_bug.cgi?id=810500
    Patch81=qt-everywhere-opensource-src-4.8.2--assistant-crash.patch

    # https://bugzilla.redhat.com/show_bug.cgi?id=694385
    # https://bugs.kde.org/show_bug.cgi?id=249217
    # https://bugreports.qt-project.org/browse/QTBUG-4862
    # QDir::homePath() should account for an empty HOME environment variable on X11
    Patch82=qt-everywhere-opensource-src-4.8.5-QTBUG-4862.patch

    # poll support
    Patch83=qt-4.8-poll.patch

    # fix QTBUG-35459 (too low entityCharacterLimit=1024 for CVE-2013-4549)
    Patch84=qt-everywhere-opensource-src-4.8.5-QTBUG-35459.patch

    # systemtrayicon plugin support (for appindicators)
    Patch86=qt-everywhere-opensource-src-4.8.6-systemtrayicon.patch

    # fixes for LibreOffice from the upstream Qt bug tracker (#1105422):
    Patch87=qt-everywhere-opensource-src-4.8.6-QTBUG-37380.patch
    Patch88=qt-everywhere-opensource-src-4.8.6-QTBUG-34614.patch
    Patch89=qt-everywhere-opensource-src-4.8.6-QTBUG-38585.patch

    # build against the system clucene09-core
    Patch90=qt-everywhere-opensource-src-4.8.6-system-clucene.patch

    # fix arch autodetection for 64-bit MIPS
    Patch91=qt-everywhere-opensource-src-4.8.7-mips64.patch

    # fix build issue(s) with gcc6
    Patch100=qt-everywhere-opensource-src-4.8.7-gcc6.patch

    # support alsa-1.1.x
    Patch101=qt-everywhere-opensource-src-4.8.7-alsa-1.1.patch

    # upstream patches
    # backported from Qt5 (essentially)
    # http://bugzilla.redhat.com/702493
    # https://bugreports.qt-project.org/browse/QTBUG-5545
    Patch102=qt-everywhere-opensource-src-4.8.5-qgtkstyle_disable_gtk_theme_check.patch
    # workaround for MOC issues with Boost headers (#756395)
    # https://bugreports.qt-project.org/browse/QTBUG-22829
    Patch113=qt-everywhere-opensource-src-4.8.6-QTBUG-22829.patch

    # aarch64 support, https://bugreports.qt-project.org/browse/QTBUG-35442
    Patch180=qt-aarch64.patch

    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch4" # -p1 -b .uic_multilib
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch5" # -p1 -b .webcore_debuginfo
    # ie, where cups-1.6+ is present
    #%if 0%{?fedora} || 0%{?rhel} > 7
    ##patch6 -p1 -b .cupsEnumDests
    #%endif
    patch -Np0 -i "$INC_PATH/patches/Qt/$Patch10" # -p0 -b .prefer_adwaita_on_gnome
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch15" # -p1 -b .enable_ft_lcdfilter
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch23" # -p1 -b .glib_eventloop_nullcheck
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch25" # -p1 -b .qdbusconnection_no_debug
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch26" # -p1 -b .linguist_qtmake-qt4
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch27" # -p1 -b .qt3support_debuginfo
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch28" # -p1 -b .qt_plugin_path
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch50" # -p1 -b .qmake_pkgconfig_requires_private
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch51" # -p1 -b .firebird
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch52" # -p1 -b .QT_VERSION_CHECK
    ## TODO: still worth carrying?  if so, upstream it.
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch53" # -p1 -b .qatomic-inline-asm
    ## TODO: upstream me
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch54" # -p1 -b .mysql_config
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch55" # -p1 -b .cups-1
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch56" # -p1 -b .mariadb
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch64" # -p1 -b .QTBUG-14467
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch65" # -p1 -b .qtreeview-kpackagekit-crash
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch67" # -p1 -b .s390
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch68" # -p1 -b .no_Werror
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch69" # -p1 -b .QTBUG-22037
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch71" # -p1 -b .QTBUG-21900
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch74" # -p1 -b .tds_no_strict_aliasing
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch76" # -p1 -b .s390-atomic
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch77" # -p1 -b .icu_no_debug
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch81" # -p1 -b .assistant-crash
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch82" # -p1 -b .QTBUG-4862
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch83" # -p1 -b .poll
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch87" # -p1 -b .QTBUG-37380
    patch -Np0 -i "$INC_PATH/patches/Qt/$Patch88" # -p0 -b .QTBUG-34614
    patch -Np0 -i "$INC_PATH/patches/Qt/$Patch89" # -p0 -b .QTBUG-38585

    #%if 0%{?system_clucene}
    #patch -Np1 -i "$INC_PATH/patches/Qt/$Patch90" # -p1 -b .system_clucene
    ## delete bundled copy
    #rm -rf src/3rdparty/clucene
    #%endif
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch91" # -p1 -b .mips64
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch100" # -p1 -b .gcc6
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch101" # -p1 -b .alsa1.1

    # upstream patches
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch102" # -p1 -b .qgtkstyle_disable_gtk_theme_check
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch113" # -p1 -b .QTBUG-22829

    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch180" # -p1 -b .aarch64

    # upstream git

    # security fixes
    # regression fixes for the security fixes
    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch84" # -p1 -b .QTBUG-35459

    patch -Np1 -i "$INC_PATH/patches/Qt/$Patch86" # -p1 -b .systemtrayicon

    #####################################################################
    # Natron-specific patches
    
    patch -Np0 -i "$INC_PATH"/patches/Qt/patch-qt-custom-threadpool.diff
    patch -Np0 -i "$INC_PATH"/patches/Qt/qt4-kde-fix.diff


    #####################################################################
    # MacPorts/homebrew patches

    # (25) avoid zombie processes; see also:
    # https://trac.macports.org/ticket/46608
    # https://codereview.qt-project.org/#/c/61294/
    # approved but abandoned.
    patch -Np0 -i "$INC_PATH"/patches/Qt/patch-src_corelib_io_qprocess_unix.cpp.diff    
    # (28) Better invalid fonttable handling
    # Qt commit 0a2f2382 on July 10, 2015 at 7:22:32 AM EDT.
    # not included in 4.8.7 release.
    patch -Np0 -i "$INC_PATH"/patches/Qt/patch-better-invalid-fonttable-handling.diff
    # (30) Backport of Qt5 patch to fix an issue with null bytes in QSetting strings (QTBUG-56124).
    patch -Np0 -i "$INC_PATH"/patches/Qt/patch-qsettings-null.diff
    # avoid conflict with newer versions of pcre, see eg https://github.com/LLNL/spack/pull/4270
    patch -Np0 -i "$INC_PATH"/patches/Qt/qt4-pcre.patch
    patch -Np1 -i "$INC_PATH"/patches/Qt/0001-Enable-building-with-C-11-and-C-14.patch
    patch -Np1 -i "$INC_PATH"/patches/Qt/qt4-selection-flags-static_cast.patch
    if version_gt "$OPENSSL_VERSION" 1.0.9999; then
        # OpenSSL 1.1 support from ArchLinux https://aur.archlinux.org/cgit/aur.git/tree/qt4-openssl-1.1.patch?h=qt4
        patch -Np1 -i "$INC_PATH"/patches/Qt/qt4-openssl-1.1.patch
    fi

    #QT_SRC="$(pwd)/src"
    env CFLAGS="$BF" CXXFLAGS="$BF" OPENSSL_LIBS="-L$SDK_HOME/lib -lssl -lcrypto" ./configure -prefix "$QT4PREFIX" "${QT_CONF[@]}" -shared

    # https://bugreports.qt-project.org/browse/QTBUG-5385
    LD_LIBRARY_PATH="$(pwd)/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" make -j${MKJOBS} || exit  1

    make install
    if [ -d "$QT4PREFIX/tests" ]; then
        rm -rf "$QT4PREFIX/tests"
    fi
    #  Remove references to the build directory from the installed .pc files by running the following command as the root user:
    find $QT4PREFIX/lib/pkgconfig -name "*.pc" -exec perl -pi -e "s, -L$PWD/?\S+,,g" {} \;
    #  Remove references to the build directory in the installed library dependency (prl) files by running the following command as the root user:
    for file in $QT4PREFIX/lib/libQt*.prl; do
        sed -r -e '/^QMAKE_PRL_BUILD_DIR/d'    \
            -e 's/(QMAKE_PRL_LIBS =).*/\1/' \
            -i $file
    done
    popd
    rm -rf "qt-everywhere-opensource-src-${QT4_VERSION}"
    end_build
fi

step="$QT4WEBKIT_TAR"
if build_step && { force_build || { [ ! -s "$QT4PREFIX/lib/pkgconfig/QtWebKit.pc" ] || [ "$(env PKG_CONFIG_PATH=$QT4PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion QtWebKit)" != "$QT4WEBKIT_VERSION_PKG" ]; }; }; then
    start_build
    # install a more recent qtwebkit, see http://www.linuxfromscratch.org/blfs/view/7.9/x/qt4.html
    download "$QT4WEBKIT_SITE" "$QT4WEBKIT_TAR"
    mkdir "qtwebkit-${QT4WEBKIT_VERSION}"
    pushd "qtwebkit-${QT4WEBKIT_VERSION}"
    untar "$SRC_PATH/$QT4WEBKIT_TAR"
    patch -Np1 -i "$INC_PATH"/patches/Qt/qtwebkit-2.3.4-gcc5-1.patch
    # disable xslt if libxml2 is compiled with ICU support, or qtwebkit will not compile, see https://aur.archlinux.org/packages/qtwebkit
    if [ "$LIBXML2_ICU" -eq 1 ]; then
        xslt_flag="--no-xslt"
    else
        xslt_flag=""
    fi
    QTDIR="$QT4PREFIX" PATH="$QT4PREFIX/bin:$PATH" \
         Tools/Scripts/build-webkit --qt            \
         --no-webkit2    \
         "$xslt_flag" \
         --prefix="$QT4PREFIX" --makeargs=-j${MKJOBS}
    make -C WebKitBuild/Release install -j${MKJOBS}
    popd
    rm -rf "qtwebkit-${QT4WEBKIT_VERSION}"
    end_build
fi

if dobuild; then
    # add qt4 to lib path to build shiboken and pyside
    LD_LIBRARY_PATH="$QT4PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    #LD_RUN_PATH="$LD_LIBRARY_PATH"
fi

# Install shiboken
SHIBOKEN_VERSION=1.2.4
#SHIBOKEN_TAR="shiboken-${SHIBOKEN_VERSION}.tar.bz2"
#SHIBOKEN_SITE="https://download.qt.io/official_releases/pyside"
SHIBOKEN_TAR="Shiboken-${SHIBOKEN_VERSION}.tar.gz"
step="$SHIBOKEN_TAR"
if dobuild; then
    SHIBOKEN_PREFIX="$QT4PREFIX"
fi
if build_step && { force_build || { [ ! -s "$SHIBOKEN_PREFIX/lib/pkgconfig/shiboken.pc" ] || [ "$(env PKG_CONFIG_PATH=$SHIBOKEN_PREFIX/lib/pkgconfig:$QT4PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion shiboken)" != "$SHIBOKEN_VERSION" ]; }; }; then
    start_build
    #download "$SHIBOKEN_SITE" "$SHIBOKEN_TAR"
    download_github pyside Shiboken "${SHIBOKEN_VERSION}" "" "${SHIBOKEN_TAR}"
    untar "$SRC_PATH/$SHIBOKEN_TAR"
    #pushd "shiboken-$SHIBOKEN_VERSION"
    pushd "Shiboken-$SHIBOKEN_VERSION"

    mkdir -p build
    pushd build
    env  cmake ../ -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$SHIBOKEN_PREFIX"  \
        -DCMAKE_PREFIX_PATH="$SDK_HOME;$SHIBOKEN_PREFIX;$QT4PREFIX" \
        -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF -I${SDK_HOME}/include/libxml2" \
        -DCMAKE_SHARED_LINKER_FLAGS="-L$QT4PREFIX/lib" \
        -DCMAKE_EXE_LINKER_FLAGS="-L$QT4PREFIX/lib" \
        -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
        -DBUILD_TESTS=OFF            \
        -DPYTHON_EXECUTABLE="$PY_EXE" \
        -DPYTHON_LIBRARY="$PY_LIB" \
        -DPYTHON_INCLUDE_DIR="$PY_INC" \
        -DUSE_PYTHON3="$USE_PY3" \
        -DQT_QMAKE_EXECUTABLE="$QT4PREFIX/bin/qmake"
    make -j${MKJOBS} 
    make install
    popd
    popd
    rm -rf "shiboken-$SHIBOKEN_VERSION"
    end_build
fi

# Install pyside
PYSIDE_VERSION=1.2.4
#PYSIDE_TAR="pyside-qt4.8+${PYSIDE_VERSION}.tar.bz2"
#PYSIDE_SITE="https://download.qt.io/official_releases/pyside"
PYSIDE_TAR="PySide-${PYSIDE_VERSION}.tar.gz"
step="$PYSIDE_TAR"
if dobuild; then
    PYSIDE_PREFIX="$QT4PREFIX"
fi
if build_step && { force_build || { [ ! -s "$PYSIDE_PREFIX/lib/pkgconfig/pyside${PYSIDE_V:-}.pc" ] || [ "$(env PKG_CONFIG_PATH=$PYSIDE_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion pyside)" != "$PYSIDE_VERSION" ]; }; }; then
    start_build
    #download "$PYSIDE_SITE" "$PYSIDE_TAR"
    download_github pyside PySide "${PYSIDE_VERSION}" "" "${PYSIDE_TAR}"
    untar "$SRC_PATH/$PYSIDE_TAR"
    #pushd "pyside-qt4.8+${PYSIDE_VERSION}"
    pushd "PySide-${PYSIDE_VERSION}"

    mkdir -p build
    pushd build
    cmake .. -DCMAKE_INSTALL_PREFIX="$PYSIDE_PREFIX" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DBUILD_TESTS=OFF \
          -DCMAKE_PREFIX_PATH="$SDK_HOME;$PYSIDE_PREFIX;$QT4PREFIX" \
          -DQT_QMAKE_EXECUTABLE="$QT4PREFIX/bin/qmake" \
          -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" \
          -DCMAKE_SHARED_LINKER_FLAGS="-L$QT4PREFIX/lib" \
          -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" \
          -DPYTHON_EXECUTABLE="$PY_EXE" \
          -DPYTHON_LIBRARY="$PY_LIB" \
          -DPYTHON_INCLUDE_DIR="$PY_INC"
    make -j${MKJOBS}
    make install
    popd
    popd
    #rm -rf "pyside-qt4.8+${PYSIDE_VERSION}"
    rm -rf "PySide-${PYSIDE_VERSION}"
    end_build
fi

if dobuild; then
    # make sure all libs are user-writable and executable
    chmod 755 "$SDK_HOME"/lib*/lib*.so*

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
        LD_LIBRARY_PATH="$SDK_HOME/$subdir/lib:${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" find "$SDK_HOME/$subdir/lib" -name '*.so' -exec bash -c 'ldd {} 2>&1 | fgrep "not found" &>/dev/null' \; -print
        LD_LIBRARY_PATH="$SDK_HOME/$subdir/lib:${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" find "$SDK_HOME/$subdir/bin" -maxdepth 1 -exec bash -c 'ldd {} 2>&1 | fgrep "not found" &>/dev/null' \; -print
    done
    echo "Check for broken libraries and binaries... done!"

    echo
    echo "Natron SDK Done"
    echo
fi

if [ "${GEN_DOCKERFILE:-}" = "1" ]; then
    cat <<EOF
WORKDIR /home
RUN rm -rf /home/Natron-sdk-build
RUN rm -rf /opt/Natron-sdk/var/log/Natron-Linux-x86_64-SDK
EOF
fi

exit 0

# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:

