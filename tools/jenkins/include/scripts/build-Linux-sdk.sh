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

#
# Build Natron SDK (CY2015) for Linux
#

#options:
#TAR_SDK=1 : Make an archive of the SDK when building is done and store it in $SRC_PATH
#UPLOAD_SDK=1 : Upload the SDK tar archive to $REPO_DEST if TAR_SDK=1

source common.sh

if [ "${DEBUG:-}" = "1" ]; then
    CMAKE_BUILD_TYPE="Debug"
else
    CMAKE_BUILD_TYPE="Release"
fi

# Check distro and version. CentOS/RHEL 6.4 only!

if [ "$PKGOS" = "Linux" ]; then
    if [ ! -s /etc/redhat-release ]; then
        echo "Build system has been designed for CentOS/RHEL, use at OWN risk!"
        sleep 5
    else
        RHEL_MAJOR=$(cut -d" " -f3 < /etc/redhat-release | cut -d "." -f1)
        RHEL_MINOR=$(cut -d" " -f3 < /etc/redhat-release | cut -d "." -f2)
        if [ "$RHEL_MAJOR" != "6" ] || [ "$RHEL_MINOR" != "4" ]; then
            echo "Wrong version of CentOS/RHEL, 6.4 is the only supported version!"
            sleep 5
        fi
    fi
fi


BINARIES_URL="$REPO_DEST/Third_Party_Binaries"


SDK="Linux-$ARCH-SDK"

if [ -z "${MKJOBS:-}" ]; then
    #Default to 4 threads
    MKJOBS=$DEFAULT_MKJOBS
fi

download() {
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
}

download_github() {
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
}

# see https://stackoverflow.com/a/43136160/2607517
# $1: git repo
# $2: commit SHA1
git_clone_commit() {
    local repo=${1##*/}
    repo=${repo%.git}
    git clone -n "$1" && ( cd "$repo" && git checkout "$2" )
    ## the following requires git 2.5.0 with uploadpack.allowReachableSHA1InWant=true
    #git init
    #git remote add origin "$1"
    #git fetch --depth 1 origin "$2"
    #git checkout FETCH_HEAD
}

untar() {
    echo "*** extracting $1"
    tar xf "$1"
}

start_build() {
    echo "*** Building $1..."
    touch "$LOGDIR/$1".stamp
    exec 3>&1 4>&2 1>"$LOGDIR/$1".log 2>&1
    pushd "$TMP_PATH"
}

end_build() {
    popd #"$TMP_PATH"
    find "$SDK_HOME" -newer "$LOGDIR/$1".stamp > "$LOGDIR/$1"-files.txt
    rm "$LOGDIR/$1".stamp
    exec 1>&3 2>&4
    echo "*** Done"
}

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

# Install openssl for installer
# see http://www.linuxfromscratch.org/blfs/view/svn/postlfs/openssl10.html
OPENSSL_VERSION=1.0.2o
OPENSSL_TAR="openssl-${OPENSSL_VERSION}.tar.gz" # always a new version around the corner
OPENSSL_SITE="https://www.openssl.org/source"
if [ ! -s "$SDK_HOME/installer/lib/pkgconfig/openssl.pc" ] || [ "$(env PKG_CONFIG_PATH="$SDK_HOME/installer/lib/pkgconfig:$SDK_HOME/installer/share/pkgconfig" pkg-config --modversion openssl)" != "$OPENSSL_VERSION" ]; then
    start_build "$OPENSSL_TAR.installer"
    download "$OPENSSL_SITE" "$OPENSSL_TAR"
    untar "$SRC_PATH/$OPENSSL_TAR"
    pushd "openssl-$OPENSSL_VERSION"
    # Use versioned symbols for OpenSSL binary compatibility.
    patch -Np1 -i "$INC_PATH"/patches/openssl/openssl-1.0.2m-compat_versioned_symbols-1.patch
    # the static qt4 uses -openssl-linked to link to the static version
    ./config --prefix="$SDK_HOME/installer" no-shared
    # no parallel build for openssl, see https://github.com/openssl/openssl/issues/298
    make
    make install
    popd
    rm -rf "openssl-$OPENSSL_VERSION"
    end_build "$OPENSSL_TAR.installer"
fi

# Install static qt4 for installer
# see http://www.linuxfromscratch.org/blfs/view/7.9/x/qt4.html
QT4_VERSION=4.8.7
QT4_VERSION_SHORT=${QT4_VERSION%.*}
QT4_TAR="qt-everywhere-opensource-src-${QT4_VERSION}.tar.gz"
QT4_SITE="https://download.qt.io/official_releases/qt/${QT4_VERSION_SHORT}/${QT4_VERSION}"
QT4WEBKIT_VERSION=2.3.4
QT4WEBKIT_VERSION_SHORT=${QT4WEBKIT_VERSION%.*}
QT4WEBKIT_VERSION_PKG=4.10.4 # version from QtWebkit.pc
QT4WEBKIT_TAR="qtwebkit-${QT4WEBKIT_VERSION}.tar.gz"
QT4WEBKIT_SITE=" http://download.kde.org/stable/qtwebkit-${QT4WEBKIT_VERSION_SHORT}/${QT4WEBKIT_VERSION}"
if [ ! -s "$SDK_HOME/installer/bin/qmake" ]; then
    start_build "$QT4_TAR.installer"
    QTIFW_CONF=( "-no-multimedia" "-no-gif" "-qt-libpng" "-no-opengl" "-no-libmng" "-no-libtiff" "-no-libjpeg" "-static" "-openssl-linked" "-confirm-license" "-release" "-opensource" "-nomake" "demos" "-nomake" "docs" "-nomake" "examples" "-no-gtkstyle" "-no-webkit" "-no-avx" "-no-openvg" "-no-phonon" "-no-phonon-backend" "-I${SDK_HOME}/installer/include" "-L${SDK_HOME}/installer/lib" )

    download "$QT4_SITE" "$QT4_TAR"
    untar "$SRC_PATH/$QT4_TAR"
    pushd "qt-everywhere-opensource-src-${QT4_VERSION}"
    #patch -p0 -i "$INC_PATH"/patches/Qt/patch-qt-custom-threadpool.diff
    patch -p0 -i "$INC_PATH"/patches/Qt/qt4-kde-fix.diff
    # (25) avoid zombie processes; see also:
    # https://trac.macports.org/ticket/46608
    # https://codereview.qt-project.org/#/c/61294/
    # approved but abandoned.
    patch -p0 -i "$INC_PATH"/patches/Qt/patch-src_corelib_io_qprocess_unix.cpp.diff    
    # (28) Better invalid fonttable handling
    # Qt commit 0a2f2382 on July 10, 2015 at 7:22:32 AM EDT.
    # not included in 4.8.7 release.
    patch -p0 -i "$INC_PATH"/patches/Qt/patch-better-invalid-fonttable-handling.diff
    # (30) Backport of Qt5 patch to fix an issue with null bytes in QSetting strings (QTBUG-56124).
    patch -p0 -i "$INC_PATH"/patches/Qt/patch-qsettings-null.diff
    # avoid conflict with newer versions of pcre, see eg https://github.com/LLNL/spack/pull/4270
    patch -p0 -i "$INC_PATH"/patches/Qt/qt4-pcre.patch
    patch -p1 -i "$INC_PATH"/patches/Qt/qt-everywhere-opensource-src-4.8.7-gcc6.patch
    patch -p1 -i "$INC_PATH"/patches/Qt/0001-Enable-building-with-C-11-and-C-14.patch
    patch -p1 -i "$INC_PATH"/patches/Qt/qt4-selection-flags-static_cast.patch

    ./configure -prefix "$SDK_HOME/installer" "${QTIFW_CONF[@]}" -v

    # https://bugreports.qt-project.org/browse/QTBUG-5385
    env LD_LIBRARY_PATH="$(pwd)/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" make -j${MKJOBS}
    make install
    popd
    rm -rf "qt-everywhere-opensource-src-${QT4_VERSION}"
    end_build "$QT4_TAR.installer"
fi

# Install qtifw
QTIFW_GIT=https://github.com/devernay/installer-framework.git
if [ ! -s $SDK_HOME/installer/bin/binarycreator ]; then
    start_build "qtifw"
    git_clone_commit "$QTIFW_GIT" 1.6-natron
    pushd installer-framework
    "$SDK_HOME/installer/bin/qmake"
    make -j${MKJOBS}
    strip -s bin/*
    cp bin/* "$SDK_HOME/installer/bin/"
    popd
    rm -rf installer-framework
    end_build "qtifw"
fi

# Setup env
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

# Old Natron 2 version is 4.8.5
#GCC_VERSION=8.1.0 # GCC 8.1 breaks the timeline GUI on Linux, see https://github.com/NatronGitHub/Natron/issues/279
GCC_VERSION=7.3.0
#GCC_VERSION=5.4.0
#GCC_VERSION=4.9.4
GCC_TAR="gcc-${GCC_VERSION}.tar.gz"
GCC_SITE="ftp://ftp.funet.fi/pub/mirrors/sources.redhat.com/pub/gcc/releases/gcc-${GCC_VERSION}"

# Install gcc
MPC_VERSION=1.1.0
MPC_TAR="mpc-${MPC_VERSION}.tar.gz"
MPC_SITE="https://ftp.gnu.org/gnu/mpc"

MPFR_VERSION=4.0.1
MPFR_TAR="mpfr-${MPFR_VERSION}.tar.bz2"
MPFR_SITE="http://www.mpfr.org/mpfr-${MPFR_VERSION}"

GMP_VERSION=6.1.2
GMP_TAR="gmp-${GMP_VERSION}.tar.bz2"
GMP_SITE="https://gmplib.org/download/gmp"

ISL_VERSION=0.19
if [ "$GCC_VERSION" = 4.8.5 ]; then
    ISL_VERSION=0.14.1
fi
ISL_TAR="isl-${ISL_VERSION}.tar.bz2"
ISL_SITE="http://isl.gforge.inria.fr"

CLOOG_VERSION=0.18.4
CLOOG_TAR="cloog-${CLOOG_VERSION}.tar.gz"
CLOOG_SITE="http://www.bastoul.net/cloog/pages/download/count.php3?url=."

if [ ! -s "$SDK_HOME/gcc-$GCC_VERSION/bin/gcc" ]; then
    start_build "$GCC_TAR"
    download "$GCC_SITE" "$GCC_TAR"
    download "$MPC_SITE" "$MPC_TAR"
    download "$MPFR_SITE" "$MPFR_TAR"
    download "$GMP_SITE" "$GMP_TAR"
    download "$ISL_SITE" "$ISL_TAR"
    download "$CLOOG_SITE" "$CLOOG_TAR"
    untar "$SRC_PATH/$GCC_TAR"
    pushd "gcc-$GCC_VERSION"
    untar "$SRC_PATH/$MPC_TAR"
    mv "mpc-$MPC_VERSION" mpc
    untar "$SRC_PATH/$MPFR_TAR"
    mv "mpfr-$MPFR_VERSION" mpfr
    untar "$SRC_PATH/$GMP_TAR"
    mv "gmp-$GMP_VERSION" gmp
    untar "$SRC_PATH/$ISL_TAR"
    mv "isl-$ISL_VERSION" isl
    untar "$SRC_PATH/$CLOOG_TAR"
    mv "cloog-$CLOOG_VERSION" cloog
    ./configure --prefix="$SDK_HOME/gcc-${GCC_VERSION}" --disable-multilib --enable-languages=c,c++
    make -j$MKJOBS
    #make -k check
    make install
    popd #"gcc-$GCC_VERSION"
    rm -rf "gcc-$GCC_VERSION"
    ln -sfnv "gcc-${GCC_VERSION}" "$SDK_HOME/gcc"
    end_build "$GCC_TAR"
fi

export CC="${SDK_HOME}/gcc/bin/gcc"
export CXX="${SDK_HOME}/gcc/bin/g++"

# Install m4
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/m4.html
M4_VERSION=1.4.18
M4_TAR="m4-${M4_VERSION}.tar.xz"
M4_SITE="https://ftp.gnu.org/gnu/m4"
if [ ! -s "$SDK_HOME/bin/m4" ]; then
    start_build "$M4_TAR"
    download "$M4_SITE" "$M4_TAR"
    untar "$SRC_PATH/$M4_TAR"
    pushd "m4-${M4_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "m4-${M4_VERSION}"
    end_build "$M4_TAR"
fi

# Install bzip2
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/bzip2.html
BZIP2_VERSION=1.0.6
BZIP2_TAR="bzip2-${BZIP2_VERSION}.tar.gz"
BZIP2_SITE="http://www.bzip.org/${BZIP2_VERSION}"
if [ ! -s "$SDK_HOME/lib/libbz2.so.1" ]; then
    start_build "$BZIP2_TAR"
    download "$BZIP2_SITE" "$BZIP2_TAR"
    untar "$SRC_PATH/$BZIP2_TAR"
    pushd "bzip2-${BZIP2_VERSION}"
    $GSED -e 's/^CFLAGS=\(.*\)$/CFLAGS=\1 \$\(BIGFILES\)/' -i ./Makefile-libbz2_so
    $GSED -i "s/libbz2.so.1.0 -o libbz2.so.1.0.6/libbz2.so.1 -o libbz2.so.1.0.6/;s/rm -f libbz2.so.1.0/rm -f libbz2.so.1/;s/ln -s libbz2.so.1.0.6 libbz2.so.1.0/ln -s libbz2.so.1.0.6 libbz2.so.1/" Makefile-libbz2_so
    make -f Makefile-libbz2_so
    install -m755 "libbz2.so.${BZIP2_VERSION}" "$SDK_HOME/lib"
    install -m644 bzlib.h "$SDK_HOME/include/"
    cd "$SDK_HOME/lib"
    ln -sfv "libbz2.so.${BZIP2_VERSION}" libbz2.so.1.0
    ln -sfv libbz2.so.1.0 libbz2.so.1
    ln -sfv libbz2.so.1 libbz2.so
    popd
    rm -rf "bzip2-${BZIP2_VERSION}"
    end_build "$BZIP2_TAR"
fi

# install ncurses (required for gdb and readline)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/ncurses.html
NCURSES_VERSION=6.1
NCURSES_VERSION_PKG=6.1.20180127
NCURSES_TAR="ncurses-${NCURSES_VERSION}.tar.gz"
NCURSES_SITE="ftp://ftp.gnu.org/gnu/ncurses"
if [ ! -s "$SDK_HOME/lib/pkgconfig/ncurses.pc" ] || [ "$(pkg-config --modversion ncurses)" != "$NCURSES_VERSION_PKG" ]; then
    start_build "$NCURSES_TAR"
    download "$NCURSES_SITE" "$NCURSES_TAR"
    untar "$SRC_PATH/$NCURSES_TAR"
    pushd "ncurses-${NCURSES_VERSION}"
    # Don't install a static library that is not handled by configure:
    sed -i '/LIBTOOL_INSTALL/d' c++/Makefile.in
    # see http://archive.linuxfromscratch.org/mail-archives/lfs-dev/2015-August/070322.html
    sed -i 's/.:space:./ \t/g' ncurses/base/MKlib_gen.sh
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --with-shared --without-debug --without-normal --enable-pc-files --with-pkg-config-libdir="$SDK_HOME/lib/pkgconfig" --enable-widec
    make -j${MKJOBS}
    make install
    #  Many applications still expect the linker to be able to find non-wide-character Ncurses libraries. Trick such applications into linking with wide-character libraries by means of symlinks and linker scripts: 
    for lib in ncurses form panel menu ; do
        rm -vf                    "$SDK_HOME/lib/lib${lib}.so"
        echo "INPUT(-l${lib}w)" > "$SDK_HOME/lib/lib${lib}.so"
        ln -sfv ${lib}w.pc        "$SDK_HOME/lib/pkgconfig/${lib}.pc"
    done
    # Finally, make sure that old applications that look for -lcurses at build time are still buildable:
    rm -vf                     "$SDK_HOME/lib/libcursesw.so"
    echo "INPUT(-lncursesw)" > "$SDK_HOME/lib/libcursesw.so"
    ln -sfv libncurses.so      "$SDK_HOME/lib/libcurses.so"
    ln -sfnv ncursesw "$SDK_HOME/include/ncurses"

    for f in curses.h form.h ncurses.h panel.h term.h termcap.h; do
        ln -sfv "ncursesw/$f" "$SDK_HOME/include/$f"
    done
    popd
    rm -rf "ncurses-${NCURSES_VERSION}"
    end_build "$NCURSES_TAR"
fi

# install bison (for SeExpr)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/bison.html
BISON_VERSION=3.0.4
BISON_TAR="bison-${BISON_VERSION}.tar.gz"
BISON_SITE="http://ftp.gnu.org/pub/gnu/bison"
if [ ! -s "$SDK_HOME/bin/bison" ]; then
    start_build "$BISON_TAR"
    download "$BISON_SITE" "$BISON_TAR"
    untar "$SRC_PATH/$BISON_TAR"
    pushd "bison-${BISON_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "bison-${BISON_VERSION}"
    end_build "$BISON_TAR"
fi

# install flex (for SeExpr)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/flex.html
FLEX_VERSION=2.6.4
FLEX_TAR="flex-${FLEX_VERSION}.tar.gz"
FLEX_SITE="https://github.com/westes/flex/releases/download/v${FLEX_VERSION}"
if [ "${REBUILD_OPENJPEG:-}" = "1" ]; then
    rm -rf "$SDK_HOME"/lib/libopenjp* || true
fi
if [ ! -s "$SDK_HOME/bin/flex" ]; then
    start_build "$FLEX_TAR"
    download "$FLEX_SITE" "$FLEX_TAR"
    #download_github westes flex "${FLEX_VERSION}" v "${FLEX_TAR}"
    untar "$SRC_PATH/$FLEX_TAR"
    pushd "flex-${FLEX_VERSION}"
    # First, fix a problem introduced with glibc-2.26:
    sed -i "/math.h/a #include <malloc.h>" src/flexdef.h
    HELP2MAN=true ./configure --prefix="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "flex-${FLEX_VERSION}"
    end_build "$FLEX_TAR"
fi

# install pkg-config
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/pkg-config.html
PKGCONFIG_VERSION=0.29.2
PKGCONFIG_TAR="pkg-config-${PKGCONFIG_VERSION}.tar.gz"
PKGCONFIG_SITE="https://pkg-config.freedesktop.org/releases"
if [ ! -s "$SDK_HOME/bin/pkg-config" ] || [ "$($SDK_HOME/bin/pkg-config --version)" != "$PKGCONFIG_VERSION" ]; then
    start_build "$PKGCONFIG_TAR"
    download "$PKGCONFIG_SITE" "$PKGCONFIG_TAR"
    untar "$SRC_PATH/$PKGCONFIG_TAR"
    pushd "pkg-config-${PKGCONFIG_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --with-internal-glib --disable-host-tool
    make -j${MKJOBS}
    make install
    popd
    rm -rf "pkg-config-${PKGCONFIG_VERSION}"
    end_build "$PKGCONFIG_TAR"
fi

# install libtool
LIBTOOL_VERSION=2.4.6
LIBTOOL_TAR="libtool-${LIBTOOL_VERSION}.tar.gz"
LIBTOOL_SITE="http://ftp.gnu.org/pub/gnu/libtool"
if [ ! -s "$SDK_HOME/bin/libtool" ]; then
    start_build "$LIBTOOL_TAR"
    download "$LIBTOOL_SITE" "$LIBTOOL_TAR"
    untar "$SRC_PATH/$LIBTOOL_TAR"
    pushd "libtool-${LIBTOOL_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libtool-${LIBTOOL_VERSION}"
    end_build "$LIBTOOL_TAR"
fi

# Install gperf (used by fontconfig) as well as assemblers (yasm and nasm)
# Install gperf
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/gperf.html
GPERF_VERSION=3.1
GPERF_TAR="gperf-${GPERF_VERSION}.tar.gz"
GPERF_SITE="http://ftp.gnu.org/pub/gnu/gperf"
if [ ! -s "$SDK_HOME/bin/gperf" ]; then
    start_build "$GPERF_TAR"
    download "$GPERF_SITE" "$GPERF_TAR"
    untar "$SRC_PATH/$GPERF_TAR"
    pushd "gperf-${GPERF_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "gperf-${GPERF_VERSION}"
    end_build "$GPERF_TAR"
fi

# Install autoconf
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/autoconf.html
AUTOCONF_VERSION=2.69
AUTOCONF_TAR="autoconf-${AUTOCONF_VERSION}.tar.xz"
AUTOCONF_SITE="https://ftp.gnu.org/gnu/autoconf"
if [ ! -s "$SDK_HOME/bin/autoconf" ]; then
    start_build "$AUTOCONF_TAR"
    download "$AUTOCONF_SITE" "$AUTOCONF_TAR"
    untar "$SRC_PATH/$AUTOCONF_TAR"
    pushd "autoconf-${AUTOCONF_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "autoconf-${AUTOCONF_VERSION}"
    end_build "$AUTOCONF_TAR"
fi

# Install automake
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/automake.html
AUTOMAKE_VERSION=1.16.1
AUTOMAKE_TAR="automake-${AUTOMAKE_VERSION}.tar.xz"
AUTOMAKE_SITE="https://ftp.gnu.org/gnu/automake"
if [ ! -s "$SDK_HOME/bin/automake" ] || [ "$("$SDK_HOME/bin/automake" --version | head -1 | awk '{print $4}')" != "$AUTOMAKE_VERSION" ]; then
    start_build "$AUTOMAKE_TAR"
    download "$AUTOMAKE_SITE" "$AUTOMAKE_TAR"
    untar "$SRC_PATH/$AUTOMAKE_TAR"
    pushd "automake-${AUTOMAKE_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "automake-${AUTOMAKE_VERSION}"
    end_build "$AUTOMAKE_TAR"
fi

# Install yasm
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/yasm.html
YASM_VERSION=1.3.0
YASM_TAR="yasm-${YASM_VERSION}.tar.gz"
YASM_SITE="http://www.tortall.net/projects/yasm/releases"
if [ ! -s "$SDK_HOME/bin/yasm" ]; then
    start_build "$YASM_TAR"
    download "$YASM_SITE" "$YASM_TAR"
    untar "$SRC_PATH/$YASM_TAR"
    pushd "yasm-${YASM_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "yasm-${YASM_VERSION}"
    end_build "$YASM_TAR"
fi

# Install nasm (for x264, lame, ans others)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/nasm.html
NASM_VERSION=2.13.03
NASM_TAR="nasm-${NASM_VERSION}.tar.gz"
NASM_SITE="http://www.nasm.us/pub/nasm/releasebuilds/${NASM_VERSION}"
if [ ! -s "$SDK_HOME/bin/nasm" ]; then
    start_build "$NASM_TAR"
    download "$NASM_SITE" "$NASM_TAR"
    untar "$SRC_PATH/$NASM_TAR"
    pushd "nasm-${NASM_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "nasm-${NASM_VERSION}"
    end_build "$NASM_TAR"
fi

# Install gmp (used by ruby)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/gmp.html
if [ ! -s "$SDK_HOME/include/gmp.h" ]; then
    REBUILD_RUBY=1
    start_build "$GMP_TAR"
    download "$GMP_SITE" "$GMP_TAR"
    untar "$SRC_PATH/$GMP_TAR"
    pushd "gmp-${GMP_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared --enable-cxx --build="${ARCH}-unknown-linux-gnu"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "gmp-${GMP_VERSION}"
    end_build "$GMP_TAR"
fi

# Install openssl
# see http://www.linuxfromscratch.org/blfs/view/svn/postlfs/openssl10.html
#OPENSSL_VERSION=1.0.2o # defined above
OPENSSL_TAR="openssl-${OPENSSL_VERSION}.tar.gz" # always a new version around the corner
OPENSSL_SITE="https://www.openssl.org/source"
if [ ! -s "$SDK_HOME/lib/pkgconfig/openssl.pc" ] || [ "$(env PKG_CONFIG_PATH="$SDK_HOME/lib/pkgconfig:$SDK_HOME/share/pkgconfig" pkg-config --modversion openssl)" != "$OPENSSL_VERSION" ]; then
    start_build "$OPENSSL_TAR"
    download "$OPENSSL_SITE" "$OPENSSL_TAR"
    untar "$SRC_PATH/$OPENSSL_TAR"
    pushd "openssl-$OPENSSL_VERSION"
    # Use versioned symbols for OpenSSL binary compatibility.
    patch -Np1 -i "$INC_PATH"/patches/openssl/openssl-1.0.2m-compat_versioned_symbols-1.patch
    env CFLAGS="$BF" CXXFLAGS="$BF" ./config --prefix="$SDK_HOME" shared enable-cms enable-camellia enable-idea enable-mdc2 enable-tlsext enable-rfc3779
    # no parallel build for openssl, see https://github.com/openssl/openssl/issues/298
    make
    make install
    popd
    rm -rf "openssl-$OPENSSL_VERSION"
    end_build "$OPENSSL_TAR"
fi


# install patchelf
ELF_VERSION=0.9
ELF_TAR="patchelf-${ELF_VERSION}.tar.bz2"
ELF_SITE="https://nixos.org/releases/patchelf/patchelf-${ELF_VERSION}/"
if [ ! -s "$SDK_HOME/bin/patchelf" ]; then
    start_build "$ELF_TAR"
    download "$ELF_SITE" "$ELF_TAR"
    untar "$SRC_PATH/$ELF_TAR"
    pushd "patchelf-${ELF_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    make
    make install
    #cp src/patchelf "$SDK_HOME/bin/"
    popd
    rm -rf "patchelf-${ELF_VERSION}"
    end_build "$ELF_TAR"
fi


# Install gettext
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/gettext.html
GETTEXT_VERSION=0.19.8.1
GETTEXT_TAR="gettext-${GETTEXT_VERSION}.tar.gz"
GETTEXT_SITE="http://ftp.gnu.org/pub/gnu/gettext"
if [ ! -s "$SDK_HOME/bin/gettext" ]; then
    start_build "$GETTEXT_TAR"
    download "$GETTEXT_SITE" "$GETTEXT_TAR"
    untar "$SRC_PATH/$GETTEXT_TAR"
    pushd "gettext-${GETTEXT_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "gettext-${GETTEXT_VERSION}"
    end_build "$GETTEXT_TAR"
fi

# Install expat
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/expat.html
EXPAT_VERSION=2.2.5
EXPAT_TAR="expat-${EXPAT_VERSION}.tar.bz2"
EXPAT_SITE="https://sourceforge.net/projects/expat/files/expat/${EXPAT_VERSION}"
if [ "${REBUILD_EXPAT:-}" = "1" ]; then
    rm -rf "$SDK_HOME"/include/expat*.h "$SDK_HOME"/lib/libexpat* "$SDK_HOME"/lib/pkgconfig/expat* || true
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/expat.pc" ] || [ "$(pkg-config --modversion expat)" != "$EXPAT_VERSION" ]; then
    start_build "$EXPAT_TAR"
    download "$EXPAT_SITE" "$EXPAT_TAR"
    untar "$SRC_PATH/$EXPAT_TAR"
    pushd "expat-${EXPAT_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "expat-${EXPAT_VERSION}"
    end_build "$EXPAT_TAR"
fi

# Install perl (required to install XML:Parser perl module used by intltool)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/perl.html
PERL_VERSION=5.26.2
PERL_TAR="perl-${PERL_VERSION}.tar.gz"
PERL_SITE="http://www.cpan.org/src/5.0"
if [ ! -s "$SDK_HOME/bin/perl" ]; then
    start_build "$PERL_TAR"
    download "$PERL_SITE" "$PERL_TAR"
    untar "$SRC_PATH/$PERL_TAR"
    pushd "perl-${PERL_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./Configure -des -Dprefix="$SDK_HOME" -Dusethreads -Duseshrplib -Dcc="$CC"
    make -j${MKJOBS}
    make install
    PERL_MM_USE_DEFAULT=1 ${SDK_HOME}/bin/cpan -Ti XML::Parser
    popd
    rm -rf "perl-${PERL_VERSION}"
    end_build "$PERL_TAR"
fi

# Install intltool
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/intltool.html
INTLTOOL_VERSION=0.51.0
INTLTOOL_TAR="intltool-${INTLTOOL_VERSION}.tar.gz"
INTLTOOL_SITE="https://launchpad.net/intltool/trunk/${INTLTOOL_VERSION}/+download"
if [ ! -s "$SDK_HOME/bin/intltoolize" ]; then
    start_build "$INTLTOOL_TAR"
    download "$INTLTOOL_SITE" "$INTLTOOL_TAR"
    untar "$SRC_PATH/$INTLTOOL_TAR"
    pushd "intltool-${INTLTOOL_VERSION}"
    patch -p1 -i "$INC_PATH/patches/intltool-0.51.0-perl-5.22-compatibility.patch"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "intltool-${INTLTOOL_VERSION}"
    end_build "$INTLTOOL_TAR"
fi

# Install zlib
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/zlib.html
ZLIB_VERSION=1.2.11
ZLIB_TAR="zlib-${ZLIB_VERSION}.tar.gz"
ZLIB_SITE="https://zlib.net"
if [ "${REBUILD_ZLIB:-}" = "1" ]; then
    rm -rf $SDK_HOME/lib/libz.* || true
    rm -f $SDK_HOME/lib/pkgconfig/zlib.pc || true
fi
if [ ! -s "$SDK_HOME/lib/libz.so.1" ]; then
    start_build "$ZLIB_TAR"
    download "$ZLIB_SITE" "$ZLIB_TAR"
    untar "$SRC_PATH/$ZLIB_TAR"
    pushd "zlib-${ZLIB_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "zlib-${ZLIB_VERSION}"
    end_build "$ZLIB_TAR"
fi


# Install readline
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/readline.html
READLINE_VERSION=7.0
READLINE_VERSION_MAJOR=${READLINE_VERSION%.*}
READLINE_TAR="readline-${READLINE_VERSION}.tar.gz"
READLINE_SITE="https://ftp.gnu.org/gnu/readline"
if [ ! -s "$SDK_HOME/lib/libreadline.so.${READLINE_VERSION_MAJOR}" ]; then
    start_build "$READLINE_TAR"
    download "$READLINE_SITE" "$READLINE_TAR"
    untar "$SRC_PATH/$READLINE_TAR"
    pushd "readline-${READLINE_VERSION}"
    sed -i '/MV.*old/d' Makefile.in
    sed -i '/{OLDSUFF}/c:' support/shlib-install
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    make -j${MKJOBS} SHLIB_LIBS="-L${SDK_HOME}/lib -lncursesw"
    make install
    popd
    rm -rf "readline-${READLINE_VERSION}"
    end_build "$READLINE_TAR"
fi

# Install libzip
ZIP_VERSION=1.5.1
ZIP_TAR="libzip-${ZIP_VERSION}.tar.xz"
ZIP_SITE="https://libzip.org/download"
if [ ! -s "$SDK_HOME/lib/pkgconfig/libzip.pc" ] || [ "$(pkg-config --modversion libzip)" != "$ZIP_VERSION" ]; then
    start_build "$ZIP_TAR"
    download "$ZIP_SITE" "$ZIP_TAR"
    untar "$SRC_PATH/$ZIP_TAR"
    pushd "libzip-${ZIP_VERSION}"
    mkdir build-natron
    pushd build-natron
    #env CFLAGS="$BF" CXXFLAGS="$BF" ../configure --prefix="$SDK_HOME" --disable-static --enable-shared
    # libzip adds -I$SDK_HOME/include before its own includes, and thus includes the installed zip.h
    rm  "$SDK_HOME/include/zip.h" || true
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "libzip-${ZIP_VERSION}"
    end_build "$ZIP_TAR"
fi

# Install xz
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/xz.html
XZ_VERSION=5.2.4
XZ_TAR="xz-${XZ_VERSION}.tar.bz2"
XZ_SITE="https://tukaani.org/xz"
if [ ! -s "$SDK_HOME/lib/pkgconfig/liblzma.pc" ] || [ "$(pkg-config --modversion liblzma)" != "$XZ_VERSION" ]; then
    start_build "$XZ_TAR"
    download "$XZ_SITE" "$XZ_TAR"
    untar "$SRC_PATH/$XZ_TAR"
    pushd "xz-$XZ_VERSION"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "xz-$XZ_VERSION"
    end_build "$XZ_TAR"
fi


# Install nettle (for gnutls)
# see http://www.linuxfromscratch.org/blfs/view/svn/postlfs/nettle.html
# disable doc, because install-info in CentOS 6.4 is buggy and always returns exit status 1.
NETTLE_VERSION=3.4
#NETTLE_VERSION_SHORT=${NETTLE_VERSION%.*}
NETTLE_TAR="nettle-${NETTLE_VERSION}.tar.gz"
NETTLE_SITE="https://ftp.gnu.org/gnu/nettle"
if [ ! -s "$SDK_HOME/lib/pkgconfig/nettle.pc" ] || [ "$(pkg-config --modversion nettle)" != "$NETTLE_VERSION" ]; then
    start_build "$NETTLE_TAR"
    download "$NETTLE_SITE" "$NETTLE_TAR"
    untar "$SRC_PATH/$NETTLE_TAR"
    pushd "nettle-${NETTLE_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared --disable-documentation
    make -j${MKJOBS}
    make -k install
    make -k install
    popd
    rm -rf "nettle-${NETTLE_VERSION}"
    end_build "$NETTLE_TAR"
fi

# Install libtasn1 (for gnutls)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libtasn1.html
LIBTASN1_VERSION=4.13
#LIBTASN1_VERSION_SHORT=${LIBTASN1_VERSION%.*}
LIBTASN1_TAR="libtasn1-${LIBTASN1_VERSION}.tar.gz"
LIBTASN1_SITE="https://ftp.gnu.org/gnu/libtasn1"
if [ ! -s "$SDK_HOME/lib/pkgconfig/libtasn1.pc" ] || [ "$(pkg-config --modversion libtasn1)" != "$LIBTASN1_VERSION" ]; then
    start_build "$LIBTASN1_TAR"
    download "$LIBTASN1_SITE" "$LIBTASN1_TAR"
    untar "$SRC_PATH/$LIBTASN1_TAR"
    pushd "libtasn1-${LIBTASN1_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared --disable-doc
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libtasn1-${LIBTASN1_VERSION}"
    end_build "$LIBTASN1_TAR"
fi

# Install libunistring (for gnutls)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libunistring.html
LIBUNISTRING_VERSION=0.9.10
LIBUNISTRING_VERSION_HEX=0x00090A
#LIBUNISTRING_VERSION_SHORT=${LIBUNISTRING_VERSION%.*}
LIBUNISTRING_TAR="libunistring-${LIBUNISTRING_VERSION}.tar.gz"
LIBUNISTRING_SITE="https://ftp.gnu.org/gnu/libunistring"
if [ ! -s "$SDK_HOME/include/unistring/version.h" ] || ! fgrep "${LIBUNISTRING_VERSION_HEX}" "$SDK_HOME/include/unistring/version.h" &>/dev/null; then
    start_build "$LIBUNISTRING_TAR"
    download "$LIBUNISTRING_SITE" "$LIBUNISTRING_TAR"
    untar "$SRC_PATH/$LIBUNISTRING_TAR"
    pushd "libunistring-${LIBUNISTRING_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libunistring-${LIBUNISTRING_VERSION}"
    end_build "$LIBUNISTRING_TAR"
fi

# Install libffi
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/libffi.html
FFI_VERSION=3.2.1
FFI_TAR="libffi-${FFI_VERSION}.tar.gz"
FFI_SITE="ftp://sourceware.org/pub/libffi"
if [ ! -s "$SDK_HOME/lib/pkgconfig/libffi.pc" ] || [ "$(pkg-config --modversion libffi)" != "$FFI_VERSION" ]; then
    start_build "$FFI_TAR"
    download "$FFI_SITE" "$FFI_TAR"
    untar "$SRC_PATH/$FFI_TAR"
    pushd "libffi-${FFI_VERSION}"
    # Modify the Makefile to install headers into the standard /usr/include directory instead of /usr/lib/libffi-3.2.1/include.
    sed -e '/^includesdir/ s/$(libdir).*$/$(includedir)/' \
        -i include/Makefile.in
    sed -e '/^includedir/ s/=.*$/=@includedir@/' \
        -e 's/^Cflags: -I${includedir}/Cflags:/' \
        -i libffi.pc.in
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libffi-${FFI_VERSION}"
    end_build "$FFI_TAR"
fi

# Install p11-kit (for gnutls)
# http://www.linuxfromscratch.org/blfs/view/svn/postlfs/p11-kit.html
P11KIT_VERSION=0.23.12
#P11KIT_VERSION_SHORT=${P11KIT_VERSION%.*}
P11KIT_TAR="p11-kit-${P11KIT_VERSION}.tar.gz"
P11KIT_SITE="https://github.com/p11-glue/p11-kit/releases/download/${P11KIT_VERSION}"
if [ ! -s "$SDK_HOME/lib/pkgconfig/p11-kit-1.pc" ] || [ "$(pkg-config --modversion p11-kit-1)" != "$P11KIT_VERSION" ]; then
    start_build "$P11KIT_TAR"
    download "$P11KIT_SITE" "$P11KIT_TAR"
    untar "$SRC_PATH/$P11KIT_TAR"
    pushd "p11-kit-${P11KIT_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "p11-kit-${P11KIT_VERSION}"
    end_build "$P11KIT_TAR"
fi

# Install gnutls (for ffmpeg)
# see http://www.linuxfromscratch.org/blfs/view/cvs/postlfs/gnutls.html
GNUTLS_VERSION=3.6.2
GNUTLS_VERSION_SHORT=${GNUTLS_VERSION%.*}
GNUTLS_TAR="gnutls-${GNUTLS_VERSION}.tar.xz"
GNUTLS_SITE="ftp://ftp.gnupg.org/gcrypt/gnutls/v${GNUTLS_VERSION_SHORT}"
if [ ! -s "$SDK_HOME/lib/pkgconfig/gnutls.pc" ] || [ "$(pkg-config --modversion gnutls)" != "$GNUTLS_VERSION" ]; then
    start_build "$GNUTLS_TAR"
    download "$GNUTLS_SITE" "$GNUTLS_TAR"
    untar "$SRC_PATH/$GNUTLS_TAR"
    pushd "gnutls-${GNUTLS_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared --with-default-trust-store-pkcs11="pkcs11:"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "gnutls-${GNUTLS_VERSION}"
    end_build "$GNUTLS_TAR"
fi


# Install curl
# see http://www.linuxfromscratch.org/blfs/view/cvs/basicnet/curl.html
CURL_VERSION=7.60.0
CURL_TAR="curl-${CURL_VERSION}.tar.bz2"
CURL_SITE="https://curl.haxx.se/download"
if [ ! -s "$SDK_HOME/lib/pkgconfig/libcurl.pc" ] || [ "$(pkg-config --modversion libcurl)" != "$CURL_VERSION" ]; then
    start_build "$CURL_TAR"
    download "$CURL_SITE" "$CURL_TAR"
    untar "$SRC_PATH/$CURL_TAR"
    pushd "curl-${CURL_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --enable-threaded-resolver --prefix="$SDK_HOME" --with-ssl="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "curl-${CURL_VERSION}"
    end_build "$CURL_TAR"
fi

# Install libarchive (for cmake)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libarchive.html
LIBARCHIVE_VERSION=3.3.2
LIBARCHIVE_TAR="libarchive-${LIBARCHIVE_VERSION}.tar.gz"
LIBARCHIVE_SITE="http://www.libarchive.org/downloads"
if [ ! -s "$SDK_HOME/lib/pkgconfig/libarchive.pc" ] || [ "$(pkg-config --modversion libarchive)" != "$LIBARCHIVE_VERSION" ]; then
    start_build "$LIBARCHIVE_TAR"
    download "$LIBARCHIVE_SITE" "$LIBARCHIVE_TAR"
    untar "$SRC_PATH/$LIBARCHIVE_TAR"
    pushd "libarchive-${LIBARCHIVE_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libarchive-${LIBARCHIVE_VERSION}"
    end_build "$LIBARCHIVE_TAR"
fi

# # Install libuv (for cmake)
# # http://www.linuxfromscratch.org/blfs/view/svn/general/libuv.html
# LIBUV_VERSION=1.21.0
# LIBUV_TAR="libuv-${LIBUV_VERSION}.tar.gz"
# LIBUV_SITE="http://www.libuv.org/downloads"
# if [ ! -s "$SDK_HOME/lib/pkgconfig/libuv.pc" ] || [ "$(pkg-config --modversion libuv)" != "$LIBUV_VERSION" ]; then
#     start_build "$LIBUV_TAR"
#     download_github libuv libuv "${LIBUV_VERSION}" v "${LIBUV_TAR}"
#     untar "$SRC_PATH/$LIBUV_TAR"
#     pushd "libuv-${LIBUV_VERSION}"
#     ./autogen.sh
#     env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static
#     make -j${MKJOBS}
#     make install
#     popd
#     rm -rf "libuv-${LIBUV_VERSION}"
#     end_build "$LIBUV_TAR"
# fi

# Install cmake
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/cmake.html
#
# cmake 3.10.1 does not compile with the system libuv
# Utilities/cmlibuv/src/unix/posix-poll.c: In function 'uv__platform_loop_init':
# Utilities/cmlibuv/src/unix/posix-poll.c:37:7: error: 'uv_loop_t {aka struct uv_loop_s}' has no member named 'poll_fds'
#   loop->poll_fds = NULL;
#       ^~
# Since there is no way to disable libuv alone (--no-system-libuv unavailable in ./bootstrap),
# we disable all system libs. Voilà!
CMAKE_VERSION=3.11.4
CMAKE_VERSION_SHORT=${CMAKE_VERSION%.*}
CMAKE_TAR="cmake-${CMAKE_VERSION}.tar.gz"
CMAKE_SITE="https://cmake.org/files/v${CMAKE_VERSION_SHORT}"
if [ ! -s "$SDK_HOME/bin/cmake" ] || [ $("$SDK_HOME/bin/cmake" --version | head -1 |awk '{print $3}') != "$CMAKE_VERSION" ]; then
    start_build "$CMAKE_TAR"
    download "$CMAKE_SITE" "$CMAKE_TAR"
    untar "$SRC_PATH/$CMAKE_TAR"
    pushd "cmake-${CMAKE_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --no-system-libs --prefix="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "cmake-${CMAKE_VERSION}"
    end_build "$CMAKE_TAR"
fi

# Install icu
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/icu.html
ICU_VERSION=62.1
ICU_TAR="icu4c-${ICU_VERSION//./_}-src.tgz"
ICU_SITE="http://download.icu-project.org/files/icu4c/${ICU_VERSION}"
if [ ! -s "$SDK_HOME/lib/pkgconfig/icu-i18n.pc" ] || [ "$(pkg-config --modversion icu-i18n)" != "$ICU_VERSION" ]; then
    start_build "$ICU_TAR"
    download "$ICU_SITE" "$ICU_TAR"
    untar "$SRC_PATH/$ICU_TAR"
    pushd icu/source
    # Note: removed
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf icu
    end_build "$ICU_TAR"
fi

# Install Berkeley DB (optional for python2 and python3)
# see http://www.linuxfromscratch.org/blfs/view/svn/server/db.html
#BERKELEYDB_VERSION=6.2.32
#BERKELEYDB_TAR="db-${BERKELEYDB_VERSION}.tar.gz"
#BERKELEYDB_SITE="http://download.oracle.com/berkeley-db"
## FIXME if [ ! -s "$SDK_HOME/lib/pkgconfig/sqlite3.pc" ] || [ "$(pkg-config --modversion sqlite3)" != "$BERKELEYDB_VERSION" ]; then
#    start_build "$BERKELEYDB_TAR"
#    download "$BERKELEYDB_SITE" "$BERKELEYDB_TAR"
#    untar "$SRC_PATH/$BERKELEYDB_TAR"
#    pushd "sqlite-autoconf-${BERKELEYDB_VERSION_INT}"
#    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" -enable-compat185 --enable-dbm --disable-static --enable-cxx
#    make -j${MKJOBS}
#    make install
#    popd
#    rm -rf "sqlite-autoconf-${BERKELEYDB_VERSION_INT}"
#    end_build "$BERKELEYDB_TAR"
#fi

# Install sqlite (required for webkit and QtSql SQLite module, optional for python2)
# see http://www.linuxfromscratch.org/blfs/view/svn/server/sqlite.html
SQLITE_VERSION=3.24.0
SQLITE_VERSION_INT=3240000
SQLITE_YEAR=2018
SQLITE_TAR="sqlite-autoconf-${SQLITE_VERSION_INT}.tar.gz"
SQLITE_SITE="https://sqlite.org/${SQLITE_YEAR}"
if [ ! -s "$SDK_HOME/lib/pkgconfig/sqlite3.pc" ] || [ "$(pkg-config --modversion sqlite3)" != "$SQLITE_VERSION" ]; then
    start_build "$SQLITE_TAR"
    download "$SQLITE_SITE" "$SQLITE_TAR"
    untar "$SRC_PATH/$SQLITE_TAR"
    pushd "sqlite-autoconf-${SQLITE_VERSION_INT}"
    env CFLAGS="$BF \
            -DSQLITE_ENABLE_FTS3=1 \
            -DSQLITE_ENABLE_COLUMN_METADATA=1     \
            -DSQLITE_ENABLE_UNLOCK_NOTIFY=1       \
            -DSQLITE_SECURE_DELETE=1              \
            -DSQLITE_ENABLE_DBSTAT_VTAB=1" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "sqlite-autoconf-${SQLITE_VERSION_INT}"
    end_build "$SQLITE_TAR"
fi

# Install pcre (required by glib)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/pcre.html
PCRE_VERSION=8.42
PCRE_TAR="pcre-${PCRE_VERSION}.tar.bz2"
PCRE_SITE="https://ftp.pcre.org/pub/pcre"
if [ ! -s "$SDK_HOME/lib/pkgconfig/libpcre.pc" ] || [ "$(pkg-config --modversion libpcre)" != "$PCRE_VERSION" ]; then
    start_build "$PCRE_TAR"
    download "$PCRE_SITE" "$PCRE_TAR"
    untar "$SRC_PATH/$PCRE_TAR"
    pushd "pcre-${PCRE_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --enable-utf8 --enable-unicode-properties --enable-pcre16 --enable-pcre32 --enable-pcregrep-libz --enable-pcregrep-libbz2 --enable-pcretest-libreadline
    make -j${MKJOBS}
    make install
    popd
    rm -rf "pcre-${PCRE_VERSION}"
    end_build "$PCRE_TAR"
fi

# Install git (requires curl and pcre)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/git.html
GIT_VERSION=2.17.0
GIT_TAR="git-${GIT_VERSION}.tar.xz"
GIT_SITE="https://www.kernel.org/pub/software/scm/git"
if [ ! -s "$SDK_HOME/bin/git" ] || [ "$("${SDK_HOME}/bin/git" --version)" != "git version $GIT_VERSION" ] ; then
    start_build "$GIT_TAR"
    download "$GIT_SITE" "$GIT_TAR"
    untar "$SRC_PATH/$GIT_TAR"
    pushd "git-${GIT_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "git-${GIT_VERSION}"
    end_build "$GIT_TAR"
fi

# Install MariaDB (for the Qt mariadb plugin and the python mariadb adapter)
# see http://www.linuxfromscratch.org/blfs/view/svn/server/mariadb.html
MARIADB_VERSION=10.2.12
MARIADB_TAR="mariadb-${MARIADB_VERSION}.tar.gz"
MARIADB_SITE="https://downloads.mariadb.org/interstitial/mariadb-${MARIADB_VERSION}/source"
if [ ! -s "$SDK_HOME/bin/mariadb_config" ] || [ "$("${SDK_HOME}/bin/mariadb_config" --version)" != "$MARIADB_VERSION" ] ; then
    start_build "$MARIADB_TAR"
    download "$MARIADB_SITE" "$MARIADB_TAR"
    untar "$SRC_PATH/$MARIADB_TAR"
    pushd "mariadb-${MARIADB_VERSION}"
    #env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    mkdir build-natron
    pushd build-natron
    # -DWITHOUT_SERVER=ON to disable building the server, since we only want the client
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DWITH_EXTRA_CHARSETS=complex -DSKIP_TESTS=ON -DWITHOUT_SERVER=ON
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "mariadb-${MARIADB_VERSION}"
    end_build "$MARIADB_TAR"
fi

# Install PostgreSQL (for the Qt postgresql plugin and the python postgresql adapter)
# see http://www.linuxfromscratch.org/blfs/view/svn/server/postgresql.html
POSTGRESQL_VERSION=10.4
POSTGRESQL_TAR="postgresql-${POSTGRESQL_VERSION}.tar.bz2"
POSTGRESQL_SITE="http://ftp.postgresql.org/pub/source/v${POSTGRESQL_VERSION}"
if [ ! -s "$SDK_HOME/lib/pkgconfig/libpq.pc" ] || [ "$(pkg-config --modversion libpq)" != "$POSTGRESQL_VERSION" ]; then
    start_build "$POSTGRESQL_TAR"
    download "$POSTGRESQL_SITE" "$POSTGRESQL_TAR"
    untar "$SRC_PATH/$POSTGRESQL_TAR"
    pushd "postgresql-${POSTGRESQL_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static --enable-thread-safety
    make -j${MKJOBS}
    make install
    popd
    rm -rf "postgresql-${POSTGRESQL_VERSION}"
    end_build "$POSTGRESQL_TAR"
fi

# Install Python2
# see http://www.linuxfromscratch.org/blfs/view/svn/general/python2.html
PY2_VERSION=2.7.15
PY2_VERSION_SHORT=${PY2_VERSION%.*}
PY2_TAR="Python-${PY2_VERSION}.tar.xz"
PY2_SITE="https://www.python.org/ftp/python/${PY2_VERSION}"
if [ ! -s "$SDK_HOME/lib/pkgconfig/python2.pc" ] || [ "$(pkg-config --modversion python2)" != "$PY2_VERSION_SHORT" ]; then
    start_build "$PY2_TAR"
    download "$PY2_SITE" "$PY2_TAR"
    untar "$SRC_PATH/$PY2_TAR"
    pushd "Python-${PY2_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-shared --with-system-expat --with-system-ffi --with-ensurepip=yes --enable-unicode=ucs4
    make -j${MKJOBS}
    make install
    chmod -v 755 "$SDK_HOME/lib/libpython2.7.so.1.0"
    popd
    rm -rf "Python-${PY2_VERSION}"
    end_build "$PY2_TAR"
fi

# Install mysqlclient (MySQL/MariaDB connector)
# mysqlclient is a fork of MySQL-python. It adds Python 3 support and fixed many bugs.
# see https://pypi.python.org/pypi/mysqlclient
MYSQLCLIENT_VERSION=1.3.12
if [ ! -d "$SDK_HOME/lib/python${PY2_VERSION_SHORT}/site-packages/MySQLdb" ] || [ $("$SDK_HOME/bin/python${PY2_VERSION_SHORT}" -c "import MySQLdb; print MySQLdb.__version__") != ${MYSQLCLIENT_VERSION} ]; then
    start_build "mysqlclient-$MYSQLCLIENT_VERSION"
    ${SDK_HOME}/bin/pip${PY2_VERSION_SHORT} install --no-binary mysqlclient mysqlclient
    end_build "mysqlclient-$MYSQLCLIENT_VERSION"
fi

# install psycopg2 (PostgreSQL connector)
# see https://pypi.python.org/pypi/psycopg2
PSYCOPG2_VERSION=2.7.3.2
if [ ! -d "$SDK_HOME/lib/python${PY2_VERSION_SHORT}/site-packages/psycopg2" ] || [ $("$SDK_HOME/bin/python${PY2_VERSION_SHORT}" -c "import psycopg2; print psycopg2.__version__.split(' ', 1)[0]") != ${PSYCOPG2_VERSION} ]; then
    start_build "psycopg2-$PSYCOPG2_VERSION"
    ${SDK_HOME}/bin/pip${PY2_VERSION_SHORT} install --no-binary psycopg2 psycopg2
    end_build "psycopg2-$PSYCOPG2_VERSION"
fi
    
# Install Python3
# see http://www.linuxfromscratch.org/blfs/view/svn/general/python3.html
PY3_VERSION=3.6.5
PY3_VERSION_SHORT=${PY3_VERSION%.*}
PY3_TAR="Python-${PY3_VERSION}.tar.xz"
PY3_SITE="https://www.python.org/ftp/python/${PY3_VERSION}"
if [ "$PYV" = "3" ]; then
    if [ ! -s "$SDK_HOME/lib/pkgconfig/python3.pc" ] || [ "$(pkg-config --modversion python3)" != "$PY3_VERSION_SHORT" ]; then
        start_build "$PY3_TAR"
        download "$PY3_SITE" "$PY3_TAR"
        untar "$SRC_PATH/$PY3_TAR"
        pushd "Python-${PY3_VERSION}"
        env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-shared --with-system-expat --with-system-ffi --with-ensurepip=yes --with-ensurepip=yes
        make -j${MKJOBS}
        make install
        chmod -v 755 /usr/lib/libpython${PY3_VERSION_SHORT}m.so
        chmod -v 755 /usr/lib/libpython3.so
        popd
        rm -rf "Python-${PY3_VERSION}"
        end_build "$PY3_TAR"
    fi
fi

# Install llvm+osmesa
OSMESA_VERSION=17.1.10
OSMESA_TAR="mesa-${OSMESA_VERSION}.tar.gz"
OSMESA_SITE="ftp://ftp.freedesktop.org/pub/mesa"
OSMESA_GLU_VERSION=9.0.0
OSMESA_GLU_TAR="glu-${OSMESA_GLU_VERSION}.tar.gz"
OSMESA_GLU_SITE="ftp://ftp.freedesktop.org/pub/mesa/glu"
OSMESA_DEMOS_VERSION=8.3.0
OSMESA_DEMOS_TAR="mesa-demos-${OSMESA_DEMOS_VERSION}.tar.bz2"
OSMESA_DEMOS_SITE="ftp://ftp.freedesktop.org/pub/mesa/demos/${OSMESA_DEMOS_VERSION}"
OSMESA_LLVM_VERSION=4.0.1
OSMESA_LLVM_TAR="llvm-${OSMESA_LLVM_VERSION}.src.tar.xz"
OSMESA_LLVM_SITE="http://releases.llvm.org/${OSMESA_LLVM_VERSION}"
if [ "${REBUILD_OSMESA:-}" = "1" ]; then
    rm -rf "$SDK_HOME/osmesa" || true
    rm -rf "$SDK_HOME/llvm" || true
fi
if [ ! -s "$SDK_HOME/osmesa/lib/pkgconfig/gl.pc" ] || [ "$(env PKG_CONFIG_PATH=$SDK_HOME/osmesa/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion gl)" != "$OSMESA_VERSION" ]; then
    start_build "$OSMESA_TAR"
    download "$OSMESA_SITE" "$OSMESA_TAR"
    download "$OSMESA_GLU_SITE" "$OSMESA_GLU_TAR"
    download "$OSMESA_DEMOS_SITE" "$OSMESA_DEMOS_TAR"
    mkdir -p "${SDK_HOME}/osmesa" || true
    LLVM_BUILD=0
    if [ ! -s "$SDK_HOME/llvm/bin/llvm-config" ] || [ "$($SDK_HOME/llvm/bin/llvm-config --version)" != "$OSMESA_LLVM_VERSION" ]; then
        LLVM_BUILD=1
        download "$OSMESA_LLVM_SITE" "$OSMESA_LLVM_TAR"
        mkdir -p "${SDK_HOME}/llvm" || true
    fi
    git clone https://github.com/devernay/osmesa-install
    pushd osmesa-install
    mkdir build
    pushd build
    cp "$SRC_PATH/$OSMESA_TAR" .
    cp "$SRC_PATH/$OSMESA_GLU_TAR" .
    cp "$SRC_PATH/$OSMESA_DEMOS_TAR" .
    if [ "$LLVM_BUILD" = 1 ]; then
       cp "$SRC_PATH/$OSMESA_LLVM_TAR" .
    fi
    env CFLAGS="-fPIC" OSMESA_DRIVER=3 OSMESA_PREFIX="${SDK_HOME}/osmesa" OSMESA_VERSION="$OSMESA_VERSION" LLVM_PREFIX="${SDK_HOME}/llvm" LLVM_VERSION="$OSMESA_LLVM_VERSION" MKJOBS="${MKJOBS}" LLVM_BUILD="$LLVM_BUILD" ../osmesa-install.sh
    popd
    popd
    rm -rf osmesa-install
    end_build "$OSMESA_TAR"
fi

# Install libpng
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/libpng.html
LIBPNG_VERSION=1.6.34
LIBPNG_TAR="libpng-${LIBPNG_VERSION}.tar.gz"
LIBPNG_SITE="https://download.sourceforge.net/libpng"
if [ "${REBUILD_PNG:-}" = "1" ]; then
    rm -rf "$SDK_HOME"/{include,lib}/*png* || true
    rm -f "$SDK_HOME"/lib/pkgconfig/*png* || true
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/libpng.pc" ] || [ "$(pkg-config --modversion libpng)" != "$LIBPNG_VERSION" ]; then
    start_build "$LIBPNG_TAR"
    download "$LIBPNG_SITE" "$LIBPNG_TAR"
    untar "$SRC_PATH/$LIBPNG_TAR"
    pushd "libpng-${LIBPNG_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" LIBS=-lpthread ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libpng-${LIBPNG_VERSION}"
    end_build "$LIBPNG_TAR"
fi

# Install FreeType2
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/freetype2.html
FTYPE_VERSION=2.9.1
FTYPE_TAR="freetype-${FTYPE_VERSION}.tar.gz"
FTYPE_SITE="http://download.savannah.gnu.org/releases/freetype"
if [ ! -s "$SDK_HOME/lib/pkgconfig/freetype2.pc" ]; then # || [ "$(pkg-config --modversion freetype2)" != "$FTYPE_VERSION" ]; then
    start_build "$FTYPE_TAR"
    download "$FTYPE_SITE" "$FTYPE_TAR"
    untar "$SRC_PATH/$FTYPE_TAR"
    pushd "freetype-${FTYPE_VERSION}"
    # First command enables GX/AAT and OpenType table validation
    sed -ri "s:.*(AUX_MODULES.*valid):\1:" modules.cfg
    # second command enables Subpixel Rendering
    # Note that Subpixel Rendering may have patent issues. Be sure to read the 'Other patent issues' part of http://www.freetype.org/patents.html before enabling this option.
    sed -r "s:.*(#.*SUBPIXEL_RENDERING) .*:\1:" -i include/freetype/config/ftoption.h
env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "freetype-${FTYPE_VERSION}"
    end_build "$FTYPE_TAR"
fi

# Install fontconfig
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/fontconfig.html
FONTCONFIG_VERSION=2.13.0
FONTCONFIG_TAR="fontconfig-${FONTCONFIG_VERSION}.tar.gz"
FONTCONFIG_SITE="https://www.freedesktop.org/software/fontconfig/release"
if [ "${REBUILD_FONTCONFIG:-}" = "1" ]; then
    rm -f "$SDK_HOME"/lib/libfontconfig* || true
    rm -rf "$SDK_HOME"/include/fontconfig || true
    rm -f "$SDK_HOME"/lib/pkgconfig/fontconfig.pc || true
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/fontconfig.pc" ] || [ "$(pkg-config --modversion fontconfig)" != "$FONTCONFIG_VERSION" ]; then
    start_build "$FONTCONFIG_TAR"
    download "$FONTCONFIG_SITE" "$FONTCONFIG_TAR"
    untar "$SRC_PATH/$FONTCONFIG_TAR"
    pushd "fontconfig-${FONTCONFIG_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "fontconfig-${FONTCONFIG_VERSION}"
    end_build "$FONTCONFIG_TAR"
fi

# Install libmount (required by glib)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/util-linux.html
if [ "${RHEL_MAJOR:-7}" -le 6 ]; then
    # 2.31.1 fails to compile on centos 6,
    # see https://www.spinics.net/lists/util-linux-ng/msg15044.html
    # and https://www.spinics.net/lists/util-linux-ng/msg13963.html
    UTILLINUX_VERSION=2.30.2
else
    UTILLINUX_VERSION=2.31.1
fi
UTILLINUX_VERSION_SHORT=${UTILLINUX_VERSION%.*}
UTILLINUX_TAR="util-linux-${UTILLINUX_VERSION}.tar.xz"
UTILLINUX_SITE="https://www.kernel.org/pub/linux/utils/util-linux/v${UTILLINUX_VERSION_SHORT}"
if [ ! -s "$SDK_HOME/lib/pkgconfig/mount.pc" ] || [ "$(pkg-config --modversion mount)" != "$UTILLINUX_VERSION" ]; then
    start_build "$UTILLINUX_TAR"
    download "$UTILLINUX_SITE" "$UTILLINUX_TAR"
    untar "$SRC_PATH/$UTILLINUX_TAR"
    pushd "util-linux-${UTILLINUX_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-chfn-chsh  \
            --disable-login      \
            --disable-nologin    \
            --disable-su         \
            --disable-setpriv    \
            --disable-runuser    \
            --disable-pylibmount \
            --disable-static     \
            --without-python     \
            --without-systemd    \
            --without-systemdsystemunitdir \
            --disable-wall \
            --disable-makeinstall-chown \
            --disable-makeinstall-setuid
    make -j${MKJOBS}
    make install
    popd
    rm -rf "util-linux-${UTILLINUX_VERSION}"
    end_build "$UTILLINUX_TAR"
fi

# Install Texinfo (for gdb)
# see http://www.linuxfromscratch.org/lfs/view/development/chapter06/texinfo.html
TEXINFO_VERSION=6.5
TEXINFO_TAR="texinfo-${TEXINFO_VERSION}.tar.gz"
TEXINFO_SITE="https://ftp.gnu.org/gnu/texinfo"
if [ ! -s "$SDK_HOME/bin/makeinfo" ]; then
    start_build "$TEXINFO_TAR"
    download "$TEXINFO_SITE" "$TEXINFO_TAR"
    untar "$SRC_PATH/$TEXINFO_TAR"
    pushd "texinfo-${TEXINFO_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "texinfo-${TEXINFO_VERSION}"
    end_build "$TEXINFO_TAR"
fi

# Install glib
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/glib2.html
# We explicitely disable SElinux, see https://github.com/NatronGitHub/Natron/issues/265
GLIB_VERSION=2.56.1
if ! grep -F F_SETPIPE_SZ /usr/include/linux/fcntl.h &>/dev/null; then
    GLIB_VERSION=2.54.3
fi
GLIB_VERSION_SHORT=${GLIB_VERSION%.*}
GLIB_TAR="glib-${GLIB_VERSION}.tar.xz"
GLIB_SITE="https://ftp.gnome.org/pub/gnome/sources/glib/${GLIB_VERSION_SHORT}"
if [ ! -s "$SDK_HOME/lib/pkgconfig/glib-2.0.pc" ] || [ "$(pkg-config --modversion glib-2.0)" != "$GLIB_VERSION" ]; then
    start_build "$GLIB_TAR"
    download "$GLIB_SITE" "$GLIB_TAR"
    untar "$SRC_PATH/$GLIB_TAR"
    pushd "glib-${GLIB_VERSION}"
    # apply patches from http://www.linuxfromscratch.org/blfs/view/cvs/general/glib2.html
    patch -Np1 -i "$INC_PATH/patches/glib/glib-2.56.0-skip_warnings-1.patch"
    # do not apply: we do not build with meson yet
    #patch -Np1 -i "$INC_PATH/patches/glib/glib-2.54.2-meson_fixes-1.patch"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-selinux --disable-gtk-doc-html --disable-static --enable-shared --with-pcre=system
    make -j${MKJOBS}
    make install
    popd
    rm -rf "glib-${GLIB_VERSION}"
    end_build "$GLIB_TAR"
fi

# Install libxml2
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libxml2.html
LIBXML2_VERSION=2.9.8
LIBXML2_TAR="libxml2-${LIBXML2_VERSION}.tar.gz"
LIBXML2_SITE="ftp://xmlsoft.org/libxml2"
LIBXML2_ICU=1 # set to 1 if libxml2 should be compiled with ICU support. This implies things for Qt4 and QtWebkit.
if [ "${REBUILD_XML:-}" = "1" ]; then
    rm -rf "$SDK_HOME"/include/libxml2 "$SDK_HOME"/lib/libxml* "$SDK_HOME"/lib/pkgconfig/libxml* || true
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/libxml-2.0.pc" ] || [ "$(pkg-config --modversion libxml-2.0)" != "$LIBXML2_VERSION" ]; then
    REBUILD_XSLT=1
    start_build "$LIBXML2_TAR"
    download "$LIBXML2_SITE" "$LIBXML2_TAR"
    untar "$SRC_PATH/$LIBXML2_TAR"
    pushd "libxml2-${LIBXML2_VERSION}"
    if [ "$LIBXML2_ICU" -eq 1 ]; then
        icu_flag=--with-icu
    else
        icu_flag=--without-icu
    fi
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --with-history $icu_flag --without-python --with-lzma --with-threads
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libxml2-${LIBXML2_VERSION}"
    end_build "$LIBXML2_TAR"
fi

# Install libxslt
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/libxslt.html
LIBXSLT_VERSION=1.1.32
LIBXSLT_TAR="libxslt-${LIBXSLT_VERSION}.tar.gz"
LIBXSLT_SITE="ftp://xmlsoft.org/libxslt"
if [ "${REBUILD_XSLT:-}" = "1" ]; then
    rm -rf "$SDK_HOME"/include/libxslt "$SDK_HOME"/lib/libxsl* "$SDK_HOME"/lib/pkgconfig/libxslt* || true
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/libxslt.pc" ] || [ "$(pkg-config --modversion libxslt)" != "$LIBXSLT_VERSION" ]; then
    start_build "$LIBXSLT_TAR"
    download "$LIBXSLT_SITE" "$LIBXSLT_TAR"
    untar "$SRC_PATH/$LIBXSLT_TAR"
    pushd "libxslt-${LIBXSLT_VERSION}"
    # this increases the recursion limit in libxslt. This is needed by some packages for their documentation
    sed -i s/3000/5000/ libxslt/transform.c doc/xsltproc.{1,xml}
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --without-python
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libxslt-${LIBXSLT_VERSION}"
    end_build "$LIBXSLT_TAR"
fi

# Install boost
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/boost.html
# TestImagePNG and TestImagePNGOIIO used to fail with boost 1.66.0, and succeed with boost 1.65.1,
# but this should be fixed by https://github.com/NatronGitHub/Natron/commit/e360b4dd31a07147f2ad9073a773ff28714c2a69
BOOST_VERSION=1.67.0
BOOST_LIB_VERSION=$(echo "${BOOST_VERSION//./_}" | sed -e 's/_0$//')
BOOST_TAR="boost_${BOOST_VERSION//./_}.tar.bz2"
BOOST_SITE="https://dl.bintray.com/boostorg/release/${BOOST_VERSION}/source"
if [ ! -s "$SDK_HOME/lib/libboost_atomic.so" ] || ! fgrep "define BOOST_LIB_VERSION \"${BOOST_LIB_VERSION}\"" "$SDK_HOME/include/boost/version.hpp" &>/dev/null ; then
    start_build "$BOOST_TAR"
    download "$BOOST_SITE" "$BOOST_TAR"
    untar "$SRC_PATH/$BOOST_TAR"
    pushd "boost_${BOOST_VERSION//./_}"
    # fix a bug with the header files path, when Python3 is used:
    sed -e '/using python/ s@;@: /usr/include/python${PYTHON_VERSION/3*/${PYTHON_VERSION}m} ;@' -i bootstrap.sh
    env CPATH="$SDK_HOME/include:$SDK_HOME/include/python${PY2_VERSION_SHORT}" ./bootstrap.sh --prefix="$SDK_HOME"
    env CPATH="$SDK_HOME/include:$SDK_HOME/include/python${PY2_VERSION_SHORT}" CFLAGS="$BF" CXXFLAGS="$BF" ./b2 -s --prefix="$SDK_HOME" cflags="-fPIC" -j${MKJOBS} install threading=multi link=shared # link=static
    popd
    rm -rf "boost_${BOOST_VERSION//./_}"
    end_build "$BOOST_TAR"
fi

# install cppunit
CPPU_VERSION=1.13.2 # does not require c++11
if [[ ! "$GCC_VERSION" =~ ^4\. ]]; then
    CPPU_VERSION=1.14.0 # 1.14.0 is the first version to require c++11
fi
CPPU_TAR="cppunit-${CPPU_VERSION}.tar.gz"
CPPU_SITE="http://dev-www.libreoffice.org/src"
if [ ! -s "$SDK_HOME/lib/pkgconfig/cppunit.pc" ] || [ "$(pkg-config --modversion cppunit)" != "$CPPU_VERSION" ]; then
    start_build "$CPPU_TAR"
    download "$CPPU_SITE" "$CPPU_TAR"
    untar "$SRC_PATH/$CPPU_TAR"
    pushd "cppunit-${CPPU_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "cppunit-${CPPU_VERSION}"
    end_build "$CPPU_TAR"
fi

# Install libjpeg-turbo
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libjpeg.html
LIBJPEGTURBO_VERSION=1.5.3
LIBJPEGTURBO_TAR="libjpeg-turbo-${LIBJPEGTURBO_VERSION}.tar.gz"
LIBJPEGTURBO_SITE="https://sourceforge.net/projects/libjpeg-turbo/files/${LIBJPEGTURBO_VERSION}"
if [ ! -s "$SDK_HOME/lib/libturbojpeg.so.0.1.0" ]; then
    start_build "$LIBJPEGTURBO_TAR"
    download "$LIBJPEGTURBO_SITE" "$LIBJPEGTURBO_TAR"
    untar "$SRC_PATH/$LIBJPEGTURBO_TAR"
    pushd "libjpeg-turbo-${LIBJPEGTURBO_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --with-jpeg8 --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libjpeg-turbo-${LIBJPEGTURBO_VERSION}"
    end_build "$LIBJPEGTURBO_TAR"
fi

# Install giflib
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/giflib.html
GIFLIB_VERSION=5.1.4
GIFLIB_TAR="giflib-${GIFLIB_VERSION}.tar.bz2"
GIFLIB_SITE="https://sourceforge.net/projects/giflib/files"
if [ ! -s "$SDK_HOME/lib/libgif.so" ]; then
    start_build "$GIFLIB_TAR"
    download "$GIFLIB_SITE" "$GIFLIB_TAR"
    untar "$SRC_PATH/$GIFLIB_TAR"
    pushd "giflib-${GIFLIB_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "giflib-${GIFLIB_VERSION}"
    end_build "$GIFLIB_TAR"
fi

# Install tiff
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libtiff.html
TIFF_VERSION=4.0.9
TIFF_TAR="tiff-${TIFF_VERSION}.tar.gz"
TIFF_SITE="http://download.osgeo.org/libtiff"
if [ "${REBUILD_TIFF:-}" = "1" ]; then
    rm -rf "$SDK_HOME"/{lib,include}/*tiff* || true
    rm -f "$SDK_HOME"/lib/pkgconfig/*tiff* || true
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/libtiff-4.pc" ] || [ "$(pkg-config --modversion libtiff-4)" != "$TIFF_VERSION" ]; then
    start_build "$TIFF_TAR"
    download "$TIFF_SITE" "$TIFF_TAR"
    untar "$SRC_PATH/$TIFF_TAR"
    pushd "tiff-${TIFF_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "tiff-${TIFF_VERSION}"
    end_build "$TIFF_TAR"
fi

# Install jasper
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/jasper.html
JASPER_VERSION=2.0.14
JASPER_TAR="jasper-${JASPER_VERSION}.tar.gz"
JASPER_SITE="http://www.ece.uvic.ca/~mdadams/jasper/software"
if [ "${REBUILD_JASPER:-}" = "1" ]; then
    rm -rf "$SDK_HOME"/{include,lib}/*jasper* || true
fi
if [ ! -s "$SDK_HOME/lib/libjasper.so" ]; then
    start_build "$JASPER_TAR"
    download "$JASPER_SITE" "$JASPER_TAR"
    untar "$SRC_PATH/$JASPER_TAR"
    pushd "jasper-${JASPER_VERSION}"
    #env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    mkdir build-natron
    pushd build-natron
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    make -j${MKJOBS}
    make install
    popd
    popd
    rm -rf "jasper-${JASPER_VERSION}"
    end_build "$JASPER_TAR"
fi

# Install lcms2
# see www.linuxfromscratch.org/blfs/view/cvs/general/lcms2.html
LCMS_VERSION=2.9
LCMS_TAR="lcms2-${LCMS_VERSION}.tar.gz"
LCMS_SITE="https://sourceforge.net/projects/lcms/files/lcms/${LCMS_VERSION}"
if [ ! -s "$SDK_HOME/lib/pkgconfig/lcms2.pc" ] || [ "$(pkg-config --modversion lcms2)" != "$LCMS_VERSION" ]; then
    start_build "$LCMS_TAR"
    download "$LCMS_SITE" "$LCMS_TAR"
    untar "$SRC_PATH/$LCMS_TAR"
    pushd "lcms2-${LCMS_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "lcms2-${LCMS_VERSION}"
    end_build "$LCMS_TAR"
fi

# install librevenge
REVENGE_VERSION=0.0.4
REVENGE_TAR="librevenge-${REVENGE_VERSION}.tar.xz"
REVENGE_SITE="https://sourceforge.net/projects/libwpd/files/librevenge/librevenge-${REVENGE_VERSION}"
if [ ! -s "$SDK_HOME/lib/pkgconfig/librevenge-0.0.pc" ] || [ "$(pkg-config --modversion librevenge-0.0)" != "$REVENGE_VERSION" ]; then
    start_build "$REVENGE_TAR"
    download "$REVENGE_SITE" "$REVENGE_TAR"
    untar "$SRC_PATH/$REVENGE_TAR"
    pushd "librevenge-${REVENGE_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --disable-werror --prefix="$SDK_HOME" --disable-docs --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "librevenge-${REVENGE_VERSION}"
    end_build "$REVENGE_TAR"
fi

# install libcdr
LIBCDR_VERSION=0.1.4
LIBCDR_TAR="libcdr-${LIBCDR_VERSION}.tar.xz"
LIBCDR_SITE="http://dev-www.libreoffice.org/src/libcdr"
if [ ! -s "$SDK_HOME/lib/pkgconfig/libcdr-0.1.pc" ] || [ "$(pkg-config --modversion libcdr-0.1)" != "$LIBCDR_VERSION" ]; then
    start_build "$LIBCDR_TAR"
    download "$LIBCDR_SITE" "$LIBCDR_TAR"
    untar "$SRC_PATH/$LIBCDR_TAR"
    pushd "libcdr-${LIBCDR_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --disable-werror --prefix="$SDK_HOME" --disable-docs --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libcdr-${LIBCDR_VERSION}"
    end_build "$LIBCDR_TAR"
fi

# Install openjpeg
# see http://www.at.linuxfromscratch.org/blfs/view/cvs/general/openjpeg2.html
OPENJPEG2_VERSION=2.3.0
OPENJPEG2_VERSION_SHORT=${OPENJPEG2_VERSION%.*}
OPENJPEG2_TAR="openjpeg-${OPENJPEG2_VERSION}.tar.gz"
if [ "${REBUILD_OPENJPEG:-}" = "1" ]; then
    rm -rf "$SDK_HOME"/lib/libopenjp* || true
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/libopenjp2.pc" ] || [ "$(pkg-config --modversion libopenjp2)" != "$OPENJPEG2_VERSION" ]; then
    start_build "$OPENJPEG2_TAR"
    download_github uclouvain openjpeg "${OPENJPEG2_VERSION}" v "${OPENJPEG2_TAR}"
    untar "$SRC_PATH/$OPENJPEG2_TAR"
    pushd "openjpeg-${OPENJPEG2_VERSION}"
    mkdir build
    pushd build
    cmake .. -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF"  -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    make -j${MKJOBS}
    make install
    # ffmpeg < 3.4 looks for openjpeg 2.1
    #if [ ! -e "${SDK_HOME}/include/openjpeg-2.1" ]; then
    #    ln -sfv openjpeg-2.3 "${SDK_HOME}/include/openjpeg-2.1"
    #fi
    popd
    popd
    rm -rf "openjpeg-${OPENJPEG2_VERSION}"
    end_build "$OPENJPEG2_TAR"
fi


# Install libraw
# see http://www.at.linuxfromscratch.org/blfs/view/cvs/general/libraw.html
LIBRAW_VERSION=0.18.12
LIBRAW_PACKS_VERSION="${LIBRAW_VERSION}"
LIBRAW_PACKS_VERSION=0.18.8
LIBRAW_TAR="LibRaw-${LIBRAW_VERSION}.tar.gz"
LIBRAW_DEMOSAIC_PACK_GPL2="LibRaw-demosaic-pack-GPL2-${LIBRAW_PACKS_VERSION}.tar.gz"
LIBRAW_DEMOSAIC_PACK_GPL3="LibRaw-demosaic-pack-GPL3-${LIBRAW_PACKS_VERSION}.tar.gz"
LIBRAW_SITE="https://www.libraw.org/data"
if [ "${REBUILD_LIBRAW:-}" = "1" ]; then
    rm -rf "$SDK_HOME"/libraw-gpl3 || true
    rm -rf "$SDK_HOME"/libraw-gpl2 || true
    rm -rf "$SDK_HOME"/libraw-lgpl || true
fi
if [ ! -s "$SDK_HOME/libraw-gpl2/lib/pkgconfig/libraw.pc" ] || [ "$(env PKG_CONFIG_PATH=$SDK_HOME/libraw-gpl2/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion libraw)" != "$LIBRAW_VERSION" ]; then
    start_build "$LIBRAW_TAR"
    download "$LIBRAW_SITE" "$LIBRAW_TAR"
    download "$LIBRAW_SITE" "$LIBRAW_DEMOSAIC_PACK_GPL2"
    download "$LIBRAW_SITE" "$LIBRAW_DEMOSAIC_PACK_GPL3"
    untar "$SRC_PATH/$LIBRAW_TAR"
    pushd "LibRaw-${LIBRAW_VERSION}"
    untar "$SRC_PATH/$LIBRAW_DEMOSAIC_PACK_GPL2"
    untar "$SRC_PATH/$LIBRAW_DEMOSAIC_PACK_GPL3"
    LGPL_CONF_FLAGS=( --disable-static --enable-shared --enable-lcms --enable-jasper --enable-jpeg )
    GPL2_CONF_FLAGS=( "${LGPL_CONF_FLAGS[@]}" --enable-demosaic-pack-gpl2 --disable-demosaic-pack-gpl3 )
    GPL3_CONF_FLAGS=( "${LGPL_CONF_FLAGS[@]}" --enable-demosaic-pack-gpl2 --enable-demosaic-pack-gpl3 )
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS" ./configure --prefix="$SDK_HOME/libraw-gpl3" "${GPL3_CONF_FLAGS[@]}"
    make -j${MKJOBS}
    make install
    make distclean
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS" ./configure --prefix="$SDK_HOME/libraw-gpl2" "${GPL2_CONF_FLAGS[@]}"
    make -j${MKJOBS}
    make install
    make distclean
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS" ./configure --prefix="$SDK_HOME/libraw-lgpl" "${LGPL_CONF_FLAGS[@]}"
    make -j${MKJOBS}
    make install
    popd
    rm -rf "LibRaw-${LIBRAW_VERSION}"
    end_build "$LIBRAW_TAR"
fi

# Install openexr
EXR_VERSION=2.2.1
EXR_ILM_TAR="ilmbase-${EXR_VERSION}.tar.gz"
EXR_TAR="openexr-${EXR_VERSION}.tar.gz"
EXR_SITE="http://download.savannah.nongnu.org/releases/openexr"
if [ "${REBUILD_EXR:-}" = "1" ]; then
    rm -rf "$SDK_HOME"/lib/libI* "$SDK_HOME"/lib/libHalf* || true
    rm -f "$SDK_HOME"/lib/pkgconfig/{OpenEXR,IlmBase}.pc || true
    rm -f "$SDK_HOME"/bin/exr* || true
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/IlmBase.pc" ] || [ "$(pkg-config --modversion IlmBase)" != "$EXR_VERSION" ]; then
    start_build "$EXR_ILM_TAR"
    download "$EXR_SITE" "$EXR_ILM_TAR"
    untar "$SRC_PATH/$EXR_ILM_TAR"
    pushd "ilmbase-${EXR_VERSION}"
    patch -p2 -i "$INC_PATH/patches/IlmBase/ilmbase-2.2.0-threadpool_release_lock_single_thread.patch"
    # https://github.com/lgritz/openexr/tree/lg-register
    patch -p2 -i "$INC_PATH/patches/IlmBase/ilmbase-2.2.0-lg-register.patch"
    # https://github.com/lgritz/openexr/tree/lg-cpp11
    patch -p2 -i "$INC_PATH/patches/IlmBase/ilmbase-2.2.0-lg-c++11.patch"
    #  mkdir build; cd build
    #cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF -I${SDK_HOME}/include/OpenEXR" -DCMAKE_SHARED_LINKER_FLAGS="-L${SDK_HOME}/lib" -DCMAKE_C_COMPILER="$SDK_HOME/gcc/bin/gcc" -DCMAKE_CXX_COMPILER="${SDK_HOME}/gcc/bin/g++" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DBUILD_SHARED_LIBS=ON -DNAMESPACE_VERSIONING=ON -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    cp IlmBase.pc "$SDK_HOME/lib/pkgconfig/"
    popd
    rm -rf "ilmbase-${EXR_VERSION}"
    end_build "$EXR_ILM_TAR"
fi

if [ ! -s "$SDK_HOME/lib/pkgconfig/OpenEXR.pc" ] || [ "$(pkg-config --modversion OpenEXR)" != "$EXR_VERSION" ]; then    
    start_build "$EXR_TAR"
    download "$EXR_SITE" "$EXR_TAR"
    untar "$SRC_PATH/$EXR_TAR"
    pushd "openexr-${EXR_VERSION}"
    env PATH="$SDK_HOME/bin:$PATH" ./bootstrap
    #mkdir build;cd build
    # cmake .. -DCMAKE_C_FLAGS="$BF" -DCMAKE_CXX_FLAGS="$BF -I${SDK_HOME}/include/OpenEXR" -DCMAKE_LIBRARY_PATH="${SDK_HOME}/lib" -DCMAKE_SHARED_LINKER_FLAGS="-L${SDK_HOME}/lib" -DCMAKE_EXE_LINKER_FLAGS="-L${SDK_HOME}/lib" -DCMAKE_C_COMPILER="$SDK_HOME/gcc/bin/gcc" -DCMAKE_CXX_COMPILER="${SDK_HOME}/gcc/bin/g++"  -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE" -DCMAKE_INSTALL_PREFIX="$SDK_HOME" -DBUILD_SHARED_LIBS=ON -DNAMESPACE_VERSIONING=ON
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static --disable-debug --disable-dependency-tracking
    make -j${MKJOBS}
    make install
    popd
    rm -rf "openexr-${EXR_VERSION}"
    end_build "$EXR_TAR"
fi

# Install pixman
# see http://www.at.linuxfromscratch.org/blfs/view/cvs/general/pixman.html
PIX_VERSION=0.34.0
PIX_TAR="pixman-${PIX_VERSION}.tar.bz2"
PIX_SITE="https://www.cairographics.org/releases"
if [ "${REBUILD_PIXMAN:-}" = "1" ]; then
    rm -rf "$SDK_HOME"/{lib,include}/*pixman* || true
    rm -f "$SDK_HOME"/lib/pkgconfig/*pixman* || true
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/pixman-1.pc" ] || [ "$(pkg-config --modversion pixman-1)" != "$PIX_VERSION" ]; then
    REBUILD_CAIRO=1
    start_build "$PIX_TAR"
    download "$PIX_SITE" "$PIX_TAR"
    untar "$SRC_PATH/$PIX_TAR"
    pushd "pixman-${PIX_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "pixman-${PIX_VERSION}"
    end_build "$PIX_TAR"
fi

# Install cairo
# see http://www.linuxfromscratch.org/blfs/view/svn/x/cairo.html
CAIRO_VERSION=1.14.12
CAIRO_TAR="cairo-${CAIRO_VERSION}.tar.xz"
CAIRO_SITE="https://www.cairographics.org/releases"
if [ "${REBUILD_CAIRO:-}" = "1" ]; then
    rm -rf "$SDK_HOME"/include/cairo || true
    rm -f "$SDK_HOME"/lib/pkgconfig/{cairo-*.pc,cairo.pc} || true
    rm -f "$SDK_HOME"/lib/libcairo* || true
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/cairo.pc" ] || [ "$(pkg-config --modversion cairo)" != "$CAIRO_VERSION" ]; then
    REBUILD_PANGO=1
    REBUILD_SVG=1
    REBUILD_BUZZ=1
    REBUILD_POPPLER=1
    start_build "$CAIRO_TAR"
    download "$CAIRO_SITE" "$CAIRO_TAR"
    untar "$SRC_PATH/$CAIRO_TAR"
    pushd "cairo-${CAIRO_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" CPPFLAGS="-I${SDK_HOME}/include/pixman-1" LDFLAGS="-L${SDK_HOME}/lib -lpixman-1" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --with-libpng=yes --enable-shared --enable-static --enable-tee
    make -j${MKJOBS}
    make install
    popd
    rm -rf "cairo-${CAIRO_VERSION}"
    end_build "$CAIRO_TAR"
fi

# Install harbuzz
# see http://www.linuxfromscratch.org/blfs/view/svn/general/harfbuzz.html
HARFBUZZ_VERSION=1.8.1
HARFBUZZ_TAR="harfbuzz-${HARFBUZZ_VERSION}.tar.bz2"
HARFBUZZ_SITE="https://www.freedesktop.org/software/harfbuzz/release"
if [ "${REBUILD_HARFBUZZ:-}" = "1" ]; then
    rm -rf "$SDK_HOME/lib/libharfbuzz*" "$SDK_HOME/include/harfbuzz" "$SDK_HOME/lib/pkgconfig/"*harfbuzz* || true
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/harfbuzz.pc" ] || [ "$(pkg-config --modversion harfbuzz)" != "$HARFBUZZ_VERSION" ]; then
    REBUILD_PANGO=1
    start_build "$HARFBUZZ_TAR"
    download "$HARFBUZZ_SITE" "$HARFBUZZ_TAR"
    untar "$SRC_PATH/$HARFBUZZ_TAR"
    pushd "harfbuzz-${HARFBUZZ_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --disable-docs --disable-static --enable-shared --with-freetype --with-cairo --with-gobject --with-glib --with-icu
    make -j${MKJOBS}
    make install
    popd
    rm -rf "harfbuzz-${HARFBUZZ_VERSION}"
    end_build "$HARFBUZZ_TAR"
fi


# Install pango
# see http://www.linuxfromscratch.org/blfs/view/svn/x/pango.html
PANGO_VERSION=1.42.1
PANGO_VERSION_SHORT=${PANGO_VERSION%.*}
PANGO_TAR="pango-${PANGO_VERSION}.tar.xz"
PANGO_SITE="http://ftp.gnome.org/pub/GNOME/sources/pango/${PANGO_VERSION_SHORT}"
if [ "${REBUILD_PANGO:-}" = "1" ]; then
    rm -rf $SDK_HOME/include/pango* $SDK_HOME/lib/libpango* $SDK_HOME/lib/pkgconfig/pango* || true
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/pango.pc" ] || [ "$(pkg-config --modversion pango)" != "$PANGO_VERSION" ]; then
    REBUILD_SVG=1
    start_build "$PANGO_TAR"
    download "$PANGO_SITE" "$PANGO_TAR"
    untar "$SRC_PATH/$PANGO_TAR"
    pushd "pango-${PANGO_VERSION}"
    env FONTCONFIG_LIBS="-lfontconfig" CFLAGS="$BF -g" CXXFLAGS="$BF -g" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --with-included-modules=basic-fc
    make -j${MKJOBS}
    make install
    popd
    rm -rf "pango-${PANGO_VERSION}"
    end_build "$PANGO_TAR"
fi

# Install libcroco (requires glib and libxml2)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/libcroco.html
CROCO_VERSION=0.6.12
CROCO_VERSION_SHORT=${CROCO_VERSION%.*}
CROCO_TAR="libcroco-${CROCO_VERSION}.tar.xz"
CROCO_SITE="http://ftp.gnome.org/pub/gnome/sources/libcroco/${CROCO_VERSION_SHORT}"
if [ ! -s "$SDK_HOME/lib/libcroco-0.6.so" ]; then
    start_build "$CROCO_TAR"
    download "$CROCO_SITE" "$CROCO_TAR"
    untar "$SRC_PATH/$CROCO_TAR"
    pushd "libcroco-${CROCO_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libcroco-${CROCO_VERSION}"
    end_build "$CROCO_TAR"
fi

# Install shared-mime-info (required by gdk-pixbuf)
# see http://www.linuxfromscratch.org/blfs/view/svn/general/shared-mime-info.html
MIME_VERSION=1.9
MIME_TAR="shared-mime-info-${MIME_VERSION}.tar.xz"
MIME_SITE="http://freedesktop.org/~hadess"
if [ ! -s "$SDK_HOME/share/pkgconfig/shared-mime-info.pc" ] || [ "$(pkg-config --modversion shared-mime-info)" != "$MIME_VERSION" ]; then
    start_build "$MIME_TAR"
    download "$MIME_SITE" "$MIME_TAR"
    untar "$SRC_PATH/$MIME_TAR"
    pushd "shared-mime-info-${MIME_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --without-libtiff
    #make -j${MKJOBS}
    make
    make install
    popd
    rm -rf "shared-mime-info-${MIME_VERSION}"
    end_build "$MIME_TAR"
fi

# Install gdk-pixbuf
# see http://www.linuxfromscratch.org/blfs/view/svn/x/gdk-pixbuf.html
GDKPIXBUF_VERSION=2.36.11
GDKPIXBUF_VERSION_SHORT=${GDKPIXBUF_VERSION%.*}
GDKPIXBUF_TAR="gdk-pixbuf-${GDKPIXBUF_VERSION}.tar.xz"
GDKPIXBUF_SITE="http://ftp.gnome.org/pub/gnome/sources/gdk-pixbuf/${GDKPIXBUF_VERSION_SHORT}"
if [ ! -s "$SDK_HOME/lib/pkgconfig/gdk-pixbuf-2.0.pc" ] || [ "$(pkg-config --modversion gdk-pixbuf-2.0)" != "$GDKPIXBUF_VERSION" ]; then
    start_build "$GDKPIXBUF_TAR"
    download "$GDKPIXBUF_SITE" "$GDKPIXBUF_TAR"
    untar "$SRC_PATH/$GDKPIXBUF_TAR"
    pushd "gdk-pixbuf-${GDKPIXBUF_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --without-libtiff
    make -j${MKJOBS}
    make install
    popd
    rm -rf "gdk-pixbuf-${GDKPIXBUF_VERSION}"
    end_build "$GDKPIXBUF_TAR"
fi

# Install librsvg (without vala support)
# see http://www.linuxfromscratch.org/blfs/view/systemd/general/librsvg.html
LIBRSVG_VERSION=2.40.20 # 2.41 requires rust
LIBRSVG_VERSION_SHORT=${LIBRSVG_VERSION%.*}
LIBRSVG_TAR="librsvg-${LIBRSVG_VERSION}.tar.xz"
LIBRSVG_SITE="https://download.gnome.org/sources/librsvg/${LIBRSVG_VERSION_SHORT}"
if [ "${REBUILD_SVG:-}" = "1" ]; then
    rm -rf "$SDK_HOME"/include/librsvg* "$SDK_HOME"/lib/librsvg* "$SDK_HOME"/lib/pkgconfig/librsvg* || true
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/librsvg-2.0.pc" ] || [ "$(pkg-config --modversion librsvg-2.0)" != "$LIBRSVG_VERSION" ]; then
    start_build "$LIBRSVG_TAR"
    if [ "$LIBRSVG_VERSION_SHORT" = 2.41 ]; then
        # librsvg 2.41 requires rust
        if [ ! -s "$HOME/.cargo/env" ]; then
            echo "Error: librsvg requires rust. Please install rust by executing:"
            echo "$SDK_HOME/bin/curl https://sh.rustup.rs -sSf | sh"
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
    end_build "$LIBRSVG_TAR"
fi

# Install fftw (GPLv2, for openfx-gmic)
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/fftw.html
FFTW_VERSION=3.3.8
FFTW_TAR="fftw-${FFTW_VERSION}.tar.gz"
FFTW_SITE="http://www.fftw.org/"
if [ ! -s "$SDK_HOME/lib/pkgconfig/fftw3.pc" ] || [ "$(pkg-config --modversion fftw3)" != "$FFTW_VERSION" ]; then
    start_build "$FFTW_TAR"
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
    end_build "$FFTW_TAR"
fi


# Install ImageMagick6
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/imagemagick6.html
MAGICK_VERSION=6.9.10-1
MAGICK_VERSION_SHORT=${MAGICK_VERSION%-*}
MAGICK_TAR="ImageMagick-${MAGICK_VERSION}.tar.xz"
MAGICK_SITE="https://www.imagemagick.org/download/releases"
if [ "${REBUILD_MAGICK:-}" = "1" ]; then
    rm -rf "$SDK_HOME"/include/ImageMagick-6/ "$SDK_HOME"/lib/libMagick* "$SDK_HOME"/share/ImageMagick-6/ "$SDK_HOME"/lib/pkgconfig/{Image,Magick}* "$SDK_HOME"/magick7 || true
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/Magick++.pc" ] || [ "$(pkg-config --modversion Magick++)" != "$MAGICK_VERSION_SHORT" ]; then
    start_build "$MAGICK_TAR"
    download "$MAGICK_SITE" "$MAGICK_TAR"
    untar "$SRC_PATH/$MAGICK_TAR"
    pushd "ImageMagick-${MAGICK_VERSION}"
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
    rm -rf "ImageMagick-${MAGICK_VERSION}"
    end_build "$MAGICK_TAR"
fi
# install ImageMagick7
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/imagemagick.html
MAGICK7_VERSION=7.0.8-1
MAGICK7_VERSION_SHORT=${MAGICK7_VERSION%-*}
MAGICK7_TAR="ImageMagick-${MAGICK7_VERSION}.tar.xz"
if [ ! -s "$SDK_HOME/magick7/lib/pkgconfig/Magick++.pc" ] || [ "$(env PKG_CONFIG_PATH=$SDK_HOME/magick7/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion Magick++)" != "$MAGICK7_VERSION_SHORT" ]; then
    start_build "$MAGICK7_TAR"
    download "$MAGICK_SITE" "$MAGICK7_TAR"
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
    end_build "$MAGICK7_TAR"
fi

# Install glew
GLEW_VERSION=2.1.0
GLEW_TAR="glew-${GLEW_VERSION}.tgz"
GLEW_SITE="https://sourceforge.net/projects/glew/files/glew/${GLEW_VERSION}"
if [ ! -s "$SDK_HOME/lib/pkgconfig/glew.pc" ] || [ "$(pkg-config --modversion glew)" != "$GLEW_VERSION" ]; then
    start_build "$GLEW_TAR"
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
    end_build "$GLEW_TAR"
fi

# Install poppler-glib (without curl, nss3, qt4, qt5)
# see http://www.linuxfromscratch.org/blfs/view/stable/general/poppler.html
POPPLER_VERSION=0.66.0
POPPLER_TAR="poppler-${POPPLER_VERSION}.tar.xz"
POPPLER_SITE="https://poppler.freedesktop.org"
if [ "${REBUILD_POPPLER:-}" = "1" ]; then
    rm -rf "$SDK_HOME/include/poppler" "$SDK_HOME/lib/"libpoppler* "$SDK_HOME/lib/pkgconfig/"*poppler* || true 
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/poppler-glib.pc" ] || [ "$(pkg-config --modversion poppler-glib)" != "$POPPLER_VERSION" ]; then
    start_build "$POPPLER_TAR"
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
    end_build "$POPPLER_TAR"
fi

# Install poppler-data
POPPLERD_VERSION=0.4.9
POPPLERD_TAR="poppler-data-${POPPLERD_VERSION}.tar.gz"
if [ ! -d "$SDK_HOME/share/poppler" ]; then
    start_build "$POPPLERD_TAR"
    download "$POPPLER_SITE" "$POPPLERD_TAR"
    untar "$SRC_PATH/$POPPLERD_TAR"
    pushd "poppler-data-${POPPLERD_VERSION}"
    make install datadir="${SDK_HOME}/share"
    popd
    rm -rf "poppler-data-${POPPLERD_VERSION}"
    end_build "$POPPLERD_TAR"
fi

# Install tinyxml (used by OpenColorIO)
TINYXML_VERSION=2.6.2
TINYXML_TAR="tinyxml_${TINYXML_VERSION//./_}.tar.gz"
TINYXML_SITE="http://sourceforge.net/projects/tinyxml/files/tinyxml/${TINYXML_VERSION}"
if [ ! -s "$SDK_HOME/lib/pkgconfig/tinyxml.pc" ] || [ "$(pkg-config --modversion tinyxml)" != "$TINYXML_VERSION" ]; then
    start_build "$TINYXML_TAR"
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
    end_build "$TINYXML_TAR"
fi

# Install yaml-cpp (0.5.3 requires boost, 0.6+ requires C++11, used by OpenColorIO)
YAMLCPP_VERSION=0.5.3
if [[ ! "$GCC_VERSION" =~ ^4\. ]]; then
    YAMLCPP_VERSION=0.6.2 # 0.6.0 is the first version to require C++11
fi
YAMLCPP_VERSION_SHORT=${YAMLCPP_VERSION%.*}
YAMLCPP_TAR="yaml-cpp-${YAMLCPP_VERSION}.tar.gz"
if [ ! -s "$SDK_HOME/lib/pkgconfig/yaml-cpp.pc" ] || [ "$(pkg-config --modversion yaml-cpp)" != "$YAMLCPP_VERSION" ]; then
    start_build "$YAMLCPP_TAR"
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
    end_build "$YAMLCPP_TAR"
fi

# Install OpenColorIO (uses yaml-cpp, tinyxml, lcms)
# We build a version more recent than 1.0.9, because 1.0.9 only supports yaml-cpp 0.3 and we
# installed yaml-cpp 0.5.
OCIO_BUILD_GIT=0 # set to 1 to build the git version instead of the release
# non-GIT version:
OCIO_VERSION=1.1.0
OCIO_TAR="OpenColorIO-${OCIO_VERSION}.tar.gz"
## GIT version:
#OCIO_GIT="https://github.com/imageworks/OpenColorIO.git"
## 7bd4b1e556e6c98c0aa353d5ecdf711bb272c4fa is October 25, 2017
#OCIO_COMMIT="7bd4b1e556e6c98c0aa353d5ecdf711bb272c4fa"
if [ ! -s "$SDK_HOME/lib/pkgconfig/OpenColorIO.pc" ] || [ "$(pkg-config --modversion OpenColorIO)" != "$OCIO_VERSION" ]; then
    if [ "$OCIO_BUILD_GIT" = 1 ]; then
        start_build "OpenColorIO-$OCIO_COMMIT"
        git_clone_commit "$OCIO_GIT" "$OCIO_COMMIT"
        pushd OpenColorIO
    else
        start_build "$OCIO_TAR"
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
    if [ "$OCIO_BUILD_GIT" = 1 ]; then
        end_build "OpenColorIO-$OCIO_COMMIT"
    else
        end_build "$OCIO_TAR"
    fi
fi

# Install webp (for OIIO)
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/libwebp.html
WEBP_VERSION=1.0.0
WEBP_TAR="libwebp-${WEBP_VERSION}.tar.gz"
WEBP_SITE="https://storage.googleapis.com/downloads.webmproject.org/releases/webp"
if [ ! -s "$SDK_HOME/lib/pkgconfig/libwebp.pc" ] || [ "$(pkg-config --modversion libwebp)" != "$WEBP_VERSION" ]; then
    start_build "$WEBP_TAR"
    download "$WEBP_SITE" "$WEBP_TAR"
    untar "$SRC_PATH/$WEBP_TAR"
    pushd "libwebp-${WEBP_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-static --enable-shared --enable-libwebpmux --enable-libwebpdemux --enable-libwebpdecoder --enable-libwebpextras --disable-silent-rules --enable-swap-16bit-csp
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libwebp-${WEBP_VERSION}"
    end_build "$WEBP_TAR"
fi

# Install oiio
#OIIO_VERSION=1.7.19
OIIO_VERSION=1.8.12
OIIO_VERSION_SHORT=${OIIO_VERSION%.*}
OIIO_TAR="oiio-Release-${OIIO_VERSION}.tar.gz"
if [ "${REBUILD_OIIO:-}" = "1" ]; then
    rm -rf "$SDK_HOME/lib/libOpenImage"* "$SDK_HOME/include/OpenImage"* || true
fi
if [ ! -s "$SDK_HOME/lib/libOpenImageIO.so" ]; then
    start_build "openimageio-${OIIO_VERSION}"
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
    end_build "openimageio-${OIIO_VERSION}"
fi

# Install SeExpr
SEEXPR_VERSION=2.11
SEEXPR_TAR="SeExpr-${SEEXPR_VERSION}.tar.gz"
if [ ! -s "$SDK_HOME/lib/libSeExpr.so" ]; then
    start_build "$SEEXPR_TAR"
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
    end_build "$SEEXPR_TAR"
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
if [ ! -s "$SDK_HOME/lib/libmp3lame.so" ] || [ "$("$SDK_HOME/bin/lame" --version |head -1 | sed -e 's/^.*[Vv]ersion \([^ ]*\) .*$/\1/')" != "$LAME_VERSION" ]; then
    start_build "$LAME_TAR"
    download "$LAME_SITE" "$LAME_TAR"
    untar "$SRC_PATH/$LAME_TAR"
    pushd "lame-${LAME_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-mp3rtp --enable-shared --disable-static --enable-nasm
    make -j${MKJOBS}
    make install
    popd
    rm -rf "lame-${LAME_VERSION}"
    end_build "$LAME_TAR"
fi

# Install libogg
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/libogg.html
LIBOGG_VERSION=1.3.3
LIBOGG_TAR="libogg-${LIBOGG_VERSION}.tar.gz"
LIBOGG_SITE="http://downloads.xiph.org/releases/ogg"
if [ ! -s "$SDK_HOME/lib/pkgconfig/ogg.pc" ] || [ "$(pkg-config --modversion ogg)" != "$LIBOGG_VERSION" ]; then
    start_build "$LIBOGG_TAR"
    download "$LIBOGG_SITE" "$LIBOGG_TAR"
    untar "$SRC_PATH/$LIBOGG_TAR"
    pushd "libogg-${LIBOGG_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libogg-${LIBOGG_VERSION}"
    end_build "$LIBOGG_TAR"
fi

# Install libvorbis
# http://www.linuxfromscratch.org/blfs/view/cvs/multimedia/libvorbis.html
LIBVORBIS_VERSION=1.3.6
LIBVORBIS_TAR="libvorbis-${LIBVORBIS_VERSION}.tar.gz"
LIBVORBIS_SITE="http://downloads.xiph.org/releases/vorbis"
if [ ! -s "$SDK_HOME/lib/pkgconfig/vorbis.pc" ] || [ "$(pkg-config --modversion vorbis)" != "$LIBVORBIS_VERSION" ]; then
    start_build "$LIBVORBIS_TAR"
    download "$LIBVORBIS_SITE" "$LIBVORBIS_TAR"
    untar "$SRC_PATH/$LIBVORBIS_TAR"
    pushd "libvorbis-${LIBVORBIS_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libvorbis-${LIBVORBIS_VERSION}"
    end_build "$LIBVORBIS_TAR"
fi

# Install libtheora
# see http://www.linuxfromscratch.org/blfs/view/cvs/multimedia/libtheora.html
LIBTHEORA_VERSION=1.1.1
LIBTHEORA_TAR="libtheora-${LIBTHEORA_VERSION}.tar.bz2"
LIBTHEORA_SITE="http://downloads.xiph.org/releases/theora"
if [ ! -s "$SDK_HOME/lib/pkgconfig/theora.pc" ] || [ "$(pkg-config --modversion theora)" != "$LIBTHEORA_VERSION" ]; then
    start_build "$LIBTHEORA_TAR"
    download "$LIBTHEORA_SITE" "$LIBTHEORA_TAR"
    untar "$SRC_PATH/$LIBTHEORA_TAR"
    pushd "libtheora-${LIBTHEORA_VERSION}"
    $GSED -i 's/png_\(sizeof\)/\1/g' examples/png2theora.c # fix libpng16
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libtheora-${LIBTHEORA_VERSION}"
    end_build "$LIBTHEORA_TAR"
fi

# Install libmodplug
LIBMODPLUG_VERSION=0.8.9.0
LIBMODPLUG_TAR="libmodplug-${LIBMODPLUG_VERSION}.tar.gz"
LIBMODPLUG_SITE="https://sourceforge.net/projects/modplug-xmms/files/libmodplug/${LIBMODPLUG_VERSION}"
if [ ! -s "$SDK_HOME/lib/libmodplug.so" ]; then
    start_build "$LIBMODPLUG_TAR"
    download "$LIBMODPLUG_SITE" "$LIBMODPLUG_TAR"
    untar "$SRC_PATH/$LIBMODPLUG_TAR"
    pushd "libmodplug-$LIBMODPLUG_VERSION"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --disable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libmodplug-$LIBMODPLUG_VERSION"
    end_build "$LIBMODPLUG_TAR"
fi

# Install libvpx
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/libvpx.html
LIBVPX_VERSION=1.7.0
LIBVPX_TAR="libvpx-${LIBVPX_VERSION}.tar.gz"
#LIBVPX_SITE=http://storage.googleapis.com/downloads.webmproject.org/releases/webm
if [ "${REBUILD_LIBVPX:-}" = "1" ]; then
    rm -rf "$SDK_HOME"/lib/libvpx* || true
    rm -rf "$SDK_HOME"/lib/pkgconfig/vpx.pc || true
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/vpx.pc" ] || [ "$(pkg-config --modversion vpx)" != "$LIBVPX_VERSION" ]; then
    start_build "$LIBVPX_TAR"
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
    end_build "$LIBVPX_TAR"
fi

# Install speex (EOL, use opus)
# see http://www.linuxfromscratch.org/blfs/view/stable/multimedia/speex.html
SPEEX_VERSION=1.2.0
SPEEX_TAR="speex-${SPEEX_VERSION}.tar.gz"
SPEEX_SITE="http://downloads.us.xiph.org/releases/speex"
if [ ! -s "$SDK_HOME/lib/pkgconfig/speex.pc" ] || [ "$(pkg-config --modversion speex)" != "$SPEEX_VERSION" ]; then
    start_build "$SPEEX_TAR"
    download "$SPEEX_SITE" "$SPEEX_TAR"
    untar "$SRC_PATH/$SPEEX_TAR"
    pushd "speex-$SPEEX_VERSION"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "speex-$SPEEX_VERSION"
    end_build "$SPEEX_TAR"
fi

# Install opus
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/opus.html
OPUS_VERSION=1.2.1
OPUS_TAR="opus-${OPUS_VERSION}.tar.gz"
OPUS_SITE="https://archive.mozilla.org/pub/opus"
if [ ! -s "$SDK_HOME/lib/pkgconfig/opus.pc" ] || [ "$(pkg-config --modversion opus)" != "$OPUS_VERSION" ]; then
    start_build "$OPUS_TAR"
    download "$OPUS_SITE" "$OPUS_TAR"
    untar "$SRC_PATH/$OPUS_TAR"
    pushd "opus-$OPUS_VERSION"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static --enable-custom-modes
    make -j${MKJOBS}
    make install
    popd
    rm -rf "opus-$OPUS_VERSION"
    end_build "$OPUS_TAR"
fi

# Install orc
ORC_VERSION=0.4.28
ORC_TAR="orc-${ORC_VERSION}.tar.xz"
ORC_SITE="http://gstreamer.freedesktop.org/src/orc"
if [ ! -s "$SDK_HOME/lib/pkgconfig/orc-0.4.pc" ] || [ "$(pkg-config --modversion orc-0.4)" != "$ORC_VERSION" ]; then
    start_build "$ORC_TAR"
    download "$ORC_SITE" "$ORC_TAR"
    untar "$SRC_PATH/$ORC_TAR"
    pushd "orc-$ORC_VERSION"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static
    make -j${MKJOBS}
    make install
    popd
    rm -rf "orc-$ORC_VERSION"
    end_build "$ORC_TAR"
fi

# Install dirac (obsolete since ffmpeg-3.4)
if false; then
DIRAC_VERSION=1.0.11
DIRAC_TAR=schroedinger-${DIRAC_VERSION}.tar.gz
DIRAC_SITE="http://diracvideo.org/download/schroedinger"
if [ ! -s "$SDK_HOME/lib/pkgconfig/schroedinger-1.0.pc" ] || [ "$(pkg-config --modversion schroedinger-1.0)" != "$DIRAC_VERSION" ]; then
    start_build "$DIRAC_TAR"
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
    end_build "$DIRAC_TAR"
fi
fi

# x264
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/x264.html
X264_VERSION=20180212-2245-stable
X264_VERSION_PKG=0.152.x
X264_TAR="x264-snapshot-${X264_VERSION}.tar.bz2"
X264_SITE="https://download.videolan.org/x264/snapshots"
if [ "${REBUILD_X264:-}" = "1" ]; then
    REBUILD_FFMPEG=1
    rm -rf "$SDK_HOME"/lib/libx264* || true
    rm -rf "$SDK_HOME"/lib/pkgconfig/x264* || true
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/x264.pc" ] || [ "$(pkg-config --modversion x264)" != "$X264_VERSION_PKG" ]; then
    start_build "$X264_TAR"
    download "$X264_SITE" "$X264_TAR"
    untar "$SRC_PATH/$X264_TAR"
    pushd "x264-snapshot-${X264_VERSION}"
    # add --bit-depth=10 to build a x264 that can only produce 10bit videos
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --libdir="$SDK_HOME/lib" --enable-shared --enable-static --enable-pic --disable-cli
    make -j${MKJOBS}
    make install
    popd
    rm -rf "x264-snapshot-${X264_VERSION}"
    end_build "$X264_TAR"
fi

# x265
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/x265.html
# and https://git.archlinux.org/svntogit/packages.git/tree/trunk/PKGBUILD?h=packages/x265
X265_VERSION=2.8
X265_TAR="x265_${X265_VERSION}.tar.gz"
X265_SITE="https://bitbucket.org/multicoreware/x265/downloads"
if [ "${REBUILD_X265:-}" = "1" ]; then
    rm -rf "$SDK_HOME"/lib/libx265* || true
    rm -rf "$SDK_HOME"/lib/pkgconfig/x265* || true
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/x265.pc" ] || [ "$(pkg-config --modversion x265)" != "$X265_VERSION" ]; then
    start_build "$X265_TAR"
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
    end_build "$X265_TAR"
fi

# xvid
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/xvid.html
XVID_VERSION=1.3.5
XVID_TAR="xvidcore-${XVID_VERSION}.tar.gz"
XVID_SITE="http://downloads.xvid.org/downloads"
if [ ! -s "$SDK_HOME/lib/libxvidcore.so" ]; then
    start_build "$XVID_TAR"
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
    end_build "$XVID_TAR"
fi

# install soxr
SOXR_VERSION=0.1.2
SOXR_TAR="soxr-${SOXR_VERSION}-Source.tar.xz"
SOXR_SITE="https://sourceforge.net/projects/soxr/files"
if [ ! -s "$SDK_HOME/lib/pkgconfig/soxr.pc" ] || [ "$(pkg-config --modversion soxr)" != "$SOXR_VERSION" ]; then
    start_build "$SOXR_TAR"
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
    end_build "$SOXR_TAR"
fi

# Install fribidi (for libass and ffmpeg)
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/fribidi.html
FRIBIDI_VERSION=1.0.4
FRIBIDI_TAR="fribidi-${FRIBIDI_VERSION}.tar.bz2"
FRIBIDI_SITE="https://github.com/fribidi/fribidi/releases/download/v${FRIBIDI_VERSION}"
if [ ! -s "$SDK_HOME/lib/pkgconfig/fribidi.pc" ] || [ "$(pkg-config --modversion fribidi)" != "$FRIBIDI_VERSION" ]; then
    REBUILD_FFMPEG=1
    start_build "$FRIBIDI_TAR"
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
    end_build "$FRIBIDI_TAR"
fi

# Install libass (for ffmpeg)
# see http://www.linuxfromscratch.org/blfs/view/cvs/multimedia/libass.html
LIBASS_VERSION=0.14.0
LIBASS_TAR="libass-${LIBASS_VERSION}.tar.xz"
LIBASS_SITE="https://github.com/libass/libass/releases/download/${LIBASS_VERSION}"
if [ ! -s "$SDK_HOME/lib/pkgconfig/libass.pc" ] || [ "$(pkg-config --modversion libass)" != "$LIBASS_VERSION" ]; then
    REBUILD_FFMPEG=1
    start_build "$LIBASS_TAR"
    download "$LIBASS_SITE" "$LIBASS_TAR"
    untar "$SRC_PATH/$LIBASS_TAR"
    pushd "libass-${LIBASS_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libass-${LIBASS_VERSION}"
    end_build "$LIBASS_TAR"
fi

# Install libbluray (for ffmpeg)
# BD-Java support is disabled, because it requires apache-ant and a JDK for building
LIBBLURAY_VERSION=1.0.2
LIBBLURAY_TAR="libbluray-${LIBBLURAY_VERSION}.tar.bz2"
LIBBLURAY_SITE="ftp://ftp.videolan.org/pub/videolan/libbluray/${LIBBLURAY_VERSION}"
if [ ! -s "$SDK_HOME/lib/pkgconfig/libbluray.pc" ] || [ "$(pkg-config --modversion libbluray)" != "$LIBBLURAY_VERSION" ]; then
    REBUILD_FFMPEG=1
    start_build "$LIBBLURAY_TAR"
    download "$LIBBLURAY_SITE" "$LIBBLURAY_TAR"
    untar "$SRC_PATH/$LIBBLURAY_TAR"
    pushd "libbluray-${LIBBLURAY_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-static --enable-shared --disable-bdjava --disable-bdjava-jar
    make -j${MKJOBS}
    make install
    popd
    rm -rf "libbluray-${LIBBLURAY_VERSION}"
    end_build "$LIBBLURAY_TAR"
fi

# Install openh264 (for ffmpeg)
OPENH264_VERSION=1.7.0
OPENH264_TAR="openh264-${OPENH264_VERSION}.tar.gz"
#OPENH264_SITE="https://github.com/cisco/openh264/archive"
if [ ! -s "$SDK_HOME/lib/pkgconfig/openh264.pc" ] || [ "$(pkg-config --modversion openh264)" != "$OPENH264_VERSION" ]; then
    REBUILD_FFMPEG=1
    start_build "$OPENH264_TAR"
    download_github cisco openh264 "${OPENH264_VERSION}" v "${OPENH264_TAR}"
    #download "$OPENH264_SITE" "$OPENH264_TAR"
    untar "$SRC_PATH/$OPENH264_TAR"
    pushd "openh264-${OPENH264_VERSION}"
    # AVX2 is only supported from Kaby Lake on
    # see https://en.wikipedia.org/wiki/Intel_Core
    make HAVE_AVX2=No PREFIX="$SDK_HOME" install
    popd
    rm -rf "openh264-${OPENH264_VERSION}"
    end_build "$OPENH264_TAR"
fi

# Install snappy (for ffmpeg)
SNAPPY_VERSION=1.1.7
SNAPPY_TAR="snappy-${SNAPPY_VERSION}.tar.gz"
SNAPPY_SITE="https://github.com/google/snappy/releases/download/${SNAPPY_VERSION}"
if [ ! -s "$SDK_HOME/lib/libsnappy.so" ]; then
    REBUILD_FFMPEG=1
    start_build "$SNAPPY_TAR"
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
    end_build "$SNAPPY_TAR"
fi

# Install FFmpeg
# see http://www.linuxfromscratch.org/blfs/view/svn/multimedia/ffmpeg.html
FFMPEG_VERSION=4.0.1
FFMPEG_VERSION_LIBAVCODEC=58.18.100
FFMPEG_TAR="ffmpeg-${FFMPEG_VERSION}.tar.bz2"
FFMPEG_SITE="http://www.ffmpeg.org/releases"
if [ "${REBUILD_FFMPEG:-}" = "1" ]; then
    rm -rf "$SDK_HOME"/ffmpeg-* || true
fi
if [ ! -d "$SDK_HOME/ffmpeg-gpl2" ] || [ ! -d "$SDK_HOME/ffmpeg-lgpl" ] || [ "$(env PKG_CONFIG_PATH=$SDK_HOME/ffmpeg-gpl2/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion libavcodec)" != "$FFMPEG_VERSION_LIBAVCODEC" ]; then
    start_build "$FFMPEG_TAR"
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
    end_build "$FFMPEG_TAR"
fi

# Install dbus (for QtDBus)
# see http://www.linuxfromscratch.org/lfs/view/systemd/chapter06/dbus.html
DBUS_VERSION=1.12.8
DBUS_TAR="dbus-${DBUS_VERSION}.tar.gz"
DBUS_SITE="https://dbus.freedesktop.org/releases/dbus"
if [ ! -s "$SDK_HOME/lib/pkgconfig/dbus-1.pc" ] || [ "$(pkg-config --modversion dbus-1)" != "$DBUS_VERSION" ]; then
    start_build "$DBUS_TAR"
    download "$DBUS_SITE" "$DBUS_TAR"
    untar "$SRC_PATH/$DBUS_TAR"
    pushd "dbus-${DBUS_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-docs --disable-doxygen-docs --disable-xml-docs --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "dbus-${DBUS_VERSION}"
    end_build "$DBUS_TAR"
fi


# Install ruby (necessary for qtwebkit)
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/ruby.html
RUBY_VERSION=2.4.3
RUBY_VERSION_SHORT=${RUBY_VERSION%.*}
RUBY_VERSION_PKG=${RUBY_VERSION_SHORT}.0
RUBY_TAR="ruby-${RUBY_VERSION}.tar.xz"
RUBY_SITE="http://cache.ruby-lang.org/pub/ruby/${RUBY_VERSION_SHORT}"
if [ "${REBUILD_RUBY:-}" = "1" ]; then
    rm -rf "$SDK_HOME/lib/pkgconfig/ruby"*.pc
fi
if [ ! -s "$SDK_HOME/lib/pkgconfig/ruby-${RUBY_VERSION_SHORT}.pc" ] || [ "$(pkg-config --modversion "ruby-${RUBY_VERSION_SHORT}")" != "$RUBY_VERSION_PKG" ]; then
    start_build "$RUBY_TAR"
    download "$RUBY_SITE" "$RUBY_TAR"
    untar "$SRC_PATH/$RUBY_TAR"
    pushd "ruby-${RUBY_VERSION}"
    env CFLAGS="$BF" ./configure --prefix="$SDK_HOME" --disable-install-doc --disable-install-rdoc --disable-static --enable-shared
    make -j${MKJOBS}
    make install
    popd
    rm -rf "ruby-${RUBY_VERSION}"
    end_build "$RUBY_TAR"
fi

# Install breakpad tools
if [ ! -s "${SDK_HOME}/bin/dump_syms" ]; then

    GIT_BREAKPAD=https://github.com/olear/breakpad.git # breakpad for server-side or linux dump_syms, not for Natron!
    GIT_BREAKPAD_COMMIT=f264b48eb0ed8f0893b08f4f9c7ae9685090ccb8

    start_build "breakpad-olear-${GIT_BREAKPAD_COMMIT}"
    rm -f breakpad || true
    git_clone_commit "$GIT_BREAKPAD" "$GIT_BREAKPAD_COMMIT"
    pushd breakpad
    git submodule update -i
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure -prefix "$SDK_HOME"
    make
    cp src/tools/linux/dump_syms/dump_syms "$SDK_HOME/bin/"
    popd
    rm -rf breakpad
    end_build "breakpad-olear-${GIT_BREAKPAD_COMMIT}"
fi

# Install valgrind
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/valgrind.html
VALGRIND_VERSION=3.13.0
VALGRIND_TAR="valgrind-${VALGRIND_VERSION}.tar.bz2"
VALGRIND_SITE="https://sourceware.org/ftp/valgrind"
if [ ! -s "$SDK_HOME/bin/valgrind" ]; then
    start_build "$VALGRIND_TAR"
    download "$VALGRIND_SITE" "$VALGRIND_TAR"
    untar "$SRC_PATH/$VALGRIND_TAR"
    pushd "valgrind-${VALGRIND_VERSION}"
    sed -i '1904s/4/5/' coregrind/m_syswrap/syswrap-linux.c
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-only"${BITS}"bit
    make -j${MKJOBS}
    make install
    popd
    rm -rf "valgrind-${VALGRIND_VERSION}"
    end_build "$VALGRIND_TAR"
fi

# install gdb (requires xz, zlib ncurses, python2, texinfo)
# see http://www.linuxfromscratch.org/blfs/view/cvs/general/gdb.html
GDB_VERSION=8.1
GDB_TAR="gdb-${GDB_VERSION}.tar.xz" # when using the sdk during debug we get a conflict with native gdb, so bundle our own
GDB_SITE="ftp://ftp.gnu.org/gnu/gdb"
if [ ! -s "$SDK_HOME/bin/gdb" ]; then
    start_build "$GDB_TAR"
    download "$GDB_SITE" "$GDB_TAR"
    untar "$SRC_PATH/$GDB_TAR"
    pushd "gdb-${GDB_VERSION}"
    env CFLAGS="$BF" CXXFLAGS="$BF" ./configure --prefix="$SDK_HOME" --enable-tui --with-system-readline --without-guile
    make -j${MKJOBS}
    make install
    popd
    rm -rf "gdb-${GDB_VERSION}"
    end_build "$GDB_TAR"
fi


if [ "${REBUILD_QT5:-}" = "1" ]; then
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
if [ ! -s "$QT5PREFIX/lib/pkgconfig/Qt5Core.pc" ] || [ "$(env PKG_CONFIG_PATH=$QT5PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion Qt5Core)" != "$QT5_VERSION" ]; then
    start_build "qt5-${QT5_VERSION}"

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
    end_build "qt5-${QT5_VERSION}"

    for (( i=0; i<${#QT_MODULES[@]}; i++ )); do
        module=${QT_MODULES[i]}
        moduleurl=${QT_MODULES_URL[i]}
        start_build "${module}-${QT5_VERSION}"
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
            echo "Error: more than one match for ${module}-*: ${dirs[*]}"
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

        end_build "${module}-${QT5_VERSION}"
    done
fi


# pysetup
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


# Install shiboken2. Requires clang. Not required for PySide2 5.6, which bundles shiboken2
SHIBOKEN2_VERSION=2
SHIBOKEN2_GIT="https://code.qt.io/pyside/shiboken"
SHIBOKEN2_COMMIT="5.9"
# SKIP: not required when using pyside-setup.git, see below
if false; then #[ ! -s "$QT5PREFIX/lib/pkgconfig/shiboken2.pc" ] || [ "$(env PKG_CONFIG_PATH=$QT5PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion "shiboken2")" != "$SHIBOKEN2_VERSION" ]; then
    start_build "shiboken2"
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
    end_build "shiboken2"
fi

# Install pyside2
PYSIDE2_VERSION="5.6.0"
PYSIDE2_GIT="https://code.qt.io/pyside/pyside-setup"
PYSIDE2_COMMIT="5.6" # 5.9 is unstable, see https://github.com/fredrikaverpil/pyside2-wheels/issues/55
if [ ! -x "$SDK_HOME/lib/python${PY2_VERSION_SHORT}/site-packages/PySide2/shiboken2" ] || [ "$($SDK_HOME/lib/python${PY2_VERSION_SHORT}/site-packages/PySide2/shiboken2 --version|head -1 | sed -e 's/^.* v\([^ ]*\)/\1/')" != "$PYSIDE2_VERSION" ]; then

    start_build "pyside2-${PYSIDE2_COMMIT}"
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
    end_build "pyside2-${PYSIDE2_COMMIT}"
fi



if [ "${REBUILD_QT4:-}" = "1" ]; then
    rm -rf "$QT4PREFIX" || true
fi
# Qt4
export QTDIR="$QT4PREFIX"
if [ ! -s "$QT4PREFIX/lib/pkgconfig/QtCore.pc" ] || [ "$(env PKG_CONFIG_PATH=$QT4PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion QtCore)" != "$QT4_VERSION" ]; then
    start_build "$QT4_TAR"
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
    end_build "$QT4_TAR"
fi
if [ ! -s "$QT4PREFIX/lib/pkgconfig/QtWebKit.pc" ] || [ "$(env PKG_CONFIG_PATH=$QT4PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion QtWebKit)" != "$QT4WEBKIT_VERSION_PKG" ]; then
    start_build "$QT4WEBKIT_TAR"
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
    end_build "$QT4WEBKIT_TAR"
fi

# add qt4 to lib path to build shiboken and pyside
LD_LIBRARY_PATH="$QT4PREFIX/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
#LD_RUN_PATH="$LD_LIBRARY_PATH"

# Install shiboken
SHIBOKEN_VERSION=1.2.4
#SHIBOKEN_TAR="shiboken-${SHIBOKEN_VERSION}.tar.bz2"
#SHIBOKEN_SITE="https://download.qt.io/official_releases/pyside"
SHIBOKEN_TAR="Shiboken-${SHIBOKEN_VERSION}.tar.gz"
SHIBOKEN_PREFIX="$QT4PREFIX"
if [ ! -s "$SHIBOKEN_PREFIX/lib/pkgconfig/shiboken.pc" ] || [ "$(env PKG_CONFIG_PATH=$SHIBOKEN_PREFIX/lib/pkgconfig:$QT4PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion shiboken)" != "$SHIBOKEN_VERSION" ]; then
    start_build "$SHIBOKEN_TAR"
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
    end_build "$SHIBOKEN_TAR"
fi

# Install pyside
PYSIDE_VERSION=1.2.4
#PYSIDE_TAR="pyside-qt4.8+${PYSIDE_VERSION}.tar.bz2"
#PYSIDE_SITE="https://download.qt.io/official_releases/pyside"
PYSIDE_TAR="PySide-${PYSIDE_VERSION}.tar.gz"
PYSIDE_PREFIX="$QT4PREFIX"
if [ ! -s "$PYSIDE_PREFIX/lib/pkgconfig/pyside${PYSIDE_V:-}.pc" ] || [ "$(env PKG_CONFIG_PATH=$PYSIDE_PREFIX/lib/pkgconfig:$PKG_CONFIG_PATH pkg-config --modversion pyside)" != "$PYSIDE_VERSION" ]; then

    start_build "$PYSIDE_TAR"
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
    end_build "$PYSIDE_TAR"
fi

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
for subdir in . ffmpeg-gpl2 ffmpeg-lgpl libraw-gpl2 libraw-lgpl qt4 qt5; do
    LD_LIBRARY_PATH="$SDK_HOME/$subdir/lib:${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" find "$SDK_HOME/$subdir/lib" -name '*.so' -exec bash -c 'ldd {} 2>&1 | fgrep "not found" &>/dev/null' \; -print
    LD_LIBRARY_PATH="$SDK_HOME/$subdir/lib:${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" find "$SDK_HOME/$subdir/bin" -maxdepth 1 -exec bash -c 'ldd {} 2>&1 | fgrep "not found" &>/dev/null' \; -print
done
echo "Check for broken libraries and binaries... done!"

echo
echo "Natron SDK Done"
echo
exit 0


# Local variables:
# mode: shell-script
# sh-basic-offset: 4
# sh-indent-comment: t
# indent-tabs-mode: nil
# End:

